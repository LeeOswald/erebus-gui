#include <erebus/exception.hxx>
#include <erebus/format.hxx>

#include "pluginmgr.hpp"

#include <QDir>


namespace Erp
{

namespace Client
{

Erc::IPlugin* PluginManager::load(const QString& path)
{
    auto info = std::make_shared<PluginInfo>(path, m_params.log);
    if (!info->dll.load())
    {
        auto error = info->dll.errorString();
        ErThrow(Er::format("Plugin [{}] could not be loaded", Erc::toUtf8(path)), Er::ExceptionProps::DecodedError(Erc::toUtf8(error)));
    }

    auto createUiPluginFn = reinterpret_cast<Erc::createUiPlugin*>(info->dll.resolve("createUiPlugin"));
    if (!createUiPluginFn)
        ErThrow(Er::format("Plugin [{}] contains no createUiPlugin symbol", Erc::toUtf8(path)));

    info->ref.reset(createUiPluginFn(m_params));
    if (!info->ref)
        ErThrow(Er::format("Plugin [{}] returned no interface", Erc::toUtf8(path)));

    m_plugins.insert({path, info});

    auto pi = info->ref->info();
    Er::Log::info(m_params.log,
        "Loaded plugin {} ver {}.{}.{} [{}] from [{}]",
        pi.name,
        pi.version.major,
        pi.version.minor,
        pi.version.patch,
        pi.description,
        Erc::toUtf8(path)
    );

    return info->ref.get();
}


} // namespace Client {}

} // namespace Erp {}
