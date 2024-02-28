#include "processlist.hpp"

#include <erebus/exception.hxx>
#include <erebus/mutexpool.hxx>
#include <erebus/system/time.hxx>
#include <erebus/util/exceptionutil.hxx>


#include <unordered_map>

namespace Erp
{

namespace Private
{

ProcessInformation::ProcessInformation(Er::PropertyBag&& bag)
    : properties(std::move(bag))
{
    // find PID (must always be present)
    auto it = properties.find(Er::ProcessProps::Pid::Id::value);
    if (it == properties.end())
        throw Er::Exception(ER_HERE(), "No PID in process properties");

    this->pid = std::any_cast<uint64_t>(it->second.value);

    it = properties.find(Er::ProcessProps::IsDeleted::Id::value);
    if (it != properties.end())
    {
        this->deleted = 1;
        return;
    }

    // find 'Valid' property (must always be present unless the process is deleted)
    it = properties.find(Er::ProcessProps::Valid::Id::value);
    if (it == properties.end())
        throw Er::Exception(ER_HERE(), "No \'valid\' in process properties");

    // find 'Valid' property
    this->valid = std::any_cast<bool>(it->second.value);
    if (!this->valid)
    {
        // maybe we've got an error message
        it = properties.find(Er::ProcessProps::Error::Id::value);
        if (it != properties.end())
            this->error = Erc::fromUtf8(std::any_cast<std::string>(it->second.value));

        return;
    }

    // cache certain props
    for (auto it = properties.begin(); it != properties.end(); ++it)
    {
        switch (it->second.id)
        {
        case Er::ProcessProps::IsNew::Id::value:
            this->added = std::any_cast<bool>(it->second.value);
            break;

        case Er::ProcessProps::PPid::Id::value:
            this->ppid = std::any_cast<uint64_t>(it->second.value);
            break;

        case Er::ProcessProps::StartTime::Id::value:
        {
            this->startTime = std::any_cast<uint64_t>(it->second.value);
            Er::TimeFormatter<"%H:%M:%S %d %b %y", Er::TimeZone::Utc> fmt;
            std::ostringstream ss;
            fmt(it->second, ss);
            this->startTimeUtc = Erc::fromUtf8(ss.str());
            break;
        }

        case Er::ProcessProps::State::Id::value:
            this->processState = Erc::fromUtf8(std::any_cast<std::string>(it->second.value));
            break;

        case Er::ProcessProps::Comm::Id::value:
            this->comm = Erc::fromUtf8(std::any_cast<std::string>(it->second.value));
            break;
        }
    }

    if (this->ppid == InvalidKey)
        this->ppid = this->pid;
}

void ProcessInformation::updateFromDiff(const ProcessInformation& diff)
{
    for (auto it = diff.properties.begin(); it != diff.properties.end(); ++it)
    {
        auto& diffProp = it->second;
        auto myPropIt = this->properties.find(diffProp.id);
        if (myPropIt == this->properties.end())
            this->properties.insert({ diffProp.id, diffProp });
        else
            myPropIt->second = diffProp;

        // update cached props if modified
        switch (diffProp.id)
        {
        case Er::ProcessProps::PPid::Id::value:
            assert(diff.pid != InvalidKey);
            this->ppid = diff.pid;
            break;

        case Er::ProcessProps::StartTime::Id::value:
        {
            assert(diff.startTime > 0);
            this->startTime = diff.startTime;
            assert(!diff.startTimeUtc.isEmpty());
            this->startTimeUtc = diff.startTimeUtc;
            break;
        }

        case Er::ProcessProps::State::Id::value:
            this->processState = diff.processState;
            break;

        case Er::ProcessProps::Comm::Id::value:
            this->comm = diff.comm;
            break;
        }
    }
}

namespace
{

class ProcessListImpl
    : public IProcessList
    , public Er::NonCopyable
{
public:
    ~ProcessListImpl()
    {
        Er::protectedCall<void>(
            m_log,
            LogInstance("ProcessListImpl"),
            [this]()
            {
                m_client->endSession(Er::ProcessRequests::ListProcessesDiff, m_sessionId);
            }
        );
    }

    explicit ProcessListImpl(Er::Client::IClient* client, Er::Log::ILog* log)
        : m_client(client)
        , m_log(log)
        , m_mutexPool(MutexPoolSize)
        , m_sessionId(client->beginSession(Er::ProcessRequests::ListProcessesDiff))
    {
    }

    std::shared_ptr<Changeset> collect(Er::ProcessProps::PropMask required, std::chrono::milliseconds trackThreshold) override
    {
        auto firstRun = m_collection.empty();
        auto now = Item::now();

        auto diff = std::make_shared<Changeset>(firstRun);
        enumerateProcesses(firstRun, now, required, trackThreshold, diff.get());
        trackNewOrDeletedProcesses(now, trackThreshold, diff.get());

        return diff;
    }

private:
    using ItemContainer = std::unordered_map<typename Item::Key, std::shared_ptr<Item>>;
    
    std::shared_ptr<Item> makeProcessItem(Er::PropertyBag&& bag, Item::TimePoint now, bool firstRun) noexcept
    {
        return Er::protectedCall<std::shared_ptr<Item>>(
            m_log,
            LogInstance("ProcessListImpl"),
            [this](Er::PropertyBag&& bag, Item::TimePoint now, bool firstRun)
            {
                ProcessInformation parsed(std::move(bag));
                auto tracked = std::make_shared<Item>(m_mutexPool.mutex(), firstRun, now, std::move(parsed));
                return tracked;

            },
            std::move(bag),
            now,
            firstRun
        );
    }

    void enumerateProcesses(bool firstRun, Item::TimePoint now, Er::ProcessProps::PropMask required, std::chrono::milliseconds trackThreshold, Changeset* diff) noexcept
    {
        Er::protectedCall<void>(
            m_log,
            LogInstance("ProcessListImpl"),
            [this](bool firstRun, Item::TimePoint now, Er::ProcessProps::PropMask required, std::chrono::milliseconds trackThreshold, Changeset* diff)
            {
                return enumerateProcessesImpl(firstRun, now, required, trackThreshold, diff);
            },
            firstRun,
            now,
            required,
            trackThreshold,
            diff
        );
    }

    void enumerateProcessesImpl(bool firstRun, Item::TimePoint now, Er::ProcessProps::PropMask required, std::chrono::milliseconds trackThreshold, Changeset* diff)
    {
        Er::PropertyBag req;
        req.insert({ Er::ProcessProps::RequiredFields::Id::value, Er::Property(Er::ProcessProps::RequiredFields::Id::value, required.pack<uint64_t>()) });

        auto list = m_client->requestStream(Er::ProcessRequests::ListProcessesDiff, req, m_sessionId);
        for (auto& process : list)
        {
            auto parsedProcess = makeProcessItem(std::move(process), now, firstRun);
            if (!parsedProcess)
                continue;

            // is this an existing process?
            auto existing = m_collection.find(parsedProcess->pid);
            if (parsedProcess->deleted)
            {
                if (existing != m_collection.end())
                {
                    // the process has exited; place it into the 'deleted' list unless it's already there
                    Er::ObjectLock<Item> item(existing->second.get());
                    assert(!item->deleted);
                    item->markDeleted(now);
                    m_tracked.insert({ item->pid, existing->second });
                    diff->tracked.insert(existing->second);

                    LogDebug(m_log, LogNowhere(), "DELETED process %zu [%s]", item->pid, Erc::toUtf8(item->comm).c_str());
                }
                else
                {
                    LogWarning(m_log, LogNowhere(), "Unknown exited process %zu", parsedProcess->pid);
                }

                continue;
            }

            if (existing == m_collection.end())
            {
                // this is a new process
                m_collection.insert({ parsedProcess->pid, parsedProcess });
                diff->modified.insert(parsedProcess);

                if (!firstRun)
                {
                    // also track this process as 'new'
                    assert(parsedProcess->state() == Item::State::New);
                    m_tracked.insert({ parsedProcess->pid, parsedProcess });
                    diff->tracked.insert(parsedProcess);

                    LogDebug(m_log, LogNowhere(), "NEW process %zu [%s]", parsedProcess->pid, Erc::toUtf8(parsedProcess->comm).c_str());
                }
                else
                {
                    // this is the first run; all processes are just added w/out marking as 'new'

                    LogDebug(m_log, LogNowhere(), "EXISTING process %zu [%s]", parsedProcess->pid, Erc::toUtf8(parsedProcess->comm).c_str());
                }

                continue;
            }
            
            // this is an existing process and we've just got a few fields updated
            assert(!firstRun);
            Er::ObjectLock<Item> item(existing->second.get());
            item->updateFromDiff(*parsedProcess.get());

            diff->modified.insert(existing->second);

            LogDebug(m_log, LogNowhere(), "MODIFIED process %zu [%s]", item->pid, Erc::toUtf8(item->comm).c_str());
        }
    }

    void trackNewOrDeletedProcesses(Item::TimePoint now, std::chrono::milliseconds trackThreshold, Changeset* diff)
    {
        LogDebug(m_log, LogNowhere(), "trackNewOrDeletedProcesses() ->");

        for (auto it = m_tracked.begin(); it != m_tracked.end();)
        {
            auto ref = it->second;
            Er::ObjectLock<Item> item(ref.get());

            LogDebug(m_log, LogNowhere(), "%zu %s", ref->pid, Erc::toUtf8(ref->comm).c_str());

            if (item->maybeUntrackDeleted(now, trackThreshold))
            {
                LogDebug(m_log, LogNowhere(), "    purging");

                // item has been being marked 'deleted' for quite a long; time to purge it
                auto next = std::next(it);
                
                diff->purged.insert(ref);

                m_tracked.erase(it);
                it = next;

                LogDebug(m_log, LogNowhere(), "DELETED -> PURGED process %zu [%s]", item->pid, Erc::toUtf8(item->comm).c_str());
            }
            else if (item->maybeUntrackNew(now, trackThreshold))
            {
                LogDebug(m_log, LogNowhere(), "    unnewing");

                // item has been being marked 'new' for quite a long
                auto next = std::next(it);

                diff->untracked.insert(it->second);

                m_tracked.erase(it);
                it = next;

                LogDebug(m_log, LogNowhere(), "NEW -> EXISTING process %zu [%s]", item->pid, Erc::toUtf8(item->comm).c_str());
            }
            else
            {
                ++it;
            }
        }

        LogDebug(m_log, LogNowhere(), "trackNewOrDeletedProcesses() <-");
    }


    Er::Client::IClient* m_client;
    Er::Log::ILog* m_log;
    static constexpr size_t MutexPoolSize = 5;
    Er::MutexPool<std::recursive_mutex> m_mutexPool;
    Er::Client::IClient::SessionId m_sessionId;
    ItemContainer m_collection;
    ItemContainer m_tracked; // processes being temporarily tracked as 'recently exited' or 'recently started'
};

} // namespace {}


std::unique_ptr<IProcessList> createProcessList(Er::Client::IClient* client, Er::Log::ILog* log)
{
    return std::make_unique<ProcessListImpl>(client, log);
}

} // namespace Private {}

} // namespace Erp {}
