#pragma once

#include <erebus-gui/erebus-gui.hpp>
#include <erebus-gui/settings.hpp>
#include <erebus/log.hxx>

#include <QString>

namespace Erc
{
    
struct PluginParams
{
    QString pluginDir;
    Erc::ISettingsStorage* settings = nullptr;
    Er::Log::ILog* log = nullptr;

    constexpr PluginParams() noexcept = default;

    template <typename PluginDirT>
    explicit PluginParams(
        PluginDirT&& pluginDir,
        Erc::ISettingsStorage* settings,
        Er::Log::ILog* log
    )
        : pluginDir(std::forward<PluginDirT>(pluginDir))
        , settings(settings)
        , log(log)
    {
    }
};


struct IPlugin
{
    virtual ~IPlugin() {}
};


// the only symbols any plugin must export
typedef IPlugin* (createUiPlugin)(const PluginParams&);
typedef void (disposeUiPlugin)(IPlugin*);


} // namespace Erc {}