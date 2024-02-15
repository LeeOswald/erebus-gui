#include "client-version.h"

#include "appsettings.hpp"
#include "application/application.hpp"

#include <erebus/log.hxx>
#include <erebus-gui/exceptionutil.hpp>

#include <QtGlobal>
#include <QMessageBox>

#if defined(_MSC_VER) && ER_DEBUG
    #include <crtdbg.h>
#endif

namespace
{

Er::Log::ILog* g_log = nullptr;

void qtMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    if (!g_log)
        return;

    auto localMsg = Erc::toUtf8(msg);
    switch (type)
    {
    case QtDebugMsg:
        LogDebug(g_log, "%s", localMsg.c_str());
        break;
    case QtInfoMsg:
        LogInfo(g_log, "%s", localMsg.c_str());
        break;
    case QtWarningMsg:
        LogWarning(g_log, "%s", localMsg.c_str());
        break;
    case QtCriticalMsg:
        LogError(g_log, "%s", localMsg.c_str());
        break;
    case QtFatalMsg:
        LogFatal(g_log, "%s", localMsg.c_str());
        break;
    }
}

} // namespace {}


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
    g_log = &log;
    ::qInstallMessageHandler(qtMessageHandler);
    
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

        auto result = a.exec();

        g_log = nullptr;
        ::qInstallMessageHandler(nullptr);

        return result;
    }
    catch (Er::Exception& e)
    {
        auto msg = Erc::formatException(e);
        QMessageBox::critical(nullptr, QString::fromUtf8(EREBUS_APPLICATION_NAME), QString::fromUtf8(msg), QMessageBox::Ok);
    }
    catch (std::exception& e)
    {
        auto msg = Erc::formatException(e);
        QMessageBox::critical(nullptr, QString::fromUtf8(EREBUS_APPLICATION_NAME), QString::fromUtf8(msg), QMessageBox::Ok);
    }

    g_log = nullptr;
    ::qInstallMessageHandler(nullptr);

    return EXIT_FAILURE;
}


