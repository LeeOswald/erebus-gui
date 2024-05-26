#pragma once


#include "iconcache.hpp"

#include <erebus/condition.hxx>
#include <erebus/log.hxx>
#include <erebus-desktop/erebus-desktop.hxx>

#include <thread>
#include <vector>


namespace ErIc
{

class IconServer final
    : public Er::NonCopyable
{
public:
    ~IconServer();
    IconServer(Er::Log::ILog* log, Er::Event* crashEvent, std::shared_ptr<Er::Desktop::IIconCacheIpc> ipc, IconCache& cache, unsigned workers);

private:
    void worker(std::stop_token stop) noexcept;
    bool heartbeat(std::stop_token stop) noexcept;

    Er::Log::ILog* m_log;
    Er::Event* m_crashEvent;
    std::shared_ptr<Er::Desktop::IIconCacheIpc> m_ipc;
    IconCache& m_cache;
    std::vector<std::unique_ptr<std::jthread>> m_workers;
};

} // namespace ErIc {}
