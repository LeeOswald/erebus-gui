#include "proclistworker.hpp"


namespace Erp
{

namespace Private
{


ProcessListWorker::~ProcessListWorker()
{
}

ProcessListWorker::ProcessListWorker(Er::Client::IClient* client, Er::Log::ILog* log, QObject* parent)
    : QObject(parent)
    , m_log(log)
    , m_processList(createProcessList(client, log))
{
}

void ProcessListWorker::refresh(Er::ProcessProps::PropMask required, int trackDuration, bool manual)
{
    Er::protectedCall<void>(
        m_log,
        ErLogInstance("ProcessWorker"),
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
        ErLogInstance("ProcessWorker"),
        [this, pid, signame]()
        {
            auto result = m_processList->kill(pid, std::string_view(signame.data(), signame.length()));

            emit posixResult(result);
        }
    );
}


} // namespace Private {}

} // namespace Erp {}
