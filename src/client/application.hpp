#pragma once

#include <erebus/log.hxx>

#include "settings.hpp"

#include <QApplication>


namespace Erp::Client
{

class Application final
    : public QApplication
{
    Q_OBJECT

public:
    static void globalStartup(int argc, char** argv) noexcept;
    static void globalShutdown() noexcept;

    ~Application();
    Application(int& argc, char** argv) noexcept;

    static Application& instance() noexcept
    {
        Q_ASSERT(s_instance);
        return *s_instance;
    }

    Erc::ISettingsStorage* settings() noexcept
    {
        Q_ASSERT(m_settings);
        return m_settings.get();
    }

    Er::Log::ILog* log() noexcept
    {
        Q_ASSERT(m_logger);
        return m_logger.get();
    }

    int run(int argc, char** argv) noexcept;

private:
    static void staticTerminateHandler();
    static void staticPrintAssertFn(std::string_view message);
    static void qtMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);

    bool initialize(int argc, char** argv) noexcept;
    void finalize() noexcept;
    bool initializeRtl() noexcept;
    void finalizeRtl() noexcept;
    bool loadConfiguration(int argc, char** argv) noexcept;
    void addLoggers();

    static Application* s_instance;

    std::unique_ptr<Settings> m_settings;
    Er::Log::ILog::Ptr m_logger;
};

} // namespace Erp::Client {}
