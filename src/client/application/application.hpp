#pragma once

#include <erebus/log.hxx>
#include <erebus-gui/settings.hpp>

#include "applicationbase.hpp"


namespace Erp
{

namespace Client
{

class Application final
    : public ApplicationBase
{
    Q_OBJECT

public:
    ~Application();
    explicit Application(Er::Log::ILog* log, Erc::ISettingsStorage* settings, int& argc, char** argv);

private:
    Er::Log::ILog* m_log;
    Erc::ISettingsStorage* m_settings;

};

} // namespace Client {}

} // namespace Erp {}
