#include "processtab.hpp"

#include <QGridLayout>
#include <QHeaderView>

namespace Erp
{

namespace Private
{


ProcessTab::~ProcessTab()
{
    m_refreshTimer->stop();
    
    saveColumns();

    m_worker.clear();

    if (m_thread)
    {
        m_thread->quit();
        m_thread->wait();
        m_thread.clear();
    }

    auto index = m_params.tabWidget->indexOf(m_widget);
    Q_ASSERT(index >= 0);
    m_params.tabWidget->removeTab(index);

    delete m_treeView;
    delete m_widget;
}

ProcessTab::ProcessTab(const Erc::PluginParams& params, Er::Client::IClient* client, const std::string& endpoint)
    : QObject(params.tabWidget)
    , m_params(params)
    , m_autoRefresh(Erc::Option<bool>::get(params.settings, Erp::Settings::autoRefresh, true))
    , m_refreshRate(Erc::Option<unsigned>::get(params.settings, Erp::Settings::refreshRate, Erp::Settings::RefreshRateDefault))
    , m_trackDuration(Erc::Option<unsigned>::get(params.settings, Erp::Settings::trackDuration, Erp::Settings::TrackDurationDefault))
    , m_refreshTimer(new QTimer(this))
    , m_columns(loadProcessColumns(m_params.settings))
    , m_required(makePropMask(m_columns))
    , m_client(client)
    , m_endpoint(endpoint)
    , m_widget(new QWidget(params.tabWidget))
    , m_treeView(new QTreeView(m_widget))
{
    requireAdditionalProps(m_required);

    auto layout = new QGridLayout(m_widget);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);

    m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_treeView->setProperty("showDropIndicator", QVariant(false));
    m_treeView->setIconSize(QSize(16, 16));
    m_treeView->setSortingEnabled(false);
    m_treeView->header()->setProperty("showSortIndicator", QVariant(false));
    m_treeView->header()->setStretchLastSection(false);
    m_treeView->setUniformRowHeights(true);

    layout->addWidget(m_treeView, 0, 0, 1, 1);

    params.tabWidget->addTab(m_widget, QString());
    params.tabWidget->setTabText(params.tabWidget->indexOf(m_widget), tr("Processes"));

    startWorker();

    if (m_refreshRate < 500)
        m_refreshRate = 500;

    m_refreshTimer->setSingleShot(true);
    connect(m_refreshTimer, &QTimer::timeout, this, [this]() { refresh(false); });
    m_refreshTimer->start(m_refreshRate);
}

void ProcessTab::setAutoRefresh(bool autoRefresh)
{
    auto prev = m_autoRefresh;
    m_autoRefresh = autoRefresh;

    m_refreshTimer->stop();

    if (autoRefresh && !prev)
    {
        m_refreshTimer->start(m_refreshRate);
    }
}

void ProcessTab::setRefreshInterval(unsigned interval)
{
    Q_ASSERT(interval >= 500);
    m_refreshRate = interval;

    LogDebug(m_params.log, LogInstance("ProcessTab"), "Set refresh interval to %d msec", m_refreshRate);
}

void ProcessTab::requireAdditionalProps(Er::ProcessProps::PropMask& required) noexcept
{
    // what we need even if there's no corresponding visible column
    required.set(Er::ProcessProps::PropIndices::CmdLine);
    required.set(Er::ProcessProps::PropIndices::Exe);
    required.set(Er::ProcessProps::PropIndices::Icon);
}

void ProcessTab::saveColumns()
{
    captureColumnWidths();
    Erp::Private::saveProcessColumns(m_params.settings, m_columns);
}

void ProcessTab::reloadColumns()
{
    auto prevColumns = m_columns;
    m_columns = loadProcessColumns(m_params.settings);
    m_columnsChanged = !isProcessColumnsOrderSame(prevColumns, m_columns);
    m_required = makePropMask(m_columns);
    requireAdditionalProps(m_required);
}

void ProcessTab::startWorker()
{
    m_thread = new QThread(nullptr);
    m_worker = new ProcessListWorker(m_client, m_params.log, nullptr);

    // auto-delete thread
    connect(m_thread, SIGNAL(finished()), m_thread, SLOT(deleteLater()));

    // auto-delete worker
    connect(m_thread, SIGNAL(finished()), m_worker, SLOT(deleteLater()));
    m_worker->moveToThread(m_thread);

    // know when worker has data
    connect(m_worker, SIGNAL(dataReady(ProcessChangesetPtr,bool)), this, SLOT(dataReady(ProcessChangesetPtr,bool)));

    m_thread->start();
}

void ProcessTab::refresh(bool manual)
{
    if (m_worker)
    {
        LogDebug(m_params.log, LogInstance("ProcessTab"), "Refreshing...");

        QMetaObject::invokeMethod(m_worker, "refresh", Qt::AutoConnection, Q_ARG(Er::ProcessProps::PropMask, m_required), Q_ARG(int, m_trackDuration), Q_ARG(bool, manual));
    }
}

void ProcessTab::dataReady(ProcessChangesetPtr changeset, bool manual)
{
    Er::protectedCall<void>(
        m_params.log,
        LogInstance("ProcessTab"),
        [this, changeset]()
        {
            if (!m_model)
            {
                m_model = new ProcessTreeModel(m_params.log, changeset, m_columns, this);

                m_treeView->setModel(m_model);
                m_treeView->expandAll();

                restoreColumnWidths();
            }
            else
            {
                if (m_columnsChanged)
                {
                    m_columnsChanged = false;
                    m_model->setColumns(m_columns);

                    restoreColumnWidths();
                }

                // QTreeView does not expand new items automatically; we need to do this explicitly
                auto parentsToExpand = m_model->update(changeset);
                for (auto& index : parentsToExpand)
                {
                    m_treeView->expandRecursively(index);
                }
            }
        }
    );

    if (!manual && m_autoRefresh)
    {
        // schedule the next refresh
        m_refreshTimer->start(m_refreshRate);
    }
}

void ProcessTab::restoreColumnWidths()
{
    int index = 0;
    for (auto& c: m_columns)
    {
        m_treeView->setColumnWidth(index, c.width);
        ++index;
    }
}

void ProcessTab::captureColumnWidths()
{
    int index = 0;
    for (auto& c: m_columns)
    {
        c.width = m_treeView->columnWidth(index);
        ++index;
    }
}

} // namespace Private {}

} // namespace Erp {}
