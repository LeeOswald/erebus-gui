#include "processtab.hpp"

#include <QGridLayout>
#include <QHeaderView>
#include <QTimer>

namespace Erp
{

namespace Private
{


ProcessTab::~ProcessTab()
{
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
    : QObject()
    , m_params(params)
    , m_refreshRate(Erc::Option<unsigned>::get(params.settings, Erp::Settings::refreshRate, Erp::Settings::RefreshRateDefault))
    , m_columns(loadProcessColumns(m_params.settings))
    , m_required(makePropMask(m_columns))
    , m_client(client)
    , m_endpoint(endpoint)
    , m_widget(new QWidget(params.tabWidget))
    , m_treeView(new QTreeView(m_widget))
    , m_thread()
    , m_worker()
{
    requireAdditionalProps(m_required);

    m_widget->setObjectName("processTabWidget");
    auto layout = new QGridLayout(m_widget);
    layout->setSpacing(0);
    layout->setObjectName("gridLayout");
    layout->setContentsMargins(0, 0, 0, 0);

    m_treeView->setObjectName("processesTree");
    m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_treeView->setProperty("showDropIndicator", QVariant(false));
    m_treeView->setIconSize(QSize(16, 16));
    m_treeView->setSortingEnabled(false);
    m_treeView->header()->setProperty("showSortIndicator", QVariant(false));
    m_treeView->header()->setStretchLastSection(false);

    layout->addWidget(m_treeView, 0, 0, 1, 1);

    params.tabWidget->addTab(m_widget, QString());
    params.tabWidget->setTabText(params.tabWidget->indexOf(m_widget), tr("Processes"));

    startWorker();

    if (m_refreshRate < 500)
        m_refreshRate = 500;

    QTimer::singleShot(0, this, SLOT(refresh()));
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
}

void ProcessTab::saveColumns()
{
    captureColumnWidths();
    Erp::Private::saveProcessColumns(m_params.settings, m_columns);
}

void ProcessTab::reloadColumns()
{
    delete m_model;
    m_model = nullptr;

    m_columns = loadProcessColumns(m_params.settings);
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
    connect(m_worker, SIGNAL(dataReady(ProcessChangesetPtr)), this, SLOT(dataReady(ProcessChangesetPtr)));

    m_thread->start();
}

void ProcessTab::refresh()
{
    if (m_worker)
    {
        QMetaObject::invokeMethod(m_worker, "refresh", Qt::AutoConnection, Q_ARG(Er::ProcessProps::PropMask, m_required), Q_ARG(int, 5000));
    }
}

void ProcessTab::dataReady(ProcessChangesetPtr changeset)
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
                // QTreeView does not expand new items automatically; we need to do this explicitly
                auto parentsToExpand = m_model->update(changeset);
                for (auto& index : parentsToExpand)
                {
                    m_treeView->expandRecursively(index);
                }
            }
        }
    );

    // schedule the next refresh
    QTimer::singleShot(m_refreshRate, this, SLOT(refresh()));
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
