#pragma once

#include <erebus/log.hxx>
#include <erebus-gui/settings.hpp>

#include <QAction>
#include <QActionGroup>
#include <QCoreApplication>
#include <QMainWindow>
#include <QMenu>
#include <QPlainTextEdit>



namespace Erc
{
    
namespace Private
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
        QWidget* parent
    );

    QWidget* widget() const noexcept;
    QAction* action() const noexcept;

    void retranslateUi();

signals:
    void logLevelChanged(Er::Log::Level prev, Er::Log::Level now);

public slots:
    void log(QString text);
    void setLogLevel(QAction* action);
    void clearLog();

private:
    void logDelegate(std::shared_ptr<Er::Log::Record> r);

    Er::Log::ILog* m_log;
    Er::Log::ILogControl* m_logCtl;
    Erc::ISettingsStorage* m_settings;
    QPlainTextEdit* m_view;
    QMenu* m_menu;
    QActionGroup* m_actionGroup;
    QAction* m_actionLog;
    QAction* m_actionDebug;
    QAction* m_actionInfo;
    QAction* m_actionWarning;
    QAction* m_actionError;
    QAction* m_actionFatal;
    QAction* m_actionOff;
    QAction* m_actionClear;

};

} // namespace Ui {}

} // namespace Private {}

} // namespace Erc {}
