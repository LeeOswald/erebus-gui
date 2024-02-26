#pragma once

#include "processmgr.hpp"

#include <erebus/lockable.hxx>
#include <erebus/trackable.hxx>
#include <erebus-clt/erebus-clt.hxx>
#include <erebus-processmgr/processprops.hxx>

#include <chrono>
#include <mutex>
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
    bool valid = false;
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
    using Items = std::vector<ItemPtr>;

    struct Changeset
    {
        Items items;
        Items removed;

        Changeset(Items&& items, Items&& removed) noexcept
            : items(std::move(items))
            , removed(std::move(removed))
        {
        }
    };

    virtual ~IProcessList() {}
    virtual std::shared_ptr<Changeset> collect(Er::ProcessProps::PropMask required, std::chrono::milliseconds trackThreshold) = 0;
};


std::unique_ptr<IProcessList> createProcessList(Er::Client::IClient* client, Er::Log::ILog* log);

} // namespace Private {}

} // namespace Erp {}



