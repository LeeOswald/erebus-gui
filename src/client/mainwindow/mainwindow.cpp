#include "client-version.h"

#include <erebus/util/exceptionutil.hxx>
#include <erebus/util/file.hxx>
#include <erebus-gui/waitcursor.hpp>

#include "../appsettings.hpp"
#include "../connectdlg/connectdlg.hpp"
#include "../plugindlg/plugindlg.hpp"
#include "mainwindow.hpp"
#include "pluginlist.hpp"

#include <QLocale>
#include <QMessageBox>


namespace Erc
{

namespace Private
{

namespace Ui
{

MainWindow::~MainWindow()
{

}

MainWindow::MainWindow(
    Er::Log::ILog* log,
    Er::Log::ILogControl* logCtl,
    Erc::ISettingsStorage* settings,
    QWidget* parent
    )
    : Base(parent)
    , m_recentEndpoints(Erc::toUtf8(Erc::Option<QString>::get(settings, Erc::Private::AppSettings::Connections::recentConnections, QString())), Erc::Private::AppSettings::Connections::kMaxRecentConnections)
    , m_log(log)
    , m_settings(settings)
    , m_mainMenu(this)
    , m_trayIcon(this, m_mainMenu.actionExit)
    , m_centralWidget(new QWidget(this))
    , m_mainLayout(new QVBoxLayout(m_centralWidget))
    , m_mainSplitter(new QSplitter(m_centralWidget))
    , m_logView(new LogView(log, logCtl, settings, this, m_mainSplitter, m_mainMenu.actionLog))
    , m_tabWidget(new QTabWidget(m_mainSplitter))
    , m_statusbar(new QStatusBar(this))
    , m_pluginMgr(Erc::PluginParams(settings, log, m_tabWidget, m_mainMenu.menuBar, m_statusbar))
{
    resize(1024, 768);
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(this->sizePolicy().hasHeightForWidth());
    setSizePolicy(sizePolicy);
    setMinimumSize(QSize(1024, 768));
    QIcon icon;
    icon.addFile(QString::fromUtf8(":/images/logo32.png"), QSize(), QIcon::Normal, QIcon::Off);
    setWindowIcon(icon);
    setLocale(QLocale(QLocale::English, QLocale::UnitedStates));

    setMenuBar(m_mainMenu.menuBar);

    sizePolicy.setHeightForWidth(m_centralWidget->sizePolicy().hasHeightForWidth());
    m_centralWidget->setSizePolicy(sizePolicy);
    setCentralWidget(m_centralWidget);

    m_mainLayout->setSpacing(2);
    m_mainLayout->setContentsMargins(2, 2, 2, 2);

    sizePolicy.setHeightForWidth(m_mainSplitter->sizePolicy().hasHeightForWidth());
    m_mainSplitter->setSizePolicy(sizePolicy);
    m_mainSplitter->setOrientation(Qt::Vertical);
    m_mainSplitter->setHandleWidth(2);

    m_mainSplitter->addWidget(m_tabWidget);
    m_mainSplitter->addWidget(m_logView->view());

    m_mainSplitter->setStretchFactor(int(SplitterPane::Main), 1);
    m_mainSplitter->setStretchFactor(int(SplitterPane::Log), 0);

    m_mainLayout->addWidget(m_mainSplitter);

    QObject::connect(m_mainMenu.actionExit, SIGNAL(triggered()), this, SLOT(quit()));
    QObject::connect(m_mainMenu.actionAlwaysOnTop, SIGNAL(triggered()), this, SLOT(toggleAlwaysOnTop()));
    QObject::connect(m_mainMenu.actionHideOnClose, SIGNAL(triggered()), this, SLOT(toggleHideOnClose()));

    restoreGeometry();

    if (Erc::Option<bool>::get(m_settings, Erc::Private::AppSettings::MainWindow::alwaysOnTop, false))
    {
        setWindowFlag(Qt::WindowStaysOnTopHint, true);
        m_mainMenu.actionAlwaysOnTop->setChecked(true);
    }

    if (Erc::Option<bool>::get(m_settings, Erc::Private::AppSettings::MainWindow::hideOnClose, Erc::Private::AppSettings::MainWindow::hideOnCloseDefault))
    {
        m_mainMenu.actionHideOnClose->setChecked(true);
    }

    auto startHidden = Erc::Option<bool>::get(m_settings, Erc::Private::AppSettings::MainWindow::startHidden, Erc::Private::AppSettings::MainWindow::startHiddenDefault);
    if (!startHidden)
        show();

    m_logView->view()->setMaximumBlockCount(1000000);
    if (log->level() < Er::Log::Level::Off)
    {
        m_logView->view()->setVisible(true);
        adjustLogViewHeight();
    }
    else
    {
        m_logView->view()->setVisible(false);
    }

    m_trayIcon.trayIcon->show();
    QObject::connect(m_trayIcon.trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::iconActivated);

    logCtl->unmute();

    m_statusbar->setObjectName("statusBar");
    setStatusBar(m_statusbar);

    QObject::connect(this, SIGNAL(connected(std::shared_ptr<Er::Client::IClient>,std::string)), this, SLOT(onConnected(std::shared_ptr<Er::Client::IClient>,std::string)));
    QObject::connect(this, SIGNAL(disconnected(Er::Client::IClient*)), this, SLOT(onDisconnected(Er::Client::IClient*)));

    refreshTitle();
    ErLogDebug(log, ErLogNowhere(), "Client started");

    QTimer::singleShot(0, this, SLOT(start()));
}

void MainWindow::quit()
{
    m_exiting = true;
    m_trayIcon.trayIcon->hide();

    qApp->quit();
}

void MainWindow::toggleAlwaysOnTop()
{
    if (windowFlags() & Qt::WindowStaysOnTopHint)
    {
        setWindowFlag(Qt::WindowStaysOnTopHint, false);
        m_mainMenu.actionAlwaysOnTop->setChecked(false);
        Erc::Option<bool>::set(m_settings, Erc::Private::AppSettings::MainWindow::alwaysOnTop, false);
    }
    else
    {
        setWindowFlag(Qt::WindowStaysOnTopHint, true);
        m_mainMenu.actionAlwaysOnTop->setChecked(true);
        Erc::Option<bool>::set(m_settings, Erc::Private::AppSettings::MainWindow::alwaysOnTop, true);
    }

    show();
}

void MainWindow::toggleHideOnClose()
{
    auto hide = Erc::Option<bool>::get(m_settings, Erc::Private::AppSettings::MainWindow::hideOnClose, Erc::Private::AppSettings::MainWindow::hideOnCloseDefault);
    if (hide)
    {
        m_mainMenu.actionHideOnClose->setChecked(false);
        Erc::Option<bool>::set(m_settings, Erc::Private::AppSettings::MainWindow::hideOnClose, false);
    }
    else
    {
        m_mainMenu.actionHideOnClose->setChecked(true);
        Erc::Option<bool>::set(m_settings, Erc::Private::AppSettings::MainWindow::hideOnClose, true);
    }
}

void MainWindow::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason)
    {
    case QSystemTrayIcon::Trigger:
    case QSystemTrayIcon::DoubleClick:
        restore();
        break;

    default:
        break;
    }
}

void MainWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::WindowStateChange)
    {
        if (isMinimized())
        {
            if (Erc::Option<bool>::get(m_settings, Erc::Private::AppSettings::MainWindow::hideOnClose, Erc::Private::AppSettings::MainWindow::hideOnCloseDefault))
            {
                hide();
                event->ignore();
            }
        }
        else
        {
            activateWindow();
        }
    }
    else
    {
        Base::changeEvent(event);
    }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (Erc::Option<bool>::get(m_settings, Erc::Private::AppSettings::MainWindow::hideOnClose, Erc::Private::AppSettings::MainWindow::hideOnCloseDefault) && !m_exiting)
    {
        hide();
        event->ignore();
    }
    else
    {
        saveGeometry();

        Base::closeEvent(event);
    }
}

