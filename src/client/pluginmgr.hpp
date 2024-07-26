#pragma once


#include <erebus/erebus.hxx>
#include <erebus-gui/plugin.hpp>


#include <unordered_map>
#include <vector>

#include <QLibrary>


namespace Erp
{

namespace Client
{


class PluginManager final
    : public Er::NonCopyable
{
public:
    ~PluginManager()
    {
    }

    explicit PluginManager(const Erc::PluginParams& params)
        : m_params(params)
    {
    }

    bool exists(const QString& path)
    {
        return (m_plugins.find(path) != m_plugins.end());
    }

    size_t count() const noexcept
    {
        return m_plugins.size();
    }

    Erc::IPlugin* load(const QString& path);

    template <typename Visitor>
    void forEachPlugin(Visitor v)
    {
        for (auto& plugin: m_plugins)
        {
            v(plugin.second->ref.get());
        }
    }

private:
    struct PluginInfo
    {
        QString path;
        Er::Log::ILog* log = nullptr;
        QLibrary dll;
        std::unique_ptr<Erc::IPlugin> ref;

        ~PluginInfo()
        {
            ref.reset();

            if (dll.isLoaded())
            {
                log->writef(Er::Log::Level::Info, "Unloading plugin [%s]", Erc::toUtf8(path).c_str());
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

    Erc::PluginParams m_params;
    std::unordered_map<QString, std::shared_ptr<PluginInfo>> m_plugins;
};


} // namespace Client {}

} // namespace Erp {}
