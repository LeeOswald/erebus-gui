#pragma once

#include "processlist.hpp"

#include <QObject>

Q_DECLARE_METATYPE(Er::ProcessProps::PropMask);

using ProcessChangesetPtr = std::shared_ptr<Erp::Private::IProcessList::Changeset>;
Q_DECLARE_METATYPE(ProcessChangesetPtr);


namespace Erp
{

namespace Private
{


class ProcessListWorker final
    : public QObject
{
    Q_OBJECT

public:
    ~ProcessListWorker();
    explicit ProcessListWorker(Er::Client::IClient* client, Er::Log::ILog* log, QObject* parent);

public slots:
    void refresh(Er::ProcessProps::PropMask required, int trackDuration, bool manual);
    void kill(quint64 pid, QLatin1String signame);

signals:
    void dataReady(ProcessChangesetPtr, bool);
    void posixResult(Erp::Private::IProcessList::PosixResult);

private:
    Er::Log::ILog* m_log;
    std::unique_ptr<IProcessList> m_processList;
};

} // namespace Private {}

} // namespace Erp {}
