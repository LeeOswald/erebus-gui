#include <erebus/exception.hxx>
#include <erebus/util/format.hxx>

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
        ErThrow(Er::Util::format("Plugin [%s] could not be loaded", Erc::toUtf8(path).c_str()), Er::ExceptionProps::DecodedError(Erc::toUtf8(error)));
    }

    auto createUiPluginFn = reinterpret_cast<Erc::createUiPlugin*>(info->dll.resolve("createUiPlugin"));
    if (!createUiPluginFn)
        ErThrow(Er::Util::format("Plugin [%s] contains no createUiPlugin symbol", Erc::toUtf8(path).c_str()));

    info->ref.reset(createUiPluginFn(m_params));
    if (!info->ref)
        ErThrow(Er::Util::format("Plugin [%s] returned no interface", Erc::toUtf8(path).c_str()));

    m_plugins.insert({path, info});

    auto pi = info->ref->info();
    m_params.log->writef(
        Er::Log::Level::Info,
        "Loaded plugin %s ver %u.%u.%u [%s] from [%s]",
        pi.name.c_str(),
        pi.version.major,
        pi.version.minor,
        pi.version.patch,
        pi.description.c_str(),
        Erc::toUtf8(path).c_str()
    );

    return info->ref.get();
}


} // namespace Client {}

} // namespace Erp {}