void MainWindow::restore()
{
    show();
    setWindowState(windowState() & ~Qt::WindowMinimized | Qt::WindowActive);
}

void MainWindow::saveGeometry()
{
    Erc::Option<QByteArray>::set(m_settings, Erc::Private::AppSettings::MainWindow::geometry, Base::saveGeometry());
    Erc::Option<QByteArray>::set(m_settings, Erc::Private::AppSettings::MainWindow::state, Base::saveState());
    Erc::Option<bool>::set(m_settings, Erc::Private::AppSettings::MainWindow::startHidden, isHidden());

    auto splitterSizes = m_mainSplitter->sizes();
    Erc::Option<int>::set(m_settings, Erc::Private::AppSettings::MainWindow::mainPos, splitterSizes[int(SplitterPane::Main)]);
    Erc::Option<int>::set(m_settings, Erc::Private::AppSettings::MainWindow::logPos, splitterSizes[int(SplitterPane::Log)]);
}

void MainWindow::restoreGeometry()
{
    Base::restoreGeometry(Erc::Option<QByteArray>::get(m_settings, Erc::Private::AppSettings::MainWindow::geometry, QByteArray()));
    restoreState(Erc::Option<QByteArray>::get(m_settings, Erc::Private::AppSettings::MainWindow::state, QByteArray()));

    static_assert(int(SplitterPane::Main) == 0);
    static_assert(int(SplitterPane::Log) == 1);

    QList<int> splitterSizes;
    splitterSizes.push_back(Erc::Option<int>::get(m_settings, Erc::Private::AppSettings::MainWindow::mainPos, 100));
    splitterSizes.push_back(Erc::Option<int>::get(m_settings, Erc::Private::AppSettings::MainWindow::logPos, Erc::Private::AppSettings::MainWindow::kMinLogViewHeight));

    if (splitterSizes[int(SplitterPane::Main)] + splitterSizes[int(SplitterPane::Log)] > 0)
        m_mainSplitter->setSizes(splitterSizes);
}

