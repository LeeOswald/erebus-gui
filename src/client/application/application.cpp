#include "application.hpp"



namespace Erc
{

namespace Private
{

Application::~Application()
{
}

Application::Application(Er::Log::ILog* log, ISettingsStorage* settings, int& argc, char** argv)
    : ApplicationBase(argc, argv)
    , m_log(log)
    , m_settings(settings)
{
}


} // namespace Private {}

} // namespace Erc {}