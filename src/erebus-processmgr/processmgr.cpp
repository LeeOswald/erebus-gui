#include "columnsdlg.hpp"
#include "processcolumns.hpp"
#include "processmgr.hpp"
#include "processtab.hpp"
#include "settings.hpp"

#include <erebus-processmgr/processprops.hxx>

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
        Er::ProcessProps::Private::unregisterAll();
    }

    explicit ProcessMgrPlugin(const Erc::PluginParams& params)
        : QObject()
        , m_params(params)
        , m_refreshInterval(Erc::Option<unsigned>::get(params.settings, Erp::Settings::refreshRate, Erp::Settings::RefreshRateDefault))
        , m_menuProcess(params.mainMenu->addMenu(tr("Process")))
        , m_actionRefreshInterval(new QAction(tr("Refresh Interval"), this))
        , m_actionSelectColumns(new QAction(tr("Select Columns..."), this))
        , m_refreshIntervalActionGroup(new QActionGroup(this))
    {
        if (m_refreshInterval < 500)
            m_refreshInterval = 500;

        m_refreshIntervalActionGroup->setExclusive(true);
        connect(m_refreshIntervalActionGroup, &QActionGroup::triggered, this, &ProcessMgrPlugin::setRefreshInterval);

        auto menuRefreshInterval = new QMenu();
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

        m_menuProcess->addAction(m_actionSelectColumns);
        m_menuProcess->addAction(m_actionRefreshInterval);

        QObject::connect(m_actionSelectColumns, &QAction::triggered, this, &ProcessMgrPlugin::selectColumns);

        Er::ProcessProps::Private::registerAll();
    }

    void addConnection(Er::Client::IClient* client, const std::string& endpoint) override
    {
        auto it = m_tabs.find(client);
        if (it != m_tabs.end())
        {
            m_params.log->write(Er::Log::Level::Error, LogInstance("ProcessMgrPlugin"), "Connection %p to %s already has a tab", client, endpoint.c_str());
            return;
        }

        m_tabs.insert({ client, std::make_unique<Erp::Private::ProcessTab>(m_params, client, endpoint) });
    }

    void removeConnection(Er::Client::IClient* client) override
    {
        auto it = m_tabs.find(client);
        if (it == m_tabs.end())
        {
            m_params.log->write(Er::Log::Level::Error, LogInstance("ProcessMgrPlugin"), "No tab found for connection %p", client);
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

        auto columns = Erp::Private::loadProcessColumns(m_params.settings);

        Erp::Private::ColumnsDlg dlg(columns, std::begin(Erp::Private::ProcessColumnDefs), std::end(Erp::Private::ProcessColumnDefs), m_params.tabWidget);
        if (dlg.exec() != QDialog::Accepted)
            return;

        columns = dlg.columns();

        // force min widths
        for (auto& c: columns)
        {
            if (c.id == Er::ProcessProps::PropIndices::Comm)
            {
                if (c.width < Erp::Settings::MinLabelColumnWidth)
                    c.width = Erp::Settings::MinLabelColumnWidth;
            }
            else
            {
                if (c.width < Erp::Settings::MinColumnWidth)
                    c.width = Erp::Settings::MinColumnWidth;
            }
        }

        Erp::Private::saveProcessColumns(m_params.settings, columns);


        for (auto& tab: m_tabs)
        {
            tab.second->reloadColumns();
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

        Erc::Option<unsigned>::set(m_params.settings, Erp::Settings::refreshRate, m_refreshInterval);
        
        for (auto& tab : m_tabs)
        {
            tab.second->setRefreshInterval(m_refreshInterval);
        }
    }

private:
    Erc::PluginParams m_params;
    unsigned m_refreshInterval;
    QMenu* m_menuProcess;
    QAction* m_actionSelectColumns;
    QAction* m_actionRefreshInterval;
    QActionGroup* m_refreshIntervalActionGroup;
    QAction* m_refresh05Action;
    QAction* m_refresh1Action;
    QAction* m_refresh2Action;
    QAction* m_refresh5Action;
    QAction* m_refresh10Action;
    std::unordered_map<Er::Client::IClient*, std::unique_ptr<Erp::Private::ProcessTab>> m_tabs;
};

} // namespace {}


extern "C"
{

EREBUSPROCMGR_EXPORT Erc::IPlugin* createUiPlugin(const Erc::PluginParams& params)
{
    return new ProcessMgrPlugin(params);
}

EREBUSPROCMGR_EXPORT void disposeUiPlugin(Erc::IPlugin* plugin)
{
    assert(plugin);
    if (!plugin)
        return;

    auto realPlugin = dynamic_cast<ProcessMgrPlugin*>(plugin);
    assert(realPlugin);

    delete realPlugin;
}



} // extern "C" {}