void MainWindow::adjustLogViewHeight()
{
    auto splitterSizes = m_mainSplitter->sizes();
    if (splitterSizes[int(SplitterPane::Log)] <= 1)
    {
        splitterSizes[int(SplitterPane::Main)] -= Erc::Private::AppSettings::MainWindow::kMinLogViewHeight;
        splitterSizes[int(SplitterPane::Log)] += Erc::Private::AppSettings::MainWindow::kMinLogViewHeight;
    }

    m_mainSplitter->setSizes(splitterSizes);
}

void MainWindow::start()
{
    if (!checkPlugins())
        quit();

    if (!promptForConnection())
        quit();

}

bool MainWindow::promptForConnection()
{

    std::shared_ptr<Er::Client::IClient> client;
    do
    {
        auto ssl = Erc::Option<bool>::get(m_settings, Erc::Private::AppSettings::Connections::lastUseSsl, false);
        auto recentCert = Erc::Option<QString>::get(m_settings, Erc::Private::AppSettings::Connections::lastCertificate, QString());
        auto recentKey = Erc::Option<QString>::get(m_settings, Erc::Private::AppSettings::Connections::lastKey, QString());
        auto rootCA = Erc::Option<QString>::get(m_settings, Erc::Private::AppSettings::Connections::lastRootCA, QString());

        Erc::Private::Ui::ConnectDlg dlg(m_recentEndpoints.all(), ssl, Erc::toUtf8(rootCA), Erc::toUtf8(recentCert), Erc::toUtf8(recentKey), this);
        if (dlg.exec() != QDialog::Accepted)
            return false;

        std::string caCertificate;
        if (dlg.ssl() && !dlg.rootCA().empty())
        {
            caCertificate = Erc::Ui::protectedCall<std::string>(
                m_log,
                tr("Failed to load the CA certificate"),
                this,
                [this](const std::string& path)
                {
                    return Er::Util::loadTextFile(path);
                },
                dlg.rootCA()
            );
        }

        std::string certificate;
        if (dlg.ssl() && !dlg.certificate().empty())
        {
            certificate = Erc::Ui::protectedCall<std::string>(
                m_log,
                tr("Failed to load the certificate"),
                this,
                [this](const std::string& path)
                {
                    return Er::Util::loadTextFile(path);
                },
                dlg.certificate()
                );
        }

        std::string key;
        if (dlg.ssl() && !dlg.key().empty())
        {
            key = Erc::Ui::protectedCall<std::string>(
                m_log,
                tr("Failed to load the certificate key"),
                this,
                [this](const std::string& path)
                {
                    return Er::Util::loadTextFile(path);
                },
                dlg.key()
                );
        }

        Er::Log::Info(m_log) << "Connecting to [" << dlg.selected() << "]";

        Er::Client::Params params(m_log, dlg.selected(), dlg.ssl(), caCertificate, certificate, key);
        client = makeClient(params);

        if (client)
        {
            // save the successful connection params
            m_recentEndpoints.promote(dlg.selected());
            auto packed = Erc::fromUtf8(m_recentEndpoints.pack());
            Erc::Option<QString>::set(m_settings, Erc::Private::AppSettings::Connections::recentConnections, packed);

            Erc::Option<bool>::set(m_settings, Erc::Private::AppSettings::Connections::lastUseSsl, dlg.ssl());

            if (!dlg.certificate().empty())
                Erc::Option<QString>::set(m_settings, Erc::Private::AppSettings::Connections::lastCertificate, Erc::fromUtf8(dlg.certificate()));

            if (!dlg.key().empty())
                Erc::Option<QString>::set(m_settings, Erc::Private::AppSettings::Connections::lastKey, Erc::fromUtf8(dlg.key()));

            if (!dlg.rootCA().empty())
                Erc::Option<QString>::set(m_settings, Erc::Private::AppSettings::Connections::lastRootCA, Erc::fromUtf8(dlg.rootCA()));

            auto oldClient = m_client;
            m_client.reset();

            // we're temporarily disconnected
            refreshTitle();

            if (oldClient)
                emit disconnected(oldClient);

            m_client = client;
            m_connectionParams = params;
        }

    } while (!client);

    emit connected(client, m_connectionParams->endpoint);

    return true;
}

