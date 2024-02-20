#pragma once

#include "processmgr.hpp"

#include <QTreeView>
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

private:
    Erc::PluginParams m_params;
    Er::Client::IClient* m_client;
    std::string m_endpoint;
    QWidget* m_widget;
    QTreeView* m_treeView;
};

} // namespace Private {}

} // namespace Erp {}
