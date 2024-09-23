#include "itemmenu.hpp"
#include "proctreemodel.hpp"

namespace Erp::ProcessMgr
{

ItemMenu::ItemMenu(QTreeView* view)
    : m_view(view)
    , m_menu(new QMenu(view))
    , m_actionKill(new QAction(QCoreApplication::translate("ProcessTab", "Kill", nullptr), view))
    , m_menuKill(new QMenu(view))
    , m_actionGroupKill(new QActionGroup(view))
    , m_actionSIGKILL(new QAction(QCoreApplication::translate("ProcessTab", "SIGKILL", nullptr), m_actionGroupKill))
    , m_actionSIGINT(new QAction(QCoreApplication::translate("ProcessTab", "SIGINT", nullptr), m_actionGroupKill))
    , m_actionSIGTERM(new QAction(QCoreApplication::translate("ProcessTab", "SIGTERM", nullptr), m_actionGroupKill))
    , m_actionSIGQUIT(new QAction(QCoreApplication::translate("ProcessTab", "SIGQUIT", nullptr), m_actionGroupKill))
    , m_actionSIGABRT(new QAction(QCoreApplication::translate("ProcessTab", "SIGABRT", nullptr), m_actionGroupKill))
    , m_actionSIGCONT(new QAction(QCoreApplication::translate("ProcessTab", "SIGCONT", nullptr), m_actionGroupKill))
    , m_actionSIGSTOP(new QAction(QCoreApplication::translate("ProcessTab", "SIGSTOP", nullptr), m_actionGroupKill))
    , m_actionSIGTSTP(new QAction(QCoreApplication::translate("ProcessTab", "SIGTSTP", nullptr), m_actionGroupKill))
    , m_actionSIGHUP(new QAction(QCoreApplication::translate("ProcessTab", "SIGHUP", nullptr), m_actionGroupKill))
    , m_actionSIGUSR1(new QAction(QCoreApplication::translate("ProcessTab", "SIGUSR1", nullptr), m_actionGroupKill))
    , m_actionSIGUSR2(new QAction(QCoreApplication::translate("ProcessTab", "SIGUSR2", nullptr), m_actionGroupKill))
    , m_actionSIGSEGV(new QAction(QCoreApplication::translate("ProcessTab", "SIGSEGV", nullptr), m_actionGroupKill))
    , m_actionProcessProps(new QAction(QCoreApplication::translate("ProcessTab", "Properties...", nullptr), view))
{
    m_menu->addAction(m_actionKill);
    m_menu->addSeparator();
    m_menu->addAction(m_actionProcessProps);
    
    m_menuKill->addAction(m_actionSIGKILL);
    m_menuKill->addAction(m_actionSIGINT);
    m_menuKill->addAction(m_actionSIGTERM);
    m_menuKill->addAction(m_actionSIGQUIT);
    m_menuKill->addAction(m_actionSIGABRT);
    m_menuKill->addAction(m_actionSIGCONT);
    m_menuKill->addAction(m_actionSIGSTOP);
    m_menuKill->addAction(m_actionSIGTSTP);
    m_menuKill->addAction(m_actionSIGHUP);
    m_menuKill->addAction(m_actionSIGUSR1);
    m_menuKill->addAction(m_actionSIGUSR2);
    m_menuKill->addAction(m_actionSIGSEGV);
    m_actionKill->setMenu(m_menuKill);

    connect(m_actionGroupKill, SIGNAL(triggered(QAction*)), this, SLOT(onKill(QAction*)));
    connect(m_actionProcessProps, SIGNAL(triggered(QAction*)), this, SLOT(onProcessProps(QAction*)));

    view->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(view, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(onCustomContextMenu(const QPoint&)));
}

void ItemMenu::onCustomContextMenu(const QPoint& point)
{
    QModelIndex index = m_view->indexAt(point);
    if (index.isValid()) 
    {
        m_selectedPid = m_model->pid(index);
        if (m_selectedPid != uint64_t(-1))
            m_menu->exec(m_view->viewport()->mapToGlobal(point));
    }
}

void ItemMenu::onKill(QAction* action)
{
    if (action == m_actionSIGKILL)
        emit kill(m_selectedPid, QLatin1String("SIGKILL"));
    else if (action == m_actionSIGINT)
        emit kill(m_selectedPid, QLatin1String("SIGINT"));
    else if (action == m_actionSIGTERM)
        emit kill(m_selectedPid, QLatin1String("SIGTERM"));
    else if (action == m_actionSIGQUIT)
        emit kill(m_selectedPid, QLatin1String("SIGQUIT"));
    else if (action == m_actionSIGABRT)
        emit kill(m_selectedPid, QLatin1String("SIGABRT"));
    else if (action == m_actionSIGCONT)
        emit kill(m_selectedPid, QLatin1String("SIGCONT"));
    else if (action == m_actionSIGSTOP)
        emit kill(m_selectedPid, QLatin1String("SIGSTOP"));
    else if (action == m_actionSIGTSTP)
        emit kill(m_selectedPid, QLatin1String("SIGTSTP"));
    else if (action == m_actionSIGHUP)
        emit kill(m_selectedPid, QLatin1String("SIGHUP"));
    else if (action == m_actionSIGUSR1)
        emit kill(m_selectedPid, QLatin1String("SIGUSR1"));
    else if (action == m_actionSIGUSR2)
        emit kill(m_selectedPid, QLatin1String("SIGUSR2"));
    else if (action == m_actionSIGSEGV)
        emit kill(m_selectedPid, QLatin1String("SIGSEGV"));

    m_selectedPid = uint64_t(-1);
}

void ItemMenu::onProcessProps(QAction* action)
{
    emit processProps(m_selectedPid);
}

} // namespace Erp::ProcessMgr {}
