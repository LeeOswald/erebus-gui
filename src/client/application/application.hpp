#pragma once

#include <erebus/log.hxx>
#include <erebus-gui/settings.hpp>

#include "applicationbase.hpp"


namespace Erc
{

namespace Private
{

class Application final
    : public ApplicationBase
{
    Q_OBJECT

public:
    ~Application();
    explicit Application(Er::Log::ILog* log, ISettingsStorage* settings, int& argc, char** argv);

private:
    Er::Log::ILog* m_log;
    ISettingsStorage* m_settings;

};

} // namespace Private {}

} // namespace Erc {}