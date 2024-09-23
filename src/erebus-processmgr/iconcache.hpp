#pragma once

#include "processinfo.hpp"

#include <erebus/log.hxx>
#include <erebus-clt/erebus-clt.hxx>

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>


namespace Erp
{

namespace ProcessMgr
{


class IconCache
    : public Er::NonCopyable
{
public:
    using ProcessInfo = LockableTrackableProcessInformation;
    using ProcessInfoPtr = LockableTrackableProcessInformationPtr;

    ~IconCache();
    explicit IconCache(Er::Client::ChannelPtr channel, Er::Log::ILog* log);

    void requestIcon(ProcessInfoPtr process) noexcept;

private:
    void worker(std::stop_token stop) noexcept;
    ProcessInformation::IconData requestIcon(uint64_t pid, Er::Client::IClient::SessionId sessionId) noexcept;

    std::shared_ptr<Er::Client::IClient> m_client;
    Er::Log::ILog* const m_log;
    std::mutex m_mutex;
    std::condition_variable_any m_pendingCv;
    std::jthread m_worker;
    std::queue<ProcessInfoPtr> m_pending;
};


} // namespace ProcessMgr {}

} // namespace Erp {}
