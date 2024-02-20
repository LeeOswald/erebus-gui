#pragma once

#include <erebus-clt/erebus-clt.hxx>
#include <erebus-gui/erebus-gui.hpp>
#include <erebus-gui/settings.hpp>
#include <erebus/log.hxx>

#include <QString>
#include <QTabWidget>

namespace Erc
{
    
struct PluginParams
{
    Erc::ISettingsStorage* settings = nullptr;
    Er::Log::ILog* log = nullptr;
    QTabWidget* tabWidget = nullptr;

    explicit PluginParams(
        Erc::ISettingsStorage* settings,
        Er::Log::ILog* log,
        QTabWidget* tabWidget
    )
        : settings(settings)
        , log(log)
        , tabWidget(tabWidget)
    {
    }
};


struct IPlugin
{
    virtual void addConnection(Er::Client::IClient* client, const std::string& endpoint) = 0;
    virtual void removeConnection(Er::Client::IClient* client) = 0;

protected:
    virtual ~IPlugin() {}
};


// the only symbols any plugin must export
typedef IPlugin* (createUiPlugin)(const PluginParams&);
typedef void (disposeUiPlugin)(IPlugin*);


} // namespace Erc {}
