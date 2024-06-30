#pragma once

#include <erebus/log.hxx>
#include <erebus-clt/erebus-clt.hxx>
#include <erebus-gui/settings.hpp>

#include "endpoints.hpp"
#include "logview.hpp"
#include "mainmenu.hpp"
#include "pluginmgr.hpp"
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

signals:
    void connected(std::shared_ptr<Er::Client::IClient> client, std::string endpoint);
    void disconnected(std::shared_ptr<Er::Client::IClient> client);

public slots:
    void quit();
    void restore();
    void toggleAlwaysOnTop();
    void toggleHideOnClose();
    void iconActivated(QSystemTrayIcon::ActivationReason reason);
    bool promptForConnection();

private slots:
    void start();
    void onConnected(std::shared_ptr<Er::Client::IClient> client, std::string endpoint);
    void onDisconnected(std::shared_ptr<Er::Client::IClient> client);

private:
    void saveGeometry();
    void restoreGeometry();
    void adjustLogViewHeight();
    void refreshTitle();
    bool checkPlugins();
    size_t loadPlugins(const QStringList& paths);

    std::shared_ptr<void> makeChannel(const Er::Client::ChannelParams& params);
    std::shared_ptr<Er::Client::IClient> makeClient(std::shared_ptr<void> channel);

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
    std::shared_ptr<void> m_channel;
    std::shared_ptr<Er::Client::IClient> m_client;
    std::optional<Er::Client::ChannelParams> m_connectionParams;
    Erc::Private::PluginManager m_pluginMgr;
};

} // namespace Ui {}

} // namespace Private {}

} // namespace Erc {}
