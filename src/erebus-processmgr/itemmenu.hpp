#pragma once

#include <QAction>
#include <QActionGroup>
#include <QCoreApplication>
#include <QMenu>
#include <QTreeView>

namespace Erp::ProcessMgr
{

class ProcessTreeModel;


class ItemMenu final
    : public QObject
{
    Q_OBJECT

public:
    explicit ItemMenu(QTreeView* view);

    void setModel(ProcessTreeModel* model) noexcept
    {
        m_model = model;
    }

signals:
    void kill(quint64 pid, QLatin1String signal);
    void processProps(quint64 pid);

private slots:
    void onCustomContextMenu(const QPoint& point);
    void onKill(QAction* action);
    void onProcessProps(QAction* action);

private:
    QTreeView* m_view;
    QMenu* m_menu;
    QAction* m_actionKill;
    QMenu* m_menuKill;
    QActionGroup* m_actionGroupKill;
    QAction* m_actionSIGKILL;
    QAction* m_actionSIGINT;
    QAction* m_actionSIGTERM;
    QAction* m_actionSIGQUIT;
    QAction* m_actionSIGABRT;
    QAction* m_actionSIGCONT;
    QAction* m_actionSIGSTOP;
    QAction* m_actionSIGTSTP;
    QAction* m_actionSIGHUP;
    QAction* m_actionSIGUSR1;
    QAction* m_actionSIGUSR2;
    QAction* m_actionSIGSEGV;
    QAction* m_actionProcessProps;
    ProcessTreeModel* m_model = nullptr;
    uint64_t m_selectedPid = uint64_t(-1);
};

} // namespace Erp::ProcessMgr {}
