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

namespace Private
{


class IconCache
    : public Er::NonCopyable
{
public:
    using ProcessInfo = LockableTrackableProcessInformation;
    using ProcessInfoPtr = LockableTrackableProcessInformationPtr;

    ~IconCache();
    explicit IconCache(std::shared_ptr<void> channel, Er::Log::ILog* log);

    void requestIcon(ProcessInfoPtr process) noexcept;

private:
    void worker(std::stop_token stop) noexcept;
    ProcessInformation::IconData requestIcon(uint64_t pid) noexcept;

    std::shared_ptr<Er::Client::IClient> m_client;
    Er::Log::ILog* const m_log;
    std::mutex m_mutex;
    std::condition_variable_any m_pendingCv;
    std::jthread m_worker;
    std::queue<ProcessInfoPtr> m_pending;
};


} // namespace Private {}

} // namespace Erp {}
