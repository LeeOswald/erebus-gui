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

        case Er::ProcessProps::UTime::Id::value:
            this->uTime = std::any_cast<double>(it->second.value);
            break;

        case Er::ProcessProps::STime::Id::value:
            this->sTime = std::any_cast<double>(it->second.value);
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

        // this should go after enumerateProcesses() so that the server can just send 
        // what it has collected during enumerateProcesses()
        readProcessesGlobal(diff.get());

        return diff;
    }

    PosixResult kill(uint64_t pid, std::string_view signame) override
    {
        return Er::protectedCall<PosixResult>(
            m_log,
            LogInstance("ProcessListImpl"),
            [this, pid, signame]()
            {
                Er::PropertyBag request;
                request.insert({ Er::ProcessesGlobal::Pid::Id::value, Er::Property(Er::ProcessesGlobal::Pid::Id::value, pid) });
                request.insert({ Er::ProcessesGlobal::Signal::Id::value, Er::Property(Er::ProcessesGlobal::Signal::Id::value, std::string(signame)) });

                auto response = m_client->request(Er::ProcessRequests::KillProcess, request);

                auto code = Er::getProperty<Er::ProcessesGlobal::PosixResult::ValueType>(response, Er::ProcessesGlobal::PosixResult::Id::value);
                auto message = Er::getProperty<Er::ProcessesGlobal::ErrorText::ValueType>(response, Er::ProcessesGlobal::ErrorText::Id::value);

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

    void readProcessesGlobal(Changeset* diff) noexcept
    {
        Er::protectedCall<void>(
            m_log,
            LogInstance("ProcessListImpl"),
            [this, diff]()
            {
                return readProcessesGlobalImpl(diff);
            }
        );
    }

    void readProcessesGlobalImpl(Changeset* diff)
    {
        Er::ProcessesGlobal::PropMask required
        { 
            Er::ProcessesGlobal::PropIndices::ProcessCount,
            Er::ProcessesGlobal::PropIndices::RTime,
            Er::ProcessesGlobal::PropIndices::STime,
            Er::ProcessesGlobal::PropIndices::UTime,
        };

        Er::PropertyBag req;
        req.insert({ Er::ProcessesGlobal::RequiredFields::Id::value, Er::Property(Er::ProcessesGlobal::RequiredFields::Id::value, required.pack<uint64_t>()) });
        req.insert({ Er::ProcessesGlobal::Lazy::Id::value, Er::Property(Er::ProcessesGlobal::Lazy::Id::value, true) });

        auto response = m_client->request(Er::ProcessRequests::ProcessesGlobal, req);
        
        diff->totalProcesses = Er::getProperty(response, Er::ProcessesGlobal::ProcessCount::Id::value, std::size_t(0));

        m_rTimePrev = m_rTime;
        m_rTime = Er::getProperty(response, Er::ProcessesGlobal::RTime::Id::value, 0.0);

        m_sTimePrev = m_sTime;
        m_sTime = Er::getProperty(response, Er::ProcessesGlobal::STime::Id::value, 0.0);

        m_uTimePrev = m_uTime;
        m_uTime = Er::getProperty(response, Er::ProcessesGlobal::UTime::Id::value, 0.0);

        diff->rTime = m_rTime - m_rTimePrev;
        diff->sTime = m_sTime - m_sTimePrev;
        diff->uTime = m_uTime - m_uTimePrev;
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
                }
                else
                {
                    // this is the first run; all processes are just added w/out marking as 'new'
                }

                continue;
            }
            
            // this is an existing process and we've just got a few fields updated
            assert(!firstRun);
            Er::ObjectLock<Item> item(existing->second.get());
            item->updateFromDiff(*parsedProcess.get());

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


    Er::Client::IClient* m_client;
    Er::Log::ILog* m_log;
    static constexpr size_t MutexPoolSize = 5;
    Er::MutexPool<std::recursive_mutex> m_mutexPool;
    Er::Client::IClient::SessionId m_sessionId;
    ItemContainer m_collection;
    ItemContainer m_tracked; // processes being temporarily tracked as 'recently exited' or 'recently started'
    double m_rTime = 0;
    double m_rTimePrev = 0;
    double m_uTime = 0;
    double m_uTimePrev = 0;
    double m_sTime = 0;
    double m_sTimePrev = 0;
};

} // namespace {}


std::unique_ptr<IProcessList> createProcessList(Er::Client::IClient* client, Er::Log::ILog* log)
{
    return std::make_unique<ProcessListImpl>(client, log);
}

} // namespace Private {}

} // namespace Erp {}
