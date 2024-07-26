#include "server.hpp"

#include <erebus/system/thread.hxx>

namespace Erp
{

namespace IconCache
{


IconServer::~IconServer()
{
    m_workers.clear();
}

IconServer::IconServer(Er::Log::ILog* log, Er::Event* crashEvent, std::shared_ptr<Er::Desktop::IIconCacheIpc> ipc, IconCache& cache, unsigned workers)
    : m_log(log)
    , m_crashEvent(crashEvent)
    , m_ipc(ipc)
    , m_cache(cache)
{
    m_workers.reserve(workers);
    while (workers--)
    {
        m_workers.emplace_back(new std::jthread([this](std::stop_token stop) { worker(stop); }));
    }
}

void IconServer::worker(std::stop_token stop) noexcept
{
    Er::Log::Debug(m_log) << "Icon cache server started";
    Er::System::CurrentThread::setName("iconcache_worker");

    try
    {
        while (!stop.stop_requested() && heartbeat(stop))
        {

        }
    }
    catch (std::exception& e)
    {
        Er::Log::Error(m_log) << "Unexpected exception: " << e.what();

        if (m_crashEvent)
            m_crashEvent->setAndNotifyAll(true);
    }

    Er::Log::Debug(m_log) << "Icon cache server stopped";
}

bool IconServer::heartbeat(std::stop_token stop) noexcept
{
    try
    {
        auto request = m_ipc->pullIconRequest(Timeout);
        if (request && !stop.stop_requested())
        {
            auto icon = m_cache.cacheIcon(request->name, request->size);

            if (icon.type == IconCache::IconInfo::Type::Invalid)
                m_ipc->sendIcon(Er::Desktop::IIconCacheIpc::IconResponse(request->name, request->size, Er::Desktop::IIconCacheIpc::IconResponse::Result::NotFound), Timeout);
            else
                m_ipc->sendIcon(Er::Desktop::IIconCacheIpc::IconResponse(request->name, request->size, Er::Desktop::IIconCacheIpc::IconResponse::Result::Ok, icon.path), Timeout);
        }
    }
    catch (std::exception& e)
    {
        Er::Log::Error(m_log) << "Unexpected exception: " << e.what();
        return false;
    }

    return true;
}

} // namespace IconCache {}

} // namespace Erp {}
