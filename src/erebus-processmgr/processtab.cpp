#include "processtab.hpp"

#include <QGridLayout>
#include <QHeaderView>

namespace Erp
{

namespace Private
{


ProcessTab::~ProcessTab()
{
    saveColumns();

    resetWorker();

    auto index = m_params.tabWidget->indexOf(m_widget);
    Q_ASSERT(index >= 0);
    m_params.tabWidget->removeTab(index);

    delete m_treeView;
    delete m_widget;
}

ProcessTab::ProcessTab(const Erc::PluginParams& params, Er::Client::IClient* client, const std::string& endpoint)
    : QObject()
    , m_params(params)
    , m_columns(loadProcessColumns(m_params.settings))
    , m_client(client)
    , m_endpoint(endpoint)
    , m_timer(new QTimer(this))
    , m_widget(new QWidget(params.tabWidget))
    , m_treeView(new QTreeView(m_widget))
    , m_thread()
    , m_worker()
{
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

    connect(m_timer, SIGNAL(timeout()), this, SLOT(refresh()));
    m_timer->start(5000);

    QTimer::singleShot(0, this, SLOT(refresh()));
}

void ProcessTab::saveColumns()
{
    captureColumnWidths();
    Erp::Private::saveProcessColumns(m_params.settings, m_columns);
}

void ProcessTab::reloadColumns()
{
    m_params.log->write(Er::Log::Level::Debug, LogNowhere(), "reloadColumns() ->");

    resetWorker();
    m_params.log->write(Er::Log::Level::Debug, LogNowhere(), "-- 1 -");
    m_columns = loadProcessColumns(m_params.settings);
    m_params.log->write(Er::Log::Level::Debug, LogNowhere(), "-- 2 -");
    startWorker();

    m_params.log->write(Er::Log::Level::Debug, LogNowhere(), "reloadColumns() <-");
}

void ProcessTab::resetWorker()
{
    m_worker.clear();

    if (m_thread)
    {
        m_thread->quit();
        m_thread->wait();
        m_thread.clear();
    }

    delete m_model;
    m_model = nullptr;
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
    m_params.log->write(Er::Log::Level::Debug, LogInstance("Tab"), "refresh() ->");
    if (m_worker)
    {
        QMetaObject::invokeMethod(m_worker, "refresh", Qt::AutoConnection, Q_ARG(int, 5000));
    }
    m_params.log->write(Er::Log::Level::Debug, LogInstance("Tab"), "refresh() <-");
}

void ProcessTab::dataReady(ProcessChangesetPtr changeset)
{
    m_params.log->write(Er::Log::Level::Debug, LogNowhere(), "dataReady() ->");

    Erc::Ui::protectedCall<void>(
        m_params.log,
        [this, changeset]()
        {
            if (!m_model)
            {
                m_params.log->write(Er::Log::Level::Debug, LogNowhere(), "Model CREATING");
                m_model = new ProcessTreeModel(m_params.log, changeset, m_columns, this);

                m_treeView->setModel(m_model);
                m_treeView->expandAll();

                restoreColumnWidths();
            }
            else
            {
                m_params.log->write(Er::Log::Level::Debug, LogNowhere(), "Model exists");
                m_model->update(changeset);
            }
        }
    );

    m_params.log->write(Er::Log::Level::Debug, LogNowhere(), "dataReady() <-");
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
