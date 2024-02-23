#include "columnsdlg.hpp"
#include "processcolumns.hpp"
#include "processmgr.hpp"
#include "processtab.hpp"
#include "settings.hpp"

#include <erebus-processmgr/processprops.hxx>

#include <unordered_map>

#include <QAction>
#include <QMenu>
#include <QObject>

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
        , m_menuProcess(params.mainMenu->addMenu(tr("Process")))
        , m_actionSelectColumns(new QAction(tr("Select Columns..."), this))
    {
        m_menuProcess->addAction(m_actionSelectColumns);

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

private:
    Erc::PluginParams m_params;
    QMenu* m_menuProcess;
    QAction* m_actionSelectColumns;
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
