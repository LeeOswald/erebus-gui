#include "processmgr.hpp"


namespace
{

class ProcessMgrPlugin final
    : public Erc::IPlugin
    , public Er::NonCopyable
{
public:
    ~ProcessMgrPlugin()
    {

    }

    explicit ProcessMgrPlugin(const Erc::PluginParams& params)
        : m_params(params)
    {

    }

private:
    Erc::PluginParams m_params;
};

} // namespace {}


extern "C"
{

Erc::IPlugin* createUiPlugin(const Erc::PluginParams& params)
{
    return new ProcessMgrPlugin(params);
}

void disposeUiPlugin(Erc::IPlugin* plugin)
{
    assert(plugin);
    if (!plugin)
        return;

    auto realPlugin = dynamic_cast<ProcessMgrPlugin*>(plugin);
    assert(realPlugin);

    delete realPlugin;
}



} // extern "C" {}
