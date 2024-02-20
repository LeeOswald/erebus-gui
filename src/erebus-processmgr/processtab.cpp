#include "processtab.hpp"

#include <QGridLayout>
#include <QHeaderView>

namespace Erp
{

namespace Private
{


ProcessTab::~ProcessTab()
{
    auto index = m_params.tabWidget->indexOf(m_widget);
    Q_ASSERT(index >= 0);
    m_params.tabWidget->removeTab(index);

    delete m_treeView;
    delete m_widget;
}

ProcessTab::ProcessTab(const Erc::PluginParams& params, Er::Client::IClient* client, const std::string& endpoint)
    : QObject()
    , m_params(params)
    , m_client(client)
    , m_endpoint(endpoint)
    , m_widget(new QWidget(params.tabWidget))
    , m_treeView(new QTreeView(m_widget))
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

}


} // namespace Private {}

} // namespace Erp {}
