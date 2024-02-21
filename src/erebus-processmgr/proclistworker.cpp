#include "proclistworker.hpp"


namespace Erp
{

namespace Private
{

namespace
{

bool registerMetatypes()
{
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

void ProcessListWorker::refresh(int threshold)
{
    Erc::Ui::protectedCall<void>(
        m_log,
        [this, threshold]()
        {
            auto changeset = m_processList->collect(std::chrono::milliseconds(threshold));

            emit dataReady(changeset);
        }
    );
}


} // namespace Private {}

} // namespace Erp {}
