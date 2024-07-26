#include "application.hpp"



namespace Erp
{

namespace Client
{

Application::~Application()
{
}

Application::Application(Er::Log::ILog* log, Erc::ISettingsStorage* settings, int& argc, char** argv)
    : ApplicationBase(argc, argv)
    , m_log(log)
    , m_settings(settings)
{
}


} // namespace Client {}

} // namespace Erp {}
