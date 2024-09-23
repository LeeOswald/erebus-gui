#pragma once

#include "itemmenu.hpp"
#include "processcolumns.hpp"
#include "processmgr.hpp"
#include "proclistworker.hpp"
#include "proctreemodel.hpp"

#include <QLabel>
#include <QPointer>
#include <QTimer>
#include <QTreeView>
#include <QThread>
#include <QWidget>

namespace Erp
{

namespace ProcessMgr
{

class ProcessTab final
    : public QObject
{
    Q_OBJECT

public:
    ~ProcessTab();
    explicit ProcessTab(const Erc::PluginParams& params, Er::Client::ChannelPtr channel, const std::string& endpoint);

    void saveColumns();
    void reloadColumns();
    void setRefreshInterval(unsigned interval);
    void setAutoRefresh(bool autoRefresh);

public slots:
    void refresh(bool manual);

private slots:
    void dataReady(ProcessChangesetPtr changeset, bool manual);
    void kill(quint64 pid, QLatin1String signal);
    void posixResult(Erp::ProcessMgr::IProcessList::PosixResult);
        
private:
    void captureColumnWidths();
    void restoreColumnWidths();
    void startWorker();
    static void requireAdditionalProps(Er::ProcessMgr::ProcessProps::PropMask& required) noexcept;

    Erc::PluginParams m_params;
    bool m_autoRefresh;
    unsigned m_refreshRate; // msec
    unsigned m_trackDuration;
    QTimer* m_refreshTimer;
    ProcessColumns m_columns;
    bool m_columnsChanged = false;
    Er::ProcessMgr::ProcessProps::PropMask m_required;
    Er::Client::ChannelPtr m_channel;
    std::string m_endpoint;
    QWidget* m_widget;
    QTreeView* m_treeView;
    ProcessListThread m_processListWorker;
    ProcessTreeModel* m_model = nullptr;
    QLabel* m_labelTotalProcesses;
    QLabel* m_labelCpuUsage;
    ItemMenu* m_contextMenu = nullptr;
};

} // namespace ProcessMgr {}

} // namespace Erp {}
