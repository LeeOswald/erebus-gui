#include "proclistworker.hpp"

#include <erebus/util/exceptionutil.hxx>


namespace Erp
{

namespace ProcessMgr
{


ProcessListWorker::~ProcessListWorker()
{
}

ProcessListWorker::ProcessListWorker(Er::Client::ChannelPtr channel, Er::Log::ILog* log, QObject* parent)
    : QObject(parent)
    , m_log(log)
    , m_processList(createProcessList(channel, log))
{
}

void ProcessListWorker::shutdown()
{
    m_processList.reset();
}

void ProcessListWorker::refresh(Er::ProcessMgr::ProcessProps::PropMask required, int trackDuration, bool manual)
{
    Er::protectedCall<void>(
        m_log,
        [this, required, trackDuration, manual]()
        {
            auto changeset = m_processList->collect(required, std::chrono::milliseconds(trackDuration));

            emit dataReady(changeset, manual);
        }
    );
}

void ProcessListWorker::kill(quint64 pid, QLatin1String signame)
{
    Er::protectedCall<void>(
        m_log,
        [this, pid, signame]()
        {
            auto result = m_processList->kill(pid, std::string_view(signame.data(), signame.length()));

            emit posixResult(result);
        }
    );
}


} // namespace ProcessMgr {}

} // namespace Erp {}
