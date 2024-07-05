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

    delete m_contextMenu;

    auto index = m_params.tabWidget->indexOf(m_widget);
    Q_ASSERT(index >= 0);
    m_params.tabWidget->removeTab(index);

    m_params.statusBar->removeWidget(m_labelCpuUsage);
    delete m_labelCpuUsage;

    m_params.statusBar->removeWidget(m_labelTotalProcesses);
    delete m_labelTotalProcesses;

    delete m_treeView;
    delete m_widget;
}

ProcessTab::ProcessTab(const Erc::PluginParams& params, std::shared_ptr<void> channel, const std::string& endpoint)
    : QObject(params.tabWidget)
    , m_params(params)
    , m_autoRefresh(Erc::Option<bool>::get(params.settings, Erp::Settings::autoRefresh, true))
    , m_refreshRate(Erc::Option<unsigned>::get(params.settings, Erp::Settings::refreshRate, Erp::Settings::RefreshRateDefault))
    , m_trackDuration(Erc::Option<unsigned>::get(params.settings, Erp::Settings::trackDuration, Erp::Settings::TrackDurationDefault))
    , m_refreshTimer(new QTimer(this))
    , m_columns(loadProcessColumns(m_params.settings))
    , m_required(makePropMask(m_columns))
    , m_channel(channel)
    , m_endpoint(endpoint)
    , m_widget(new QWidget(params.tabWidget))
    , m_treeView(new QTreeView(m_widget))
    , m_labelTotalProcesses(new QLabel(m_widget))
    , m_labelCpuUsage(new QLabel(m_widget))
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

    params.statusBar->addWidget(m_labelTotalProcesses);
    params.statusBar->addWidget(m_labelCpuUsage);

    m_contextMenu = new ItemMenu(m_treeView);
    connect(m_contextMenu, SIGNAL(kill(quint64,QLatin1String)), this, SLOT(kill(quint64,QLatin1String)));

    startWorker();

    if (m_refreshRate < 500)
        m_refreshRate = 1000;

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

    ErLogDebug(m_params.log, ErLogInstance("ProcessTab"), "Set refresh interval to %d msec", m_refreshRate);
}

void ProcessTab::requireAdditionalProps(Er::ProcessProps::PropMask& required) noexcept
{
    // what we need even if there's no corresponding visible column
    required.set(Er::ProcessProps::PropIndices::CmdLine);
    required.set(Er::ProcessProps::PropIndices::Exe);
    required.set(Er::ProcessProps::PropIndices::UTime);
    required.set(Er::ProcessProps::PropIndices::STime);
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
    m_worker = new ProcessListWorker(m_channel, m_params.log, nullptr);

    // auto-delete thread
    connect(m_thread, SIGNAL(finished()), m_thread, SLOT(deleteLater()));

    // auto-delete worker
    connect(m_thread, SIGNAL(finished()), m_worker, SLOT(deleteLater()));
    m_worker->moveToThread(m_thread);

    // know when worker has data
    connect(m_worker, SIGNAL(dataReady(ProcessChangesetPtr,bool)), this, SLOT(dataReady(ProcessChangesetPtr,bool)));
    connect(m_worker, SIGNAL(posixResult(Erp::Private::IProcessList::PosixResult)), this, SLOT(posixResult(Erp::Private::IProcessList::PosixResult)));

    m_thread->start();
}

void ProcessTab::refresh(bool manual)
{
    if (m_worker)
    {
        ErLogDebug(m_params.log, ErLogInstance("ProcessTab"), "Refreshing...");

        QMetaObject::invokeMethod(m_worker, "refresh", Qt::AutoConnection, Q_ARG(Er::ProcessProps::PropMask, m_required), Q_ARG(int, m_trackDuration), Q_ARG(bool, manual));
    }
}

void ProcessTab::dataReady(ProcessChangesetPtr changeset, bool manual)
{
    Er::protectedCall<void>(
        m_params.log,
        ErLogInstance("ProcessTab"),
        [this, changeset]()
        {
            if (!m_model)
            {
                m_model = new ProcessTreeModel(m_params.log, changeset, m_columns, this);

                m_treeView->setModel(m_model);
                m_treeView->expandAll();
                m_contextMenu->setModel(m_model);

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

            m_labelTotalProcesses->setText(tr("Processes: ") + QString::number(changeset->totalProcesses));

            if (changeset->firstRun)
            {
                m_labelCpuUsage->clear();
            }
            else
            {
                auto usage = changeset->cpuTime * 100.0 / changeset->realTime;
                usage = std::clamp(usage, 0.0, 100.0);

                std::ostringstream ss;
                ss << "CPU: " << std::fixed << std::setprecision(2) << usage << "%";
                m_labelCpuUsage->setText(QString::fromLatin1(ss.str()));
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

void ProcessTab::kill(quint64 pid, QLatin1String signal)
{
    if (m_worker)
    {
        ErLogDebug(m_params.log, ErLogInstance("ProcessTab"), "Kill(%zu, %s)", pid, signal.data());

        QMetaObject::invokeMethod(m_worker, "kill", Qt::AutoConnection, Q_ARG(quint64, pid), Q_ARG(QLatin1String, signal));
    }
}

void ProcessTab::posixResult(Erp::Private::IProcessList::PosixResult result)
{
    if (result.code != 0)
    {
        if (!result.message.empty())
        {
            ErLogError(m_params.log, ErLogInstance("ProcessTab"), "%d %s", result.code, result.message.c_str());
        }
        else
        {
            ErLogError(m_params.log, ErLogInstance("ProcessTab"), "Unspecified error %d", result.code);
        }
    }
}

} // namespace Private {}

} // namespace Erp {}
