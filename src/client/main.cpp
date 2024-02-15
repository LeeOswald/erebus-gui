#include "client-version.h"

#include "appsettings.hpp"
#include "application/application.hpp"

#include <erebus/log.hxx>

#include <QMessageBox>

#if defined(_MSC_VER) && ER_DEBUG
    #include <crtdbg.h>
#endif



int main(int argc, char *argv[])
{
#if defined(_MSC_VER) && ER_DEBUG
    int tmpFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    tmpFlag |= _CRTDBG_LEAK_CHECK_DF;
    _CrtSetDbgFlag(tmpFlag);
#endif

    Erc::Private::Application::setApplicationName(EREBUS_APPLICATION_NAME);
    Erc::Private::Application::setApplicationVersion(QString("%1.%2.%3").arg(EREBUS_VERSION_MAJOR).arg(EREBUS_VERSION_MINOR).arg(EREBUS_VERSION_PATCH));
    Erc::Private::Application::setOrganizationName(EREBUS_ORGANIZATION_NAME);

    Erc::Private::Settings settings;
    auto logLevel = static_cast<Er::Log::Level>(Erc::Option<int>::get(&settings, Erc::Private::AppSettings::Log::level, int(Erc::Private::AppSettings::Log::defaultLevel)));
    auto singleInstance = Erc::Option<bool>::get(&settings, Erc::Private::AppSettings::Application::singleInstance, Erc::Private::AppSettings::Application::singleInstanceDefault);

    Er::Log::LogBase log(logLevel, 65536);

    
    try
    {
        Erc::Private::Application a(&log, &settings, argc, argv);

        if (singleInstance && a.secondary())
        {
#if ER_WINDOWS
            ::AllowSetForegroundWindow(DWORD(a.primaryPid()));
#endif
            a.sendMessage("ACTIVATE_WINDOW", 100);
            return EXIT_SUCCESS;
        }

        return a.exec();
    }
    catch (std::exception& e)
    {
        QMessageBox::critical(nullptr, QString::fromLocal8Bit(EREBUS_APPLICATION_NAME), QLatin1String("Unexpected error"), QMessageBox::Ok);
    }

    return EXIT_FAILURE;
}


