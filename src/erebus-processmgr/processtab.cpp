#include "processtab.hpp"

#include <QGridLayout>
#include <QHeaderView>

namespace Erp
{

namespace ProcessMgr
{


ProcessTab::~ProcessTab()
{
    m_refreshTimer->stop();
    
    saveColumns();

    m_processListWorker.destroy();

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

ProcessTab::ProcessTab(const Erc::PluginParams& params, Er::Client::ChannelPtr channel, const std::string& endpoint)
    : QObject(params.tabWidget)
    , m_params(params)
    , m_autoRefresh(Erc::Option<bool>::get(params.settings, Erp::ProcessMgr::Settings::autoRefresh, true))
    , m_refreshRate(Erc::Option<unsigned>::get(params.settings, Erp::ProcessMgr::Settings::refreshRate, Erp::ProcessMgr::Settings::RefreshRateDefault))
    , m_trackDuration(Erc::Option<unsigned>::get(params.settings, Erp::ProcessMgr::Settings::trackDuration, Erp::ProcessMgr::Settings::TrackDurationDefault))
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

    ErLogDebug(m_params.log, "Set refresh interval to %d msec", m_refreshRate);
}

void ProcessTab::requireAdditionalProps(Er::ProcessMgr::ProcessProps::PropMask& required) noexcept
{
    // what we need even if there's no corresponding visible column
    required.set(Er::ProcessMgr::ProcessProps::PropIndices::CmdLine);
    required.set(Er::ProcessMgr::ProcessProps::PropIndices::Exe);
    required.set(Er::ProcessMgr::ProcessProps::PropIndices::UTime);
    required.set(Er::ProcessMgr::ProcessProps::PropIndices::STime);
}

void ProcessTab::saveColumns()
{
    captureColumnWidths();
    Erp::ProcessMgr::saveProcessColumns(m_params.settings, m_columns);
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
    m_processListWorker.make(m_channel, m_params.log);

    // know when worker has data
    connect(m_processListWorker.worker, SIGNAL(dataReady(ProcessChangesetPtr,bool)), this, SLOT(dataReady(ProcessChangesetPtr,bool)));
    connect(m_processListWorker.worker, SIGNAL(posixResult(Erp::ProcessMgr::IProcessList::PosixResult)), this, SLOT(posixResult(Erp::ProcessMgr::IProcessList::PosixResult)));

    m_processListWorker.start();
}

void ProcessTab::refresh(bool manual)
{
    ErLogDebug(m_params.log, "Refreshing...");

    m_processListWorker.refresh(manual, m_required, m_trackDuration);
}

void ProcessTab::dataReady(ProcessChangesetPtr changeset, bool manual)
{
    Er::protectedCall<void>(
        m_params.log,
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
    ErLogDebug(m_params.log, "Kill(%zu, %s)", pid, signal.data());

    m_processListWorker.kill(pid, signal);
}

void ProcessTab::posixResult(Erp::ProcessMgr::IProcessList::PosixResult result)
{
    if (result.code != 0)
    {
        if (!result.message.empty())
        {
            ErLogError(m_params.log, "%d %s", result.code, result.message.c_str());
        }
        else
        {
            ErLogError(m_params.log, "Unspecified error %d", result.code);
        }
    }
}

} // namespace ProcessMgr {}

} // namespace Erp {}
