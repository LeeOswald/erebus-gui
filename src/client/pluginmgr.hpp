#pragma once


#include <erebus/noncopyable.hxx>
#include <erebus-gui/plugin.hpp>

#include <vector>

#include <QLibrary>


namespace Erc
{

namespace Private
{


class PluginManager final
    : public Er::NonCopyable
{
public:
    ~PluginManager()
    {
    }

    explicit PluginManager(const PluginParams& params)
        : m_params(params)
    {
    }

    IPlugin* load(const QString& name);

private:
    struct PluginInfo
    {
        QString path;
        Er::Log::ILog* log = nullptr;
        QLibrary dll;
        Erc::IPlugin* ref = nullptr;
        Erc::disposeUiPlugin* disposeFn = nullptr;

        ~PluginInfo()
        {
            if (disposeFn)
                disposeFn(ref);

            if (dll.isLoaded())
            {
                log->write(Er::Log::Level::Info, "Unloading plugin [%s]", Erc::toUtf8(path).c_str());
                dll.unload();
            }
        }

        explicit PluginInfo(const QString& path, Er::Log::ILog* log)
            : path(path)
            , log(log)
            , dll(path)
        {
        }
    };

    PluginParams m_params;
    std::vector<std::shared_ptr<PluginInfo>> m_plugins;
};


} // namespace Private {}

} // namespace Erc {}