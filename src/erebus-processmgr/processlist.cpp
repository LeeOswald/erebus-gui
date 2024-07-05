#include "processlist.hpp"

#include <erebus/exception.hxx>
#include <erebus/mutexpool.hxx>
#include <erebus/system/time.hxx>
#include <erebus/util/exceptionutil.hxx>
#include <erebus-clt/erebus-clt.hxx>

#include <unordered_map>

namespace Erp
{

namespace Private
{

ProcessInformation::ProcessInformation(Er::PropertyBag&& bag)
    : properties(std::move(bag))
{
    // find PID (must always be present)
    this->pid = Er::getPropertyOr<Er::ProcessProps::Pid>(properties, Er::ProcessProps::Pid::ValueType(-1));
    if (this->pid == Er::ProcessProps::Pid::ValueType(-1))
        throw Er::Exception(ER_HERE(), "No PID in process properties");

    if (Er::propertyPresent<Er::ProcessProps::IsDeleted>(properties))
    {
        this->deleted = 1;
        return;
    }

    // find 'Valid' property
    this->valid = Er::getPropertyOr<Er::ProcessProps::Valid>(properties, false);
    if (!this->valid)
    {
        // maybe we've got an error message
        auto msg = Er::getProperty<Er::ProcessProps::Error>(properties);
        if (msg)
            this->error = Erc::fromUtf8(*msg);

        return;
    }

    // cache certain props
    for (auto it = properties.begin(); it != properties.end(); ++it)
    {
        switch (it->second.id)
        {
        case Er::ProcessProps::IsNew::Id::value:
            this->added = std::get<bool>(it->second.value);
            break;

        case Er::ProcessProps::PPid::Id::value:
            this->ppid = std::get<uint64_t>(it->second.value);
            break;

        case Er::ProcessProps::StartTime::Id::value:
        {
            this->startTime = std::get<uint64_t>(it->second.value);
            Er::TimeFormatter<"%H:%M:%S %d %b %y", Er::TimeZone::Utc> fmt;
            std::ostringstream ss;
            fmt(this->startTime, ss);
            this->startTimeUtc = Erc::fromUtf8(ss.str());
            break;
        }

        case Er::ProcessProps::State::Id::value:
            this->processState = Erc::fromUtf8(std::get<std::string>(it->second.value));
            break;

        case Er::ProcessProps::Comm::Id::value:
            this->comm = Erc::fromUtf8(std::get<std::string>(it->second.value));
            break;

        case Er::ProcessProps::UTime::Id::value:
            this->uTime = std::get<double>(it->second.value);
            break;

        case Er::ProcessProps::STime::Id::value:
            this->sTime = std::get<double>(it->second.value);
            break;
        }
    }

    if (this->ppid == InvalidKey)
        this->ppid = this->pid;
}

void ProcessInformation::updateFromDiff(const ProcessInformation& diff)
{
    this->uTimePrev = this->uTime;
    this->sTimePrev = this->sTime;
    this->uTime = 0.0;
    this->sTime = 0.0;
    this->uTimeDiff = std::nullopt;
    this->sTimeDiff = std::nullopt;

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

        case Er::ProcessProps::UTime::Id::value:
            this->uTime = diff.uTime;
            if (this->uTimePrev > 0.0)
                this->uTimeDiff = this->uTime - this->uTimePrev;
            break;

        case Er::ProcessProps::STime::Id::value:
            this->sTime = diff.sTime;
            if (this->sTimePrev > 0.0)
                this->sTimeDiff = this->sTime - this->sTimePrev;
            break;
        }
    }

    // show process as 'running' if it has... well... run for a while since the last cycle
    if (this->processState == QLatin1String("S"))
    {
        if ((this->uTimeDiff > 0) || (this->sTimeDiff > 0))
        {
            this->processState = QLatin1String("R");
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
            ErLogInstance("ProcessListImpl"),
            [this]()
            {
                m_client->endSession(Er::ProcessRequests::ListProcessesDiff, m_sessionId);
            }
        );
    }

    explicit ProcessListImpl(std::shared_ptr<void> channel, Er::Log::ILog* log)
        : m_client(Er::Client::createClient(channel, log))
        , m_log(log)
        , m_mutexPool(MutexPoolSize)
        , m_sessionId(m_client->beginSession(Er::ProcessRequests::ListProcessesDiff))
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

