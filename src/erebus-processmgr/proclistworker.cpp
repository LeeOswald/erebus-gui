#include "proclistworker.hpp"


namespace Erp
{

namespace Private
{

namespace
{

bool registerMetatypes()
{
    ::qRegisterMetaType<Er::ProcessProps::PropMask>();
    ::qRegisterMetaType<ProcessChangesetPtr>();

    return true;
}

} // namespace {}


ProcessListWorker::~ProcessListWorker()
{
}

ProcessListWorker::ProcessListWorker(Er::Client::IClient* client, Er::Log::ILog* log, QObject* parent)
    : QObject(parent)
    , m_log(log)
    , m_processList(createProcessList(client, log))
{
    static bool s_metatypesRegistered = registerMetatypes();
}

void ProcessListWorker::refresh(Er::ProcessProps::PropMask required, int threshold)
{
    Er::protectedCall<void>(
        m_log,
        LogInstance("ProcessWorker"),
        [this, required, threshold]()
        {
            auto changeset = m_processList->collect(required, std::chrono::milliseconds(threshold));

            emit dataReady(changeset);
        }
    );
}


} // namespace Private {}

} // namespace Erp {}
