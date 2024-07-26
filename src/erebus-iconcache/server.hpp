#pragma once


#include "iconcache.hpp"

#include <erebus/condition.hxx>
#include <erebus/log.hxx>
#include <erebus-desktop/ic.hxx>

#include <thread>
#include <vector>


namespace Erp
{

namespace IconCache
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

    static constexpr std::chrono::milliseconds Timeout = std::chrono::milliseconds(1000);

    Er::Log::ILog* m_log;
    Er::Event* m_crashEvent;
    std::shared_ptr<Er::Desktop::IIconCacheIpc> m_ipc;
    IconCache& m_cache;
    std::vector<std::unique_ptr<std::jthread>> m_workers;
};


} // namespace IconCache {}

} // namespace Erp {}