    PosixResult kill(uint64_t pid, std::string_view signame) override
    {
        return Er::protectedCall<PosixResult>(
            m_log,
            ErLogInstance("ProcessListImpl"),
            [this, pid, signame]()
            {
                Er::PropertyBag request;
                Er::addProperty<Er::ProcessesGlobal::Pid>(request, pid);
                Er::addProperty<Er::ProcessesGlobal::Signal>(request, std::string(signame));

                auto response = m_client->request(Er::ProcessRequests::KillProcess, request);

                auto code = Er::getProperty<Er::ProcessesGlobal::PosixResult>(response);
                auto message = Er::getProperty<Er::ProcessesGlobal::ErrorText>(response);

                return PosixResult(code ? *code : -1, message ? std::move(*message) : "");
            }
        );
    }

private:
    using ItemContainer = std::unordered_map<typename Item::Key, std::shared_ptr<Item>>;
    
    std::shared_ptr<Item> makeProcessItem(Er::PropertyBag&& bag, Item::TimePoint now, bool firstRun) noexcept
    {
        return Er::protectedCall<std::shared_ptr<Item>>(
            m_log,
            ErLogInstance("ProcessListImpl"),
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

    void parseGlobals(const Er::PropertyBag& bag, Changeset* diff)
    {
        diff->totalProcesses = Er::getPropertyOr<Er::ProcessesGlobal::ProcessCount>(bag, std::size_t(0));

        m_realTimePrev = m_realTime;
        m_realTime = Er::getPropertyOr<Er::ProcessesGlobal::RealTime>(bag, 0.0);

        m_cpuTimePrev = m_cpuTime;
        m_cpuTime = Er::getPropertyOr<Er::ProcessesGlobal::TotalTime>(bag, 0.0);

        diff->realTime = Er::saturatingSub(m_realTime, m_realTimePrev);
        diff->cpuTime = Er::saturatingSub(m_cpuTime, m_cpuTimePrev);
    }

    void enumerateProcesses(bool firstRun, Item::TimePoint now, Er::ProcessProps::PropMask required, std::chrono::milliseconds trackThreshold, Changeset* diff) noexcept
    {
        Er::protectedCall<void>(
            m_log,
            ErLogInstance("ProcessListImpl"),
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
        Er::addProperty<Er::ProcessProps::RequiredFields>(req, required.pack<uint64_t>());

        auto list = m_client->requestStream(Er::ProcessRequests::ListProcessesDiff, req, m_sessionId);
        for (auto& item : list)
        {
            if (Er::propertyPresent<Er::ProcessesGlobal::Global>(item))
            {
                // this is a global state record
                parseGlobals(item, diff);
                continue;
            }

            auto parsedProcess = makeProcessItem(std::move(item), now, firstRun);
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
                }
                else
                {
                    ErLogWarning(m_log, ErLogNowhere(), "Unknown exited process %zu", parsedProcess->pid);
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
                }
                else
                {
                    // this is the first run; all processes are just added w/out marking as 'new'
                }

                continue;
            }
            
            // this is an existing process and we've just got a few fields updated
            assert(!firstRun);
            Er::ObjectLock<Item> locked(existing->second.get());
            locked->updateFromDiff(*parsedProcess.get());

            diff->modified.insert(existing->second);
        }
    }

    void trackNewOrDeletedProcesses(Item::TimePoint now, std::chrono::milliseconds trackThreshold, Changeset* diff)
    {
        for (auto it = m_tracked.begin(); it != m_tracked.end();)
        {
            auto ref = it->second;
            Er::ObjectLock<Item> item(ref.get());

            if (item->maybeUntrackDeleted(now, trackThreshold))
            {
                // item has been being marked 'deleted' for quite a long; time to purge it
                auto next = std::next(it);
                
                diff->purged.insert(ref);

                m_tracked.erase(it);
                it = next;
            }
            else if (item->maybeUntrackNew(now, trackThreshold))
            {
                // item has been being marked 'new' for quite a long
                auto next = std::next(it);

                diff->untracked.insert(it->second);

                m_tracked.erase(it);
                it = next;
            }
            else
            {
                ++it;
            }
        }
    }


    std::shared_ptr<Er::Client::IClient> m_client;
    Er::Log::ILog* m_log;
    static constexpr size_t MutexPoolSize = 5;
    Er::MutexPool<std::recursive_mutex> m_mutexPool;
    Er::Client::IClient::SessionId m_sessionId;
    ItemContainer m_collection;
    ItemContainer m_tracked; // processes being temporarily tracked as 'recently exited' or 'recently started'
    double m_realTime = 0;
    double m_realTimePrev = 0;
    double m_cpuTime = 0;
    double m_cpuTimePrev = 0;
};

} // namespace {}


std::unique_ptr<IProcessList> createProcessList(std::shared_ptr<void> channel, Er::Log::ILog* log)
{
    return std::make_unique<ProcessListImpl>(channel, log);
}

} // namespace Private {}

} // namespace Erp {}
