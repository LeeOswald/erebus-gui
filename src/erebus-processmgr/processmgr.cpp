#include "columnsdlg.hpp"
#include "processcolumns.hpp"
#include "processmgr.hpp"
#include "processtab.hpp"
#include "settings.hpp"

#include "client-version.h"

#include <erebus-desktop/erebus-desktop.hxx>
#include <erebus-processmgr/erebus-processmgr.hxx>

#include <unordered_map>

#include <QAction>
#include <QActionGroup>
#include <QCoreApplication>
#include <QMenu>

namespace
{

class ProcessMgrPlugin final
    : public Erc::IPlugin
    , public QObject
{
public:
    ~ProcessMgrPlugin()
    {
        delete m_menuProcess;
        Er::Desktop::Props::Private::unregisterAll(m_params.log);
        Er::ProcessMgr::Private::unregisterAll(m_params.log);
    }

    void dispose() noexcept override
    {
        delete this;
    }

    explicit ProcessMgrPlugin(const Erc::PluginParams& params)
        : QObject()
        , m_params(params)
        , m_autoRefresh(Erc::Option<bool>::get(params.settings, Erp::ProcessMgr::Settings::autoRefresh, true))
        , m_refreshInterval(Erc::Option<unsigned>::get(params.settings, Erp::ProcessMgr::Settings::refreshRate, Erp::ProcessMgr::Settings::RefreshRateDefault))
        , m_menuProcess(params.mainMenu->addMenu(tr("Process")))
        , m_actionAutoRefresh(new QAction(tr("Refresh automatically"), this))
        , m_actionRefreshInterval(new QAction(tr("Refresh interval"), this))
        , m_actionRefreshNow(new QAction(tr("Refresh now"), this))
        , m_actionSelectColumns(new QAction(tr("Select columns..."), this))
        , m_refreshIntervalActionGroup(new QActionGroup(this))
    {
        if (m_refreshInterval < 500)
            m_refreshInterval = 500;

        m_refreshIntervalActionGroup->setExclusive(true);
        connect(m_refreshIntervalActionGroup, &QActionGroup::triggered, this, &ProcessMgrPlugin::setRefreshInterval);

        auto menuRefreshInterval = new QMenu(m_menuProcess);
        m_actionRefreshInterval->setMenu(menuRefreshInterval);

        m_refresh05Action = new QAction(tr("Fast (0.5s)"), m_refreshIntervalActionGroup);
        m_refresh05Action->setCheckable(true);
        menuRefreshInterval->addAction(m_refresh05Action);
        m_refresh05Action->setChecked(m_refreshInterval == 500);

        m_refresh1Action = new QAction(tr("Normal (1s)"), m_refreshIntervalActionGroup);
        m_refresh1Action->setCheckable(true);
        menuRefreshInterval->addAction(m_refresh1Action);
        m_refresh1Action->setChecked(m_refreshInterval == 1000);

        m_refresh2Action = new QAction(tr("Below Normal (2s)"), m_refreshIntervalActionGroup);
        m_refresh2Action->setCheckable(true);
        menuRefreshInterval->addAction(m_refresh2Action);
        m_refresh2Action->setChecked(m_refreshInterval == 2000);

        m_refresh5Action = new QAction(tr("Slow (5s)"), m_refreshIntervalActionGroup);
        m_refresh5Action->setCheckable(true);
        menuRefreshInterval->addAction(m_refresh5Action);
        m_refresh5Action->setChecked(m_refreshInterval == 5000);

        m_refresh10Action = new QAction(tr("Very Slow (10s)"), m_refreshIntervalActionGroup);
        m_refresh10Action->setCheckable(true);
        menuRefreshInterval->addAction(m_refresh10Action);
        m_refresh10Action->setChecked(m_refreshInterval == 10000);

        m_menuProcess->addAction(m_actionAutoRefresh);
        m_menuProcess->addAction(m_actionRefreshNow);
        m_menuProcess->addAction(m_actionRefreshInterval);
        m_menuProcess->addSeparator();
        m_menuProcess->addAction(m_actionSelectColumns);

        m_actionAutoRefresh->setCheckable(true);
        m_actionAutoRefresh->setChecked(m_autoRefresh);

        QObject::connect(m_actionAutoRefresh, &QAction::triggered, this, &ProcessMgrPlugin::autoRefresh);
        QObject::connect(m_actionRefreshNow, &QAction::triggered, this, &ProcessMgrPlugin::refreshNow);
        QObject::connect(m_actionSelectColumns, &QAction::triggered, this, &ProcessMgrPlugin::selectColumns);

        Er::ProcessMgr::Private::registerAll(m_params.log);
        Er::Desktop::Props::Private::registerAll(m_params.log);
    }

    Info info() const override
    {
        return Info("Process Tree", "Process tree & properties", Info::Version(EREBUS_VERSION_MAJOR, EREBUS_VERSION_MINOR, EREBUS_VERSION_PATCH));
    }

    void addConnection(Er::Client::ChannelPtr channel, const std::string& endpoint) override
    {
        auto it = m_tabs.find(channel.get());
        if (it != m_tabs.end())
        {
            m_params.log->writef(Er::Log::Level::Error, "Connection %s already has a tab", endpoint.c_str());
            return;
        }

        m_tabs.insert({ channel.get(), std::make_unique<Erp::ProcessMgr::ProcessTab>(m_params, channel, endpoint) });
    }

    void removeConnection(Er::Client::ChannelPtr channel) noexcept override
    {
        auto it = m_tabs.find(channel.get());
        if (it == m_tabs.end())
        {
            m_params.log->writef(Er::Log::Level::Error, "No tab found for connection %p", channel.get());
            return;
        }

        m_tabs.erase(it);
    }

private slots:
    void selectColumns()
    {
        for (auto& tab: m_tabs)
        {
            tab.second->saveColumns();
        }

        auto columns = Erp::ProcessMgr::loadProcessColumns(m_params.settings);

        Erp::ProcessMgr::ColumnsDlg dlg(columns, Erp::ProcessMgr::ProcessColumnDefs, m_params.tabWidget);
        if (dlg.exec() != QDialog::Accepted)
            return;

        columns = dlg.columns();
        Erp::ProcessMgr::saveProcessColumns(m_params.settings, columns);

        for (auto& tab: m_tabs)
        {
            tab.second->reloadColumns();
        }
    }

    void autoRefresh()
    {
        if (m_autoRefresh)
        {
            m_autoRefresh = false;
            m_actionAutoRefresh->setChecked(false);
        }
        else
        {
            m_autoRefresh = true;
            m_actionAutoRefresh->setChecked(true);
        }

        for (auto& tab : m_tabs)
        {
            tab.second->setAutoRefresh(m_autoRefresh);
        }
    }

    void refreshNow()
    {
        for (auto& tab : m_tabs)
        {
            tab.second->refresh(true);
        }
    }

    void setRefreshInterval(QAction* action)
    {
        if (action == m_refresh05Action) m_refreshInterval = 500;
        else if (action == m_refresh1Action) m_refreshInterval = 1000;
        else if (action == m_refresh2Action) m_refreshInterval = 2000;
        else if (action == m_refresh5Action) m_refreshInterval = 5000;
        else if (action == m_refresh10Action) m_refreshInterval = 10000;

        action->setChecked(true);

        Erc::Option<unsigned>::set(m_params.settings, Erp::ProcessMgr::Settings::refreshRate, m_refreshInterval);
        
        for (auto& tab : m_tabs)
        {
            tab.second->setRefreshInterval(m_refreshInterval);
        }
    }

private:
    Erc::PluginParams m_params;
    bool m_autoRefresh;
    unsigned m_refreshInterval;
    QMenu* m_menuProcess;
    QAction* m_actionSelectColumns;
    QAction* m_actionAutoRefresh;
    QAction* m_actionRefreshNow;
    QAction* m_actionRefreshInterval;
    QActionGroup* m_refreshIntervalActionGroup;
    QAction* m_refresh05Action;
    QAction* m_refresh1Action;
    QAction* m_refresh2Action;
    QAction* m_refresh5Action;
    QAction* m_refresh10Action;
    std::unordered_map<void*, std::unique_ptr<Erp::ProcessMgr::ProcessTab>> m_tabs; // connection -> tab
};

} // namespace {}


extern "C"
{

EREBUSPROCMGR_EXPORT Erc::IPlugin* createUiPlugin(const Erc::PluginParams& params)
{
    return new ProcessMgrPlugin(params);
}

} // extern "C" {}
