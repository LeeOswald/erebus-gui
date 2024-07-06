#pragma once

#include "processlist.hpp"

#include <QObject>
#include <QPointer>
#include <QThread>


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

    void shutdown();

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


struct ProcessListThread final
    : public Er::NonCopyable
{
    QPointer<QThread> thread;
    QPointer<ProcessListWorker> worker;

    void destroy()
    {
        if (worker)
        {
            worker->shutdown();
            worker.clear();
        }

        if (thread)
        {
            thread->quit();
            thread->wait();
            thread.clear();
        }
    }

    void make(std::shared_ptr<void> channel, Er::Log::ILog* log)
    {
        thread = new QThread(nullptr);
        worker = new ProcessListWorker(channel, log, nullptr);

        // auto-delete thread
        QObject::connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

        // auto-delete worker
        QObject::connect(thread, SIGNAL(finished()), worker, SLOT(deleteLater()));
        worker->moveToThread(thread);
    }

    void start()
    {
        if (thread)
        {
            thread->start();
            thread->setObjectName("ProcessList");
        }
    }

    void refresh(bool manual, Er::ProcessProps::PropMask required, int trackDuration)
    {
        if (worker)
        {
            QMetaObject::invokeMethod(worker, "refresh", Qt::AutoConnection, Q_ARG(Er::ProcessProps::PropMask, required), Q_ARG(int, trackDuration), Q_ARG(bool, manual));
        }
    }

    void kill(quint64 pid, QLatin1String signal)
    {
        if (worker)
        {
            QMetaObject::invokeMethod(worker, "kill", Qt::AutoConnection, Q_ARG(quint64, pid), Q_ARG(QLatin1String, signal));
        }
    }
};

} // namespace Private {}

} // namespace Erp {}