std::shared_ptr<Er::Client::IClient> MainWindow::makeClient(const Er::Client::Params& params)
{
    auto client = Erc::Ui::protectedCall<std::shared_ptr<Er::Client::IClient>>(
        m_log,
        tr("Failed to create a client instance"),
        this,
        [this](const Er::Client::Params& params)
        {
            Erc::Ui::WaitCursorScope w;
            return Er::Client::create(params);
        },
        params
    );

    return client;
}

void MainWindow::onConnected(std::shared_ptr<Er::Client::IClient> client, std::string endpoint)
{
    size_t connected = 0;

    {
        Erc::Ui::WaitCursorScope w;

        m_pluginMgr.forEachPlugin(
            [this, client, endpoint, &connected](Erc::IPlugin* plugin)
            {
                auto pi = plugin->info();
                Er::Log::Info(m_log) << "Connecting for plugin " << pi.name;

                auto ok = Er::protectedCall<bool>(
                    m_log,
                    ErLogNowhere(),
                    [this, plugin, client, endpoint]()
                    {
                        plugin->addConnection(client.get(), endpoint);
                        return true;
                    }
                );

                if (ok)
                    ++connected;
            }
        );

    }
    if (connected == 0)
    {
        Erc::Ui::errorBoxLite(QString::fromUtf8(EREBUS_APPLICATION_NAME), tr("Couuld not connect to %1").arg(QString::fromUtf8(endpoint)), this);
        QTimer::singleShot(0, [this]() { promptForConnection(); });
    }
    else
    {
        refreshTitle();
    }
}

void MainWindow::onDisconnected(std::shared_ptr<Er::Client::IClient> client)
{
    m_pluginMgr.forEachPlugin(
        [this, client](Erc::IPlugin* plugin)
        {
            plugin->removeConnection(client.get());
        }
    );
}

void MainWindow::refreshTitle()
{
    if (!m_client)
    {
        setWindowTitle(QLatin1String(EREBUS_APPLICATION_NAME));
        m_trayIcon.setToolTip(QLatin1String(EREBUS_APPLICATION_NAME));
    }
    else
    {
        Q_ASSERT(m_connectionParams);
        QString connection = Erc::fromUtf8(m_connectionParams->endpoint);

        setWindowTitle(connection);
        m_trayIcon.setToolTip(connection);
    }
}

size_t MainWindow::loadPlugins(const QStringList& paths)
{
    size_t count = 0;

    for (auto& path: paths)
    {
        if (!m_pluginMgr.exists(path))
        {
            auto plugin =
                Erc::Ui::protectedCall<IPlugin*>(
                    m_log,
                    tr("Failed to load plugin"),
                    this,
                    [this](const QString& path)
                    {
                        Erc::Ui::WaitCursorScope w;
                        return m_pluginMgr.load(path);
                    },
                    path
                );
        }

    }
    return count;
}

bool MainWindow::checkPlugins()
{
    while (!m_pluginMgr.count())
    {
        auto pluginPathsPacked = Erc::Option<QString>::get(m_settings, Erc::Private::AppSettings::Application::pluginList, QString());
        Erc::Private::PluginList pl(pluginPathsPacked);

        loadPlugins(pl.all());

        if (!m_pluginMgr.count())
        {
            auto exeDir = qApp->applicationDirPath();

            // ask for plugins
            PluginDlg d(pl.all(), Erc::Option<QString>::get(m_settings, Erc::Private::AppSettings::Application::lastPluginDir, exeDir), this);
            if (d.exec() != QDialog::Accepted)
                break;

            if (!d.plugins().isEmpty())
            {
                Erc::Option<QString>::set(m_settings, Erc::Private::AppSettings::Application::lastPluginDir, d.pluginDir());

                pl = Erc::Private::PluginList(d.plugins());
                Erc::Option<QString>::set(m_settings, Erc::Private::AppSettings::Application::pluginList, pl.pack());
            }
        }
    }

    return (m_pluginMgr.count() > 0);
}

} // namespace Ui {}

} // namespace Private {}

} // namespace Erc {}
