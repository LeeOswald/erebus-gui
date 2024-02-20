#include "processmgr.hpp"
#include "processtab.hpp"

#include <erebus-processmgr/processprops.hxx>

#include <unordered_map>

namespace
{

class ProcessMgrPlugin final
    : public Erc::IPlugin
    , public Er::NonCopyable
{
public:
    ~ProcessMgrPlugin()
    {
        Er::ProcessProps::Private::unregisterAll();
    }

    explicit ProcessMgrPlugin(const Erc::PluginParams& params)
        : m_params(params)
    {
        Er::ProcessProps::Private::registerAll();
    }

    void addConnection(Er::Client::IClient* client, const std::string& endpoint) override
    {
        auto it = m_tabs.find(client);
        if (it != m_tabs.end())
        {
            m_params.log->write(Er::Log::Level::Error, LogInstance("ProcessMgrPlugin"), "Connection %p to %s already has a tab", client, endpoint.c_str());
            return;
        }

        m_tabs.insert({ client, std::make_unique<Erp::Private::ProcessTab>(m_params, client, endpoint) });
    }

    void removeConnection(Er::Client::IClient* client) override
    {
        auto it = m_tabs.find(client);
        if (it == m_tabs.end())
        {
            m_params.log->write(Er::Log::Level::Error, LogInstance("ProcessMgrPlugin"), "No tab found for connection %p", client);
            return;
        }

        m_tabs.erase(it);
    }

private:
    Erc::PluginParams m_params;
    std::unordered_map<Er::Client::IClient*, std::unique_ptr<Erp::Private::ProcessTab>> m_tabs;
};

} // namespace {}


extern "C"
{

EREBUSPROCMGR_EXPORT Erc::IPlugin* createUiPlugin(const Erc::PluginParams& params)
{
    return new ProcessMgrPlugin(params);
}

EREBUSPROCMGR_EXPORT void disposeUiPlugin(Erc::IPlugin* plugin)
{
    assert(plugin);
    if (!plugin)
        return;

    auto realPlugin = dynamic_cast<ProcessMgrPlugin*>(plugin);
    assert(realPlugin);

    delete realPlugin;
}



} // extern "C" {}
