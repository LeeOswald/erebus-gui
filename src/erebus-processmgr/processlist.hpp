#pragma once

#include "processinfo.hpp"

#include <set>


namespace Erp
{

namespace Private
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

    struct PosixResult
    {
        int code = -1;
        std::string message;

        PosixResult() noexcept = default;

        template <typename MessageT>
        PosixResult(int code, MessageT&& message)
            : code(code)
            , message(std::forward<MessageT>(message))
        {}
    };

    virtual ~IProcessList() {}
    virtual std::shared_ptr<Changeset> collect(Er::ProcessProps::PropMask required, std::chrono::milliseconds trackThreshold) = 0;
    virtual PosixResult kill(uint64_t pid, std::string_view signame) = 0;
};


std::unique_ptr<IProcessList> createProcessList(std::shared_ptr<void> channel, Er::Log::ILog* log);

} // namespace Private {}

} // namespace Erp {}



