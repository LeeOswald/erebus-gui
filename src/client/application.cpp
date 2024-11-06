#include "application.hpp"
#include "appsettings.hpp"
#include "client-version.h"
#include "mainwindow/mainwindow.hpp"
#include "settings.hpp"

#include <erebus-clt/erebus-clt.hxx>
#include <erebus/system/process.hxx>
#include <erebus/system/thread.hxx>
#include <erebus/util/exceptionutil.hxx>

#include <boost/stacktrace.hpp>

#include <filesystem>
#include <sstream>


#if defined(_MSC_VER) && ER_DEBUG
#include <crtdbg.h>
#endif

namespace Erp::Client
{

Application* Application::s_instance = nullptr;


void Application::qtMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    auto localMsg = Erc::toUtf8(msg);
    switch (type)
    {
    case QtDebugMsg:
        Er::Log::debug(Er::Log::defaultLog(), "[Qt] {}", localMsg);
        break;
    case QtInfoMsg:
        Er::Log::info(Er::Log::defaultLog(), "[Qt] {}", localMsg);
        break;
    case QtWarningMsg:
        Er::Log::warning(Er::Log::defaultLog(), "[Qt] {}", localMsg);
        break;
    case QtCriticalMsg:
        Er::Log::error(Er::Log::defaultLog(), "[Qt] {}", localMsg);
        break;
    case QtFatalMsg:
        Er::Log::fatal(Er::Log::defaultLog(), "[Qt] {}", localMsg);
        break;
    }
}

void Application::globalStartup(int argc, char** argv) noexcept
{
#if ER_WINDOWS
    ::SetConsoleOutputCP(CP_UTF8);
#endif

#if ER_DEBUG && defined(_MSC_VER)
    int tmpFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    tmpFlag |= _CRTDBG_LEAK_CHECK_DF;
    _CrtSetDbgFlag(tmpFlag);
#endif

    std::set_terminate(staticTerminateHandler);

    Er::setPrintFailedAssertionFn(staticPrintAssertFn);

    // set current dir the same as exe dir
    {
        std::filesystem::path exe(Er::System::CurrentProcess::exe());
        auto dir = exe.parent_path();
        std::error_code ec;
        std::filesystem::current_path(dir, ec);
    }
}

void Application::globalShutdown() noexcept
{
    Er::setPrintFailedAssertionFn(nullptr);
}

void Application::staticTerminateHandler()
{
    std::ostringstream ss;
    ss << boost::stacktrace::stacktrace();

    auto message = Er::Format::format("std::terminate() called from\n{}", ss.str());
    Er::Log::fatal(Er::Log::defaultLog(), message);
    Erc::Ui::errorBox(QCoreApplication::translate("Erebus", "Unexpected Error"), QString::fromUtf8(message));

    std::abort();
}

void Application::staticPrintAssertFn(std::string_view message)
{
    Er::Log::writeln(Er::Log::defaultLog(), Er::Log::Level::Fatal, std::string(message));
    Erc::Ui::errorBox(QCoreApplication::translate("Erebus", "Assertion failed"), QString::fromUtf8(message));
}

Application::~Application()
{
    s_instance = nullptr;
}

Application::Application(int& argc, char** argv) noexcept
    : QApplication(argc, argv)
{
    Q_ASSERT(!s_instance);
    s_instance = this;
}

bool Application::loadConfiguration(int argc, char** argv) noexcept
{
    try
    {
        m_settings.reset(new Erp::Client::Settings());
        return true;
    }
    catch (std::exception& e)
    {
        qWarning() << "Failed to load configuration: " << e.what() << "\n";
    }

    return false;
}

bool Application::initialize(int argc, char** argv) noexcept
{
    if (!loadConfiguration(argc, argv))
        return false;

    if (!initializeRtl())
        return false;

    ::qInstallMessageHandler(qtMessageHandler);

    if (Er::protectedCall<bool>(
        log(),
        [this]()
        {

            Er::System::CurrentThread::setName("main");
            Er::Client::initialize(log());

            return true;
        }))
    {
        return true;
    }

    // failure
    finalizeRtl();
    return false;
}

void Application::finalize() noexcept
{
    Er::Client::finalize();
    ::qInstallMessageHandler(nullptr);
    finalizeRtl();
}

bool Application::initializeRtl() noexcept
{
    try
    {
        m_logger = Er::Log::makeAsyncLogger();

        auto logLevel = static_cast<Er::Log::Level>(Erc::Option<int>::get(m_settings.get(), Erp::Client::AppSettings::Log::level, int(Erp::Client::AppSettings::Log::defaultLevel)));
        m_logger->setLevel(logLevel);

        addLoggers();

        Er::initialize(m_logger.get());

        return true;
    }
    catch (std::exception& e)
    {
        qWarning() << "Unexpected exception " << e.what() << "\n";
    }

    return false;
}

void Application::addLoggers()
{
#if ER_WINDOWS
    if (::IsDebuggerPresent())
    {
        auto debugger = Er::Log::makeDebuggerSink(
            Er::Log::SimpleFormatter::make({ Er::Log::SimpleFormatter::Option::Time, Er::Log::SimpleFormatter::Option::Level, Er::Log::SimpleFormatter::Option::Tid }),
            Er::Log::SimpleFilter::make(Er::Log::Level::Debug, Er::Log::Level::Fatal)
        );

        m_logger->addSink("debugger", debugger);
    }
#endif
}

void Application::finalizeRtl() noexcept
{
    Er::finalize(m_logger.get());
    m_logger->flush();
    m_logger.reset();
}

int Application::run(int argc, char** argv) noexcept
{
    setApplicationName(EREBUS_APPLICATION_NAME);
    setApplicationVersion(QString("%1.%2.%3").arg(EREBUS_VERSION_MAJOR).arg(EREBUS_VERSION_MINOR).arg(EREBUS_VERSION_PATCH));
    setOrganizationName(EREBUS_ORGANIZATION_NAME);

    int result = EXIT_FAILURE;
    if (!initialize(argc, argv))
        return result;

    Er::protectedCall<void>(
        log(),
        [this, &result]()
        {
            Erp::Client::Ui::MainWindow w(log(), settings());
            result = exec();
        });

    return result;
}

} // namespace Erp::Client {}
