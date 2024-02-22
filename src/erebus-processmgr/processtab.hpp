#pragma once

#include <erebus-clt/erebus-clt.hxx>

#include "processcolumns.hpp"
#include "processmgr.hpp"
#include "proclistworker.hpp"
#include "proctreemodel.hpp"

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

    void saveColumns()
    {
        captureColumnWidths();
    }

    void reloadColumns();

private slots:
    void dataReady(ProcessChangesetPtr changeset);
    void refresh();

private:
    void captureColumnWidths();
    void restoreColumnWidths();
    void resetWorker();
    void startWorker();

    Erc::PluginParams m_params;
    ProcessColumns m_columns;
    Er::Client::IClient* m_client;
    std::string m_endpoint;
    QTimer* m_timer;
    QWidget* m_widget;
    QTreeView* m_treeView;
    QPointer<QThread> m_thread;
    QPointer<ProcessListWorker> m_worker;
    ProcessTreeModel* m_model = nullptr;
};

} // namespace Private {}

} // namespace Erp {}
