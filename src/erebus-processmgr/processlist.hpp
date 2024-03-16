#pragma once

#include "processmgr.hpp"

#include <erebus/lockable.hxx>
#include <erebus/trackable.hxx>
#include <erebus-clt/erebus-clt.hxx>
#include <erebus-processmgr/processprops.hxx>

#include <chrono>
#include <mutex>
#include <set>
#include <vector>

#include <QString>

namespace Erp
{

namespace Private
{


struct ProcessInformation
{
    using Key = Er::ProcessProps::Pid::ValueType;
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


struct IProcessList
{
    using Item = Er::ExternallyLockableObject<TrackableProcessInformation, std::recursive_mutex>;
    using ItemPtr = std::shared_ptr<Item>;

    struct ItemIsPredecessor
    {
        bool operator()(ItemPtr a, ItemPtr b) const noexcept
        {
            return a->pid < b->pid;
        }
    };

    struct ItemIsSuccessor
    {
        bool operator()(ItemPtr a, ItemPtr b) const noexcept
        {
            return b->pid < a->pid;
        }
    };

    struct Changeset
    {
        // insert new items into the view in the order of their PIDs
        // and remove them in the reverse order
        // so that parents are removed only after their children
        // and children are inserted only after their parents

        bool firstRun;
        std::set<ItemPtr, ItemIsPredecessor> modified;  
        std::set<ItemPtr, ItemIsPredecessor> tracked;
        std::set<ItemPtr, ItemIsPredecessor> untracked;
        std::set<ItemPtr, ItemIsSuccessor> purged;
        std::size_t totalProcesses = 0;

        explicit Changeset(bool firstRun) noexcept
            : firstRun(firstRun)
        {
        }
    };

    virtual ~IProcessList() {}
    virtual std::shared_ptr<Changeset> collect(Er::ProcessProps::PropMask required, std::chrono::milliseconds trackThreshold) = 0;
};


std::unique_ptr<IProcessList> createProcessList(Er::Client::IClient* client, Er::Log::ILog* log);

} // namespace Private {}

} // namespace Erp {}



