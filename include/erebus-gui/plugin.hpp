#pragma once

#include <erebus-clt/erebus-clt.hxx>
#include <erebus-gui/erebus-gui.hpp>
#include <erebus-gui/settings.hpp>
#include <erebus/log.hxx>

#include <QMenuBar>
#include <QStatusBar>
#include <QString>
#include <QTabWidget>

namespace Erc
{
    
struct PluginParams
{
    Erc::ISettingsStorage* settings = nullptr;
    Er::Log::ILog* log = nullptr;
    QTabWidget* tabWidget = nullptr;
    QMenuBar* mainMenu = nullptr;
    QStatusBar* statusBar = nullptr;

    constexpr PluginParams() noexcept = default;

    explicit PluginParams(
        Erc::ISettingsStorage* settings,
        Er::Log::ILog* log,
        QTabWidget* tabWidget,
        QMenuBar* mainMenu,
        QStatusBar* statusBar
    )
        : settings(settings)
        , log(log)
        , tabWidget(tabWidget)
        , mainMenu(mainMenu)
        , statusBar(statusBar)
    {
    }
};


struct IPlugin
{
    struct Info
    {
        struct Version
        {
            unsigned major;
            unsigned minor;
            unsigned patch;

            Version(unsigned major, unsigned minor, unsigned patch)
                : major(major)
                , minor(minor)
                , patch(patch)
            {}
        };

        std::string name;
        std::string description;
        Version version;

        Info(std::string_view name, std::string_view description, const Version& version)
            : name(name)
            , description(description)
            , version(version)
        {}
    };

    virtual ~IPlugin() {}
    virtual Info info() const = 0;
    virtual void addConnection(std::shared_ptr<void> channel, const std::string& endpoint) = 0;
    virtual void removeConnection(std::shared_ptr<void> channel) noexcept = 0;
};


typedef IPlugin* (createUiPlugin)(const PluginParams&);


} // namespace Erc {}
