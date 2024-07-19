#pragma once

#include <erebus/lockable.hxx>
#include <erebus/trackable.hxx>
#include <erebus-processmgr/erebus-processmgr.hxx>

#include <chrono>
#include <mutex>

#include <QIcon>
#include <QString>

namespace Erp
{

namespace Private
{


struct ProcessInformation
{
    using Key = Er::ProcessMgr::ProcessProps::Pid::ValueType;
    static constexpr Key InvalidKey = Key(-1);

    Er::PropertyBag properties;
    void* _context = nullptr;

    // cached for fast access
    unsigned valid:1 = 0;
    unsigned deleted:1 = 0;
    unsigned added:1 = 0;
    QString error;
    Key pid = InvalidKey;
    Key ppid = InvalidKey;
    uint64_t startTime = 0;
    QString startTimeUtc;
    QString processState = QLatin1String("?");
    QString comm;
    double uTime = 0.0; // user CPU time (sec)
    double sTime = 0.0; // system CPU time (sec)
    double uTimePrev = 0.0;
    double sTimePrev = 0.0;
    std::optional<double> uTimeDiff;
    std::optional<double> sTimeDiff;

    struct IconData
    {
        enum class State
        {
            Undefined,
            Pending,
            Invalid,
            Valid
        };

        using Clock = std::chrono::steady_clock;
        Clock::time_point timestamp = Clock::now();
        State state = State::Undefined;
        QIcon icon;
    };

    IconData icon;
    IconData::Clock::time_point iconTimestamp = IconData::Clock::now();

    ProcessInformation() noexcept = default;

    explicit ProcessInformation(Er::PropertyBag&& bag);

    ProcessInformation(const ProcessInformation& o) = delete;
    ProcessInformation& operator=(const ProcessInformation& o) = delete;

    ProcessInformation(ProcessInformation&& o) = default;
    ProcessInformation& operator=(ProcessInformation&& o) = default;

    void updateFromDiff(const ProcessInformation& diff);

    constexpr const Key& key() const noexcept
    {
        return pid;
    }

    constexpr Key& parentKey() noexcept
    {
        return ppid;
    }

    constexpr const Key& parentKey() const noexcept
    {
        return ppid;
    }

    constexpr bool isRoot() const noexcept
    {
        return (parentKey() == key());
    }

    constexpr void* const& context() const noexcept
    {
        return _context;
    }

    constexpr void*& context() noexcept
    {
        return _context;
    }
};


using TrackableProcessInformation = Er::Trackable<ProcessInformation>;
using LockableTrackableProcessInformation = Er::ExternallyLockableObject<TrackableProcessInformation, std::recursive_mutex>;
using LockableTrackableProcessInformationPtr = std::shared_ptr<LockableTrackableProcessInformation>;


} // namespace Private {}

} // namespace Erp {}



