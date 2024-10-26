#include "iconcache.hpp"
#include "processlist.hpp"

#include <erebus/mutexpool.hxx>
#include <erebus/util/exceptionutil.hxx>


namespace Erp::ProcessMgr
{

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
            [this]()
            {
                m_client->endSession(Er::ProcessMgr::Requests::ListProcessesDiff, m_sessionId);
            }
        );
    }

    explicit ProcessListImpl(Er::Client::ChannelPtr channel, Er::Log::ILog* log)
        : m_client(Er::Client::createClient(channel, log))
        , m_log(log)
        , m_mutexPool(MutexPoolSize)
        , m_sessionId(m_client->beginSession(Er::ProcessMgr::Requests::ListProcessesDiff))
        , m_iconCache(channel, log)
    {
    }

    std::shared_ptr<Changeset> collect(Er::ProcessMgr::ProcessProps::PropMask required, std::chrono::milliseconds trackThreshold) override
    {
        auto firstRun = m_collection.empty();
        auto now = Item::now();

        auto diff = std::make_shared<Changeset>(firstRun);
        
        enumerateProcesses(firstRun, now, required, trackThreshold, diff.get());
        trackNewOrDeletedProcesses(now, trackThreshold, diff.get());
        updateIcons(diff.get());

        return diff;
    }

private:
    using ItemContainer = std::unordered_map<typename Item::Key, std::shared_ptr<Item>>;
    
    std::shared_ptr<Item> makeProcessItem(Er::PropertyBag&& bag, Item::TimePoint now, bool firstRun) noexcept
    {
        return Er::protectedCall<std::shared_ptr<Item>>(
            m_log,
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
        diff->totalProcesses = Er::getPropertyValueOr<Er::ProcessMgr::GlobalProps::ProcessCount>(bag, std::size_t(0));

        m_realTimePrev = m_realTime;
        m_realTime = Er::getPropertyValueOr<Er::ProcessMgr::GlobalProps::RealTime>(bag, 0.0);

        m_cpuTimePrev = m_cpuTime;
        m_cpuTime = Er::getPropertyValueOr<Er::ProcessMgr::GlobalProps::TotalTime>(bag, 0.0);

        diff->realTime = Er::saturatingSub(m_realTime, m_realTimePrev);
        diff->cpuTime = Er::saturatingSub(m_cpuTime, m_cpuTimePrev);
    }

    void enumerateProcesses(bool firstRun, Item::TimePoint now, Er::ProcessMgr::ProcessProps::PropMask required, std::chrono::milliseconds trackThreshold, Changeset* diff) noexcept
    {
        Er::protectedCall<void>(
            m_log,
            [this](bool firstRun, Item::TimePoint now, Er::ProcessMgr::ProcessProps::PropMask required, std::chrono::milliseconds trackThreshold, Changeset* diff)
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

    void enumerateProcessesImpl(bool firstRun, Item::TimePoint now, Er::ProcessMgr::ProcessProps::PropMask required, std::chrono::milliseconds trackThreshold, Changeset* diff)
    {
        Er::PropertyBag req;
        Er::addProperty<Er::ProcessMgr::ProcessProps::RequiredFields>(req, required.pack<uint64_t>());

        auto list = m_client->requestStream(Er::ProcessMgr::Requests::ListProcessesDiff, req, m_sessionId);
        for (auto& item : list)
        {
            if (Er::propertyPresent<Er::ProcessMgr::GlobalProps::Global>(item))
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
                    Q_ASSERT(!item->deleted);
                    item->markDeleted(now);
                    m_tracked.insert({ item->pid, existing->second });
                    diff->tracked.insert(existing->second);
                }
                else
                {
                    Er::Log::warning(m_log, "Unknown exited process {}", parsedProcess->pid);
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
                    Q_ASSERT(parsedProcess->state() == Item::State::New);
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
            Q_ASSERT(!firstRun);
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
                // item has been being marked 'deleted' for quite a long time to purge it
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

    void updateIcons(Changeset* diff)
    {
        for (auto& process: m_collection)
        {
            bool needIcon = false;
            bool iconed = false;
            {
                Er::ObjectLock<Item> locked(process.second.get());

                if (locked->icon.state == ProcessInformation::IconData::State::Undefined)
                {
                    needIcon = true;
                }
                else if (locked->icon.state == ProcessInformation::IconData::State::Valid)
                {
                    if (locked->icon.timestamp > locked->iconTimestamp)
                    {
                        iconed = true;
                        locked->iconTimestamp = locked->icon.timestamp;
                    }
                }
            }

            if (needIcon)
            {
                m_iconCache.requestIcon(process.second);
            }

            if (iconed)
            {
                diff->iconed.insert(process.second);
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
    IconCache m_iconCache;
};

} // namespace {}


std::unique_ptr<IProcessList> createProcessList(Er::Client::ChannelPtr channel, Er::Log::ILog* log)
{
    return std::make_unique<ProcessListImpl>(channel, log);
}

} // namespace Erp::ProcessMgr {}
