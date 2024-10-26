#pragma once

#include <erebus/log.hxx>
#include <erebus-gui/settings.hpp>

#include <QAction>
#include <QActionGroup>
#include <QCoreApplication>
#include <QMainWindow>
#include <QMenu>
#include <QPlainTextEdit>



namespace Erp
{
    
namespace Client
{

namespace Ui
{

class LogView final
    : public QObject
{
    Q_OBJECT

public:
    ~LogView();

    explicit LogView(
        Er::Log::ILog* log,
        Erc::ISettingsStorage* settings,
        QMainWindow* mainWindow,
        QWidget* parent,
        QAction* actionLog
    );

    QPlainTextEdit* view() noexcept
    {
        return m_view;
    }

signals:
    void logLevelChanged(Er::Log::Level prev, Er::Log::Level now);

public slots:
    void log(QString text);
    void setLogLevel(QAction* action);
    void clearLog();

private:
    struct LogSink
        : public Er::Log::ISink
    {
        LogSink(LogView* owner, Er::Log::IFormatter::Ptr formatter)
            : owner(owner)
            , formatter(formatter)
        {}

        void write(Er::Log::Record::Ptr r) override;
        void flush() override;

    private:
        LogView* owner;
        Er::Log::IFormatter::Ptr formatter;
    };

    Er::Log::ILog* m_log;
    Erc::ISettingsStorage* m_settings;
    QPlainTextEdit* m_view;
    QMenu* m_menu;
    QActionGroup* m_actionGroup;
    QAction* m_actionDebug;
    QAction* m_actionInfo;
    QAction* m_actionWarning;
    QAction* m_actionError;
    QAction* m_actionFatal;
    QAction* m_actionOff;
    QAction* m_actionClear;

};

} // namespace Ui {}

} // namespace Client {}

} // namespace Erp {}
