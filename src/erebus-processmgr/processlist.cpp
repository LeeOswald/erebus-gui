#include "processlist.hpp"

#include <erebus/exception.hxx>
#include <erebus/mutexpool.hxx>
#include <erebus/system/time.hxx>
#include <erebus/util/exceptionutil.hxx>


#include <unordered_map>

namespace Erp
{

namespace Private
{

ProcessInformation::ProcessInformation(Er::PropertyBag&& bag)
    : properties(std::move(bag))
{
    // find PID
    auto it = properties.find(Er::ProcessProps::Pid::Id::value);
    if (it == properties.end())
        throw Er::Exception(ER_HERE(), "No PID in process properties");

    this->pid = std::any_cast<uint64_t>(it->second.value);

    it = properties.find(Er::ProcessProps::Valid::Id::value);
    if (it == properties.end())
        throw Er::Exception(ER_HERE(), "No \'valid\' in process properties");

    // find 'Valid' property
    this->valid = std::any_cast<bool>(it->second.value);
    if (!this->valid)
    {
        it = properties.find(Er::ProcessProps::Error::Id::value);
        if (it != properties.end())
            this->error = Erc::fromUtf8(std::any_cast<std::string>(it->second.value));

        return;
    }

    // cache certain props
    for (auto it = properties.begin(); it != properties.end(); ++it)
    {
        switch (it->second.id)
        {
        case Er::ProcessProps::Valid::Id::value:
        case Er::ProcessProps::Error::Id::value:
        case Er::ProcessProps::Pid::Id::value:
            continue;

        case Er::ProcessProps::PPid::Id::value:
            this->ppid = std::any_cast<uint64_t>(it->second.value);
            break;

        case Er::ProcessProps::StartTime::Id::value:
        {
            this->startTime = std::any_cast<uint64_t>(it->second.value);
            Er::TimeFormatter<"%H:%M:%S %d %b %y", Er::TimeZone::Utc> fmt;
            std::ostringstream ss;
            fmt(it->second, ss);
            this->startTimeUtc = Erc::fromUtf8(ss.str());
            break;
        }

        case Er::ProcessProps::State::Id::value:
            this->processState = Erc::fromUtf8(std::any_cast<std::string>(it->second.value));
            break;

        case Er::ProcessProps::Comm::Id::value:
            this->comm = Erc::fromUtf8(std::any_cast<std::string>(it->second.value));
            break;
        }
    }

    if (this->ppid == InvalidKey)
        this->ppid = this->pid;
}


namespace
{

class ProcessListImpl final
    : public IProcessList
    , public Er::NonCopyable
{
public:
    ~ProcessListImpl()
    {
    }

    explicit ProcessListImpl(Er::Client::IClient* client, Er::Log::ILog* log)
        : m_client(client)
        , m_log(log)
        , m_mutexPool(MutexPoolSize)
    {
    }

    std::shared_ptr<Changeset> collect(Er::ProcessProps::PropMask required, std::chrono::milliseconds trackThreshold) override
    {
        m_log->write(Er::Log::Level::Debug, LogInstance("ProcessList"), "collect() ->");

        auto firstRun = m_collection.empty();
        auto now = Item::now();

        auto newCount = enumerateProcesses(firstRun, now, required, trackThreshold);

        auto cleanedItems = trackNewDeletedProcesses(now, trackThreshold);

        std::vector<ItemPtr> items;
        items.reserve(m_collection.size());
        for (auto& p : m_collection)
        {
            items.push_back(p.second);
        }

        m_log->write(Er::Log::Level::Debug, LogInstance("ProcessList"), "collect() <- items: %zu cleaned: %zu ", items.size(), cleanedItems.size());

        return std::make_shared<Changeset>(std::move(items), std::move(cleanedItems));
    }

private:
    std::shared_ptr<Item> makeProcessItem(Er::PropertyBag&& bag, Item::TimePoint now, bool firstRun) noexcept
    {
        try
        {
            ProcessInformation parsed(std::move(bag));
            auto tracked = std::make_shared<Item>(m_mutexPool.mutex(), firstRun, now, std::move(parsed));
            return tracked;

        }
        catch (Er::Exception& e)
        {
            Er::Util::logException(m_log, Er::Log::Level::Error, e);
        }
        catch (std::exception& e)
        {
            Er::Util::logException(m_log, Er::Log::Level::Error, e);
        }

        return std::shared_ptr<Item>();
    }

    size_t enumerateProcesses(bool firstRun, Item::TimePoint now, Er::ProcessProps::PropMask required, std::chrono::milliseconds trackThreshold) noexcept
    {
        size_t newCount = 0;

        try
        {
            Er::PropertyBag req;
            req.insert({ Er::ProcessProps::RequiredFields::Id::value, Er::Property(Er::ProcessProps::RequiredFields::Id::value, required.pack<uint64_t>()) });

            auto list = m_client->requestStream(Er::ProcessRequests::ListProcesses, req);
            for (auto& process : list)
            {
                auto parsedProcess = makeProcessItem(std::move(process), now, firstRun);
                if (!parsedProcess)
                    continue;

                // is this an existing process?
                auto existing = m_collection.find(parsedProcess->pid);
                if (existing == m_collection.end())
                {
                    m_collection.insert({ parsedProcess->pid, parsedProcess });
                    ++newCount;

                    LogDebug(m_log, LogNowhere(), "%s process %zu [%s]", (firstRun ? "EXISTING" : "NEW"), parsedProcess->pid, Erc::toUtf8(parsedProcess->comm).c_str());
                }
                else
                {
                    auto& processData = existing->second;
                    if (processData->state() == Item::State::Deleted)
                    {
                        // we've been enumerating processes for so long that the system reused the PID
                        assert(!firstRun);
                        LogDebug(m_log, LogNowhere(), "%s process %zu [%s]", (firstRun ? "EXISTING" : "NEW"), parsedProcess->pid, Erc::toUtf8(parsedProcess->comm).c_str());

                        std::swap(parsedProcess, processData);

                        ++newCount;
                    }
                    else
                    {
                        // just update the timestamp
                        processData->updateTimeChecked(now);
                    }
                }
            }
        }
        catch (Er::Exception& e)
        {
            Er::Util::logException(m_log, Er::Log::Level::Error, e);
        }
        catch (std::exception& e)
        {
            Er::Util::logException(m_log, Er::Log::Level::Error, e);
        }

        return newCount;
    }

    std::vector<ItemPtr> trackNewDeletedProcesses(Item::TimePoint now, std::chrono::milliseconds trackThreshold)
    {
        std::vector<ItemPtr> cleanedItems;

        auto getItem = [](ItemContainer::iterator it)
        {
            // return a locked object
            return Er::ObjectLock<Item>(it->second.get());
        };

        auto updateItem = [this, &cleanedItems](ItemContainer::iterator it, Item::State newState, Item::State prevState) -> ItemContainer::iterator
        {
            // assume we're inside an object lock
            auto next = std::next(it);

            auto itemPtr = it->second;

            if (newState == Item::State::Cleaned)
            {
                cleanedItems.push_back(itemPtr);

                m_collection.erase(it);
            }

            LogDebug(m_log, LogNowhere(), "%s->%s process %zu [%s]", toString(prevState), toString(newState), itemPtr->pid, Erc::toUtf8(itemPtr->comm).c_str());

            return next;
        };

        Item::trackNewDeleted(m_collection, now, trackThreshold, getItem, updateItem);

        return cleanedItems;
    }

    static const char* toString(Item::State state) noexcept
    {
        switch (state)
        {
        case Item::State::New: return "NEW";
        case Item::State::Existing: return "EXISTING";
        case Item::State::Deleted: return "DELETED";
        case Item::State::Cleaned: return "CLEANED";
        default: return "???";
        }
    }

    using ItemContainer = std::unordered_map<typename Item::Key, std::shared_ptr<Item>>;

    Er::Client::IClient* m_client;
    Er::Log::ILog* m_log;
    static constexpr size_t MutexPoolSize = 5;
    Er::MutexPool<std::recursive_mutex> m_mutexPool;
    ItemContainer m_collection;
};

} // namespace {}


std::unique_ptr<IProcessList> createProcessList(Er::Client::IClient* client, Er::Log::ILog* log)
{
    return std::make_unique<ProcessListImpl>(client, log);
}

} // namespace Private {}

} // namespace Erp {}
