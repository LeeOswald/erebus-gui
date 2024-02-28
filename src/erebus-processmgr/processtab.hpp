#pragma once

#include <erebus-clt/erebus-clt.hxx>

#include "processcolumns.hpp"
#include "processmgr.hpp"
#include "proclistworker.hpp"
#include "proctreemodel.hpp"

#include <QPointer>
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

private slots:
    void dataReady(ProcessChangesetPtr changeset);
    void refresh();

private:
    void captureColumnWidths();
    void restoreColumnWidths();
    void startWorker();
    static void requireAdditionalProps(Er::ProcessProps::PropMask& required) noexcept;

    Erc::PluginParams m_params;
    unsigned m_refreshRate; // msec
    ProcessColumns m_columns;
    Er::ProcessProps::PropMask m_required;
    Er::Client::IClient* m_client;
    std::string m_endpoint;
    QWidget* m_widget;
    QTreeView* m_treeView;
    QPointer<QThread> m_thread;
    QPointer<ProcessListWorker> m_worker;
    ProcessTreeModel* m_model = nullptr;
};

} // namespace Private {}

} // namespace Erp {}
