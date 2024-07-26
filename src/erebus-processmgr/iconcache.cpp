#include "iconcache.hpp"

#include <erebus/util/exceptionutil.hxx>
#include <erebus/system/thread.hxx>
#include <erebus-desktop/erebus-desktop.hxx>

#include <QPixmap>


namespace Erp
{

namespace Private
{

IconCache::~IconCache()
{
}

IconCache::IconCache(std::shared_ptr<void> channel, Er::Log::ILog* log)
    : m_client(Er::Client::createClient(channel, log))
    , m_log(log)
    , m_worker(std::jthread([this](std::stop_token stop) { worker(stop); }))
{
}

void IconCache::worker(std::stop_token stop) noexcept
{
    Er::System::CurrentThread::setName("IconCache");

    ErLogDebug(m_log, "IconCache worker started");

    try
    {
        auto sessionId = m_client->beginSession(Er::Desktop::Requests::QueryIcon);

        while (!stop.stop_requested())
        {
            std::unique_lock l(m_mutex);

            if (m_pendingCv.wait(l, stop, [this]() { return !m_pending.empty(); }))
            {
                auto process = m_pending.front();
                m_pending.pop();

                auto icon = requestIcon(process->pid, sessionId);

                {
                    Er::ObjectLock<ProcessInfo> locked(*process);

                    if (locked->icon.state != ProcessInfo::IconData::State::Valid)
                        locked->icon = std::move(icon);
                }
            }
        }

        m_client->endSession(Er::Desktop::Requests::QueryIcon, sessionId);
    }
    catch (Er::Exception& e)
    {
        Er::Util::logException(m_log, Er::Log::Level::Warning, e);
    }
    catch (std::exception& e)
    {
        Er::Util::logException(m_log, Er::Log::Level::Warning, e);
    }

    ErLogDebug(m_log, "IconCache worker exited");
}

ProcessInformation::IconData IconCache::requestIcon(uint64_t pid, Er::Client::IClient::SessionId sessionId) noexcept
{
    ProcessInformation::IconData result;

    try
    {
        Er::PropertyBag req;
        Er::addProperty<Er::Desktop::Props::IconSize>(req, uint32_t(Er::Desktop::IconSize::Small));
        Er::addProperty<Er::Desktop::Props::Pid>(req, pid);

        auto reply = m_client->request(Er::Desktop::Requests::QueryIcon, req, sessionId);

        auto status = Er::getPropertyValue<Er::Desktop::Props::IconState>(reply);
        if (!status)
        {
            ErLogError(m_log, "No icon status returned for PID %zu", pid);
            result.state = ProcessInformation::IconData::State::Invalid;
            return result;
        }

        if (*status == static_cast<uint32_t>(Er::Desktop::IconState::Pending))
        {
            ErLogDebug(m_log, "Pending icon for PID %zu", pid);
            result.state = ProcessInformation::IconData::State::Pending;
            return result;
        }

        if (*status == static_cast<uint32_t>(Er::Desktop::IconState::Found))
        {
            auto rawIcon = Er::getPropertyValue<Er::Desktop::Props::Icon>(reply);
            if (rawIcon)
            {
                QPixmap pixmap;
                if (!pixmap.loadFromData(reinterpret_cast<const uchar*>(rawIcon->data()), rawIcon->size()))
                {
                    ErLogWarning(m_log, "Failed to load icon for PID %zu", pid);
                    result.state = ProcessInformation::IconData::State::Invalid;
                    return result;
                }

                result.icon = QIcon(pixmap);

                ErLogDebug(m_log, "Found an icon for PID %zu", pid);
                result.state = ProcessInformation::IconData::State::Valid;
                return result;
            }
        }

        ErLogWarning(m_log, "No icon found for PID %zu", pid);
    }
    catch (Er::Exception& e)
    {
        Er::Util::logException(m_log, Er::Log::Level::Warning, e);
    }
    catch (std::exception& e)
    {
        Er::Util::logException(m_log, Er::Log::Level::Warning, e);
    }

    result.state = ProcessInformation::IconData::State::Invalid;
    return result;
}

void IconCache::requestIcon(ProcessInfoPtr process) noexcept
{
    try
    {
        {
            Er::ObjectLock<ProcessInfo> locked(*process);

            if (locked->icon.state != ProcessInformation::IconData::State::Undefined)
            {
                return; // nothing to do
            }
        }

        {
            std::unique_lock l(m_mutex);

            m_pending.push(process);
        }

        m_pendingCv.notify_one();
    }
    catch (Er::Exception& e)
    {
        Er::Util::logException(m_log, Er::Log::Level::Warning, e);
    }
    catch (std::exception& e)
    {
        Er::Util::logException(m_log, Er::Log::Level::Warning, e);
    }
}

} // namespace Private {}

} // namespace Erp {}
