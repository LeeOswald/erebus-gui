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


ProcessTabWorker::~ProcessTabWorker()
{
}

ProcessTabWorker::ProcessTabWorker(Er::Client::IClient* client, Er::Log::ILog* log, QObject* parent)
    : QObject(parent)
    , m_log(log)
    , m_processList(createProcessList(client, log))
{
    static bool s_metatypesRegistered = registerMetatypes();
}

void ProcessTabWorker::refresh(int threshold)
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
