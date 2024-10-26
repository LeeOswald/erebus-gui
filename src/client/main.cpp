#include "client-version.h"

#include "appsettings.hpp"
#include "application/application.hpp"
#include "mainwindow/mainwindow.hpp"

#include <erebus/log.hxx>
#include <erebus/util/exceptionutil.hxx>
#include <erebus-clt/erebus-clt.hxx>

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
        Er::Log::debug(g_log, "[Qt] {}", localMsg);
        break;
    case QtInfoMsg:
        Er::Log::info(g_log, "[Qt] {}", localMsg);
        break;
    case QtWarningMsg:
        Er::Log::warning(g_log, "[Qt] {}", localMsg);
        break;
    case QtCriticalMsg:
        Er::Log::error(g_log, "[Qt] {}", localMsg);
        break;
    case QtFatalMsg:
        Er::Log::fatal(g_log, "[Qt] {}", localMsg);
        break;
    }
}


Er::Log::ILog::Ptr openLog(Er::Log::Level level)
{
    auto logger = Er::Log::makeAsyncLogger();

#if ER_WINDOWS
    if (::IsDebuggerPresent())
    {
        auto debugger = Er::Log::makeDebuggerSink(
            Er::Log::SimpleFormatter::make({ Er::Log::SimpleFormatter::Option::Time, Er::Log::SimpleFormatter::Option::Level, Er::Log::SimpleFormatter::Option::Tid }),
            Er::Log::SimpleFilter::make(Er::Log::Level::Debug, Er::Log::Level::Fatal)
        );

        logger->addSink("debugger", debugger);
    }
#endif

    logger->setLevel(level);

    return logger;
}

} // namespace {}


int main(int argc, char *argv[])
{
#if defined(_MSC_VER) && ER_DEBUG
    int tmpFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    tmpFlag |= _CRTDBG_LEAK_CHECK_DF;
    _CrtSetDbgFlag(tmpFlag);
#endif

    Erp::Client::Application::setApplicationName(EREBUS_APPLICATION_NAME);
    Erp::Client::Application::setApplicationVersion(QString("%1.%2.%3").arg(EREBUS_VERSION_MAJOR).arg(EREBUS_VERSION_MINOR).arg(EREBUS_VERSION_PATCH));
    Erp::Client::Application::setOrganizationName(EREBUS_ORGANIZATION_NAME);

    Erp::Client::Settings settings;
    auto logLevel = static_cast<Er::Log::Level>(Erc::Option<int>::get(&settings, Erp::Client::AppSettings::Log::level, int(Erp::Client::AppSettings::Log::defaultLevel)));
    auto singleInstance = Erc::Option<bool>::get(&settings, Erp::Client::AppSettings::Application::singleInstance, Erp::Client::AppSettings::Application::singleInstanceDefault);

    auto logger = openLog(logLevel);
    g_log = logger.get();
    ::qInstallMessageHandler(qtMessageHandler);
    
    try
    {
        Erp::Client::Application a(g_log, &settings, argc, argv);

        if (singleInstance && a.secondary())
        {
#if ER_WINDOWS
            ::AllowSetForegroundWindow(DWORD(a.primaryPid()));
#endif
            a.sendMessage("ACTIVATE_WINDOW", 100);
            return EXIT_SUCCESS;
        }

        Er::LibScope er(g_log);
        Er::Client::LibParams cltParams(g_log, g_log->level());
        Er::Client::LibScope cs(cltParams);

        try
        {
            Erp::Client::Ui::MainWindow w(g_log, &settings);

            if (a.primary())
            {
                QObject::connect(
                    &a,
                    &Erp::Client::Application::receivedMessage,
                    &w,
                    [&w](QByteArray message) { w.restore(); }
                );
            }

            auto result = a.exec();

            g_log = nullptr;
            ::qInstallMessageHandler(nullptr);

            return result;
        }
        catch (Er::Exception& e)
        {
            auto msg = Er::Util::formatException(e);
            Erc::Ui::errorBox(QCoreApplication::translate("Erebus", "Unexpected Error"), QString::fromUtf8(msg));
        }
        catch (std::exception& e)
        {
            auto msg = Er::Util::formatException(e);
            Erc::Ui::errorBox(QCoreApplication::translate("Erebus", "Unexpected Error"), QString::fromUtf8(msg));
        }

    }
    catch (Er::Exception& e)
    {
        auto msg = Er::Util::formatException(e);
        Erc::Ui::errorBox(QCoreApplication::translate("Erebus", "Unexpected Error"), QString::fromUtf8(msg));
    }
    catch (std::exception& e)
    {
        auto msg = Er::Util::formatException(e);
        Erc::Ui::errorBox(QCoreApplication::translate("Erebus", "Unexpected Error"), QString::fromUtf8(msg));
    }

    ::qInstallMessageHandler(nullptr);
    g_log = nullptr;

    return EXIT_FAILURE;
}


