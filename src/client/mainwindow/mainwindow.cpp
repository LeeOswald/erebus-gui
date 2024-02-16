#include "../appsettings.hpp"
#include "../connectdlg/connectdlg.hpp"
#include "mainwindow.hpp"

#include <QLocale>


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
    , m_recentEndpoints(Erc::toUtf8(Erc::Option<QString>::get(settings, Erc::Private::AppSettings::Connections::recentConnections, QString())))
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
    , m_statusLabel(new QLabel(m_statusbar))
{
    setObjectName("MainWindow");
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

    m_centralWidget->setObjectName("centralWidget");
    sizePolicy.setHeightForWidth(m_centralWidget->sizePolicy().hasHeightForWidth());
    m_centralWidget->setSizePolicy(sizePolicy);
    setCentralWidget(m_centralWidget);


    m_mainLayout->setSpacing(2);
    m_mainLayout->setObjectName("mainLayout");
    m_mainLayout->setContentsMargins(2, 2, 2, 2);

    m_mainSplitter->setObjectName("mainSplitter");
    sizePolicy.setHeightForWidth(m_mainSplitter->sizePolicy().hasHeightForWidth());
    m_mainSplitter->setSizePolicy(sizePolicy);
    m_mainSplitter->setOrientation(Qt::Vertical);
    m_mainSplitter->setHandleWidth(2);

    m_tabWidget->setObjectName("tabWidget");


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
    connect(m_trayIcon.trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::iconActivated);

    logCtl->unmute();

    m_statusbar->setObjectName("statusBar");
    setStatusBar(m_statusbar);

    m_statusLabel->setText(tr("Processes: 0"));
    m_statusbar->addPermanentWidget(m_statusLabel);

    LogDebug(log, "Client started");

    QTimer::singleShot(50, this, SLOT(initialPrompt()));
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

void MainWindow::initialPrompt()
{
    auto recentUser = Erc::Option<QString>::get(m_settings, Erc::Private::AppSettings::Connections::lastUserName, QString());

    Erc::Private::Ui::ConnectDlg dlg(m_recentEndpoints.all(), Erc::toUtf8(recentUser), this);
    auto result = dlg.exec();
    if (result != QDialog::Accepted)
        return quit();

    auto endpoint = dlg.selected();
    auto user = dlg.user();
    auto password = dlg.password();
}


} // namespace Ui {}

} // namespace Private {}

} // namespace Erc {}
