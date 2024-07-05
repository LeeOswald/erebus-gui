#pragma once

#include "processlist.hpp"

#include <QObject>


namespace Erp
{

namespace Private
{

using ProcessChangesetPtr = std::shared_ptr<Erp::Private::IProcessList::Changeset>;


class ProcessListWorker final
    : public QObject
{
    Q_OBJECT

public:
    ~ProcessListWorker();
    explicit ProcessListWorker(std::shared_ptr<void> channel, Er::Log::ILog* log, QObject* parent);

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
