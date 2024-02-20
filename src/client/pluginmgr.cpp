#include <erebus/exception.hxx>
#include <erebus/util/format.hxx>

#include "pluginmgr.hpp"

#include <QDir>


namespace Erc
{

namespace Private
{

IPlugin* PluginManager::load(const QString& path)
{
    auto info = std::make_shared<PluginInfo>(path, m_params.log);
    if (!info->dll.load())
    {
        auto error = info->dll.errorString();
        throw Er::Exception(ER_HERE(), Er::Util::format("Plugin [%s] could not be loaded", Erc::toUtf8(path).c_str()), Er::ExceptionProps::DecodedError(Erc::toUtf8(error)));
    }

    auto createUiPluginFn = reinterpret_cast<Erc::createUiPlugin*>(info->dll.resolve("createUiPlugin"));
    if (!createUiPluginFn)
        throw Er::Exception(ER_HERE(), Er::Util::format("Plugin [%s] contains no createUiPlugin symbol", Erc::toUtf8(path).c_str()));

    info->disposeFn = reinterpret_cast<Erc::disposeUiPlugin*>(info->dll.resolve("disposeUiPlugin"));
    if (!info->disposeFn)
        throw Er::Exception(ER_HERE(), Er::Util::format("Plugin [%s] contains no disposeUiPlugin symbol", Erc::toUtf8(path).c_str()));

    info->ref = createUiPluginFn(m_params);

    m_plugins.insert({path, info});

    m_params.log->write(Er::Log::Level::Info, LogNowhere(), "Loaded plugin [%s]", Erc::toUtf8(path).c_str());


    return info->ref;
}


} // namespace Private {}

} // namespace Erc {}
