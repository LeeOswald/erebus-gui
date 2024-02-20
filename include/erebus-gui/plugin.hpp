#pragma once

#include <erebus-gui/erebus-gui.hpp>
#include <erebus-gui/settings.hpp>
#include <erebus/log.hxx>

#include <QString>

namespace Erc
{
    
struct PluginParams
{
    Erc::ISettingsStorage* settings = nullptr;
    Er::Log::ILog* log = nullptr;

    constexpr PluginParams() noexcept = default;

    explicit PluginParams(
        Erc::ISettingsStorage* settings,
        Er::Log::ILog* log
    )
        : settings(settings)
        , log(log)
    {
    }
};


struct IPlugin
{
protected:
    virtual ~IPlugin() {}
};


// the only symbols any plugin must export
typedef IPlugin* (createUiPlugin)(const PluginParams&);
typedef void (disposeUiPlugin)(IPlugin*);


} // namespace Erc {}
