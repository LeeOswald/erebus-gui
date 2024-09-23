#pragma once

#include <erebus-clt/erebus-clt.hxx>

#include "posixresult.hpp"
#include "processinfo.hpp"

#include <set>


namespace Erp::ProcessMgr
{

struct IProcessList
{
    using Item = LockableTrackableProcessInformation;
    using ItemPtr = LockableTrackableProcessInformationPtr;

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
        std::set<ItemPtr, ItemIsPredecessor> iconed;
        std::set<ItemPtr, ItemIsPredecessor> tracked;
        std::set<ItemPtr, ItemIsPredecessor> untracked;
        std::set<ItemPtr, ItemIsSuccessor> purged;
        std::size_t totalProcesses = 0;
        double realTime = 0.0; // clock time diff (sec)
        double cpuTime = 0.0;  // used CPU time diff (sec)

        explicit Changeset(bool firstRun) noexcept
            : firstRun(firstRun)
        {
        }
    };


    virtual ~IProcessList() {}
    virtual std::shared_ptr<Changeset> collect(Er::ProcessMgr::ProcessProps::PropMask required, std::chrono::milliseconds trackThreshold) = 0;
};


std::unique_ptr<IProcessList> createProcessList(Er::Client::ChannelPtr channel, Er::Log::ILog* log);

} // namespace Erp::ProcessMgr {}



