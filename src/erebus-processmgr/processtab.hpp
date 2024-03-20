#pragma once

#include <erebus-clt/erebus-clt.hxx>

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

namespace Private
{

class ProcessTab final
    : public QObject
{
    Q_OBJECT

public:
    ~ProcessTab();
    explicit ProcessTab(const Erc::PluginParams& params, Er::Client::IClient* client, const std::string& endpoint);

    void saveColumns();
    void reloadColumns();
    void setRefreshInterval(unsigned interval);
    void setAutoRefresh(bool autoRefresh);

public slots:
    void refresh(bool manual);

private slots:
    void dataReady(ProcessChangesetPtr changeset, bool manual);
    
private:
    void captureColumnWidths();
    void restoreColumnWidths();
    void startWorker();
    static void requireAdditionalProps(Er::ProcessProps::PropMask& required) noexcept;

    Erc::PluginParams m_params;
    bool m_autoRefresh;
    unsigned m_refreshRate; // msec
    unsigned m_trackDuration;
    QTimer* m_refreshTimer;
    ProcessColumns m_columns;
    bool m_columnsChanged = false;
    Er::ProcessProps::PropMask m_required;
    Er::Client::IClient* m_client;
    std::string m_endpoint;
    QWidget* m_widget;
    QTreeView* m_treeView;
    QPointer<QThread> m_thread;
    QPointer<ProcessListWorker> m_worker;
    ProcessTreeModel* m_model = nullptr;
    QLabel* m_labelTotalProcesses;
    QLabel* m_labelCpuUsage;
};

} // namespace Private {}

} // namespace Erp {}
