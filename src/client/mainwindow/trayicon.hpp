#pragma once

#include "client-version.h"

#include <erebus-gui/erebus-gui.hpp>

#include <QAction>
#include <QMainWindow>
#include <QMenu>
#include <QSystemTrayIcon>

namespace Erc
{

namespace Private
{

namespace Ui
{


struct TrayIcon final
    : public Er::NonCopyable
{
    explicit TrayIcon(QMainWindow* mainWindow, QAction* quitAction)
        : quitAction(quitAction)
        , restoreAction(new QAction(QObject::tr("Restore"), mainWindow))
        , trayIconMenu(new QMenu(mainWindow))
        , trayIcon(new QSystemTrayIcon(mainWindow))
    {
        trayIconMenu->addAction(restoreAction);
        trayIconMenu->addSeparator();
        trayIconMenu->addAction(quitAction);

        trayIcon->setContextMenu(trayIconMenu);
        trayIcon->setIcon(QIcon(":/images/logo32.png"));
        trayIcon->setToolTip(EREBUS_APPLICATION_NAME);


    }

    QAction* quitAction;
    QAction* restoreAction;
    QMenu* trayIconMenu;
    QSystemTrayIcon* trayIcon;
};


} // namespace Ui {}

} // namespace Private {}

} // namespace Erc {}
