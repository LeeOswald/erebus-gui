#pragma once

#include <erebus/log.hxx>
#include <erebus-clt/erebus-clt.hxx>
#include <erebus-gui/settings.hpp>

#include "endpoints.hpp"
#include "logview.hpp"
#include "mainmenu.hpp"
#include "trayicon.hpp"

#include <QLabel>
#include <QMainWindow>
#include <QSplitter>
#include <QStatusBar>
#include <QSystemTrayIcon>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>

namespace Erc
{

namespace Private
{

namespace Ui
{

class MainWindow
    : public QMainWindow
{
    Q_OBJECT

public:
    using Base = QMainWindow;

    ~MainWindow();

    explicit MainWindow(
        Er::Log::ILog* log,
        Er::Log::ILogControl* logCtl,
        Erc::ISettingsStorage* settings,
        QWidget* parent = nullptr
    );

private:
    void changeEvent(QEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

public slots:
    void quit();
    void restore();
    void toggleAlwaysOnTop();
    void toggleHideOnClose();
    void iconActivated(QSystemTrayIcon::ActivationReason reason);
    void initialPrompt();

private:
    void saveGeometry();
    void restoreGeometry();
    void adjustLogViewHeight();

    enum class SplitterPane
    {
        Main = 0,
        Log  = 1
    };

    bool m_exiting = false;
    Erc::Private::RecentEndpoints m_recentEndpoints;
    Er::Log::ILog* m_log;
    Erc::ISettingsStorage* m_settings;
    MainMenu m_mainMenu;
    TrayIcon m_trayIcon;
    QWidget* m_centralWidget;
    QVBoxLayout* m_mainLayout;
    QSplitter* m_mainSplitter;
    LogView* m_logView;
    QTabWidget* m_tabWidget;
    QStatusBar* m_statusbar;
    QLabel* m_statusLabel;
};

} // namespace Ui {}

} // namespace Private {}

} // namespace Erc {}
