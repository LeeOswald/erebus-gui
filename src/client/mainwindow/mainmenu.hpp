#pragma once

#include <erebus/erebus.hxx>

#include <QAction>
#include <QCoreApplication>
#include <QMainWindow>
#include <QMenuBar>

namespace Erc
{

namespace Private
{

namespace Ui
{


struct MainMenu final
    : public Er::NonCopyable
{
    explicit MainMenu(QMainWindow* mainWindow)
        : menuBar(new QMenuBar(mainWindow))
        , menuFile(menuBar->addMenu(QCoreApplication::translate("MainMenu", "File", nullptr)))
        , menuView(menuBar->addMenu(QCoreApplication::translate("MainMenu", "View", nullptr)))
        , actionAlwaysOnTop(new QAction(QCoreApplication::translate("ViewMenu", "Always On Top", nullptr), mainWindow))
        , actionHideOnClose(new QAction(QCoreApplication::translate("ViewMenu", "Hide When Minimized", nullptr), mainWindow))
        , actionLog(new QAction(QCoreApplication::translate("FileMenu", "Log", nullptr), mainWindow))
        , actionExit(new QAction(QCoreApplication::translate("FileMenu", "Exit", nullptr), mainWindow))
    {
        actionAlwaysOnTop->setCheckable(true);
        actionHideOnClose->setCheckable(true);

        menuFile->addAction(actionLog);
        menuFile->addSeparator();
        menuFile->addAction(actionExit);

        menuView->addAction(actionAlwaysOnTop);
        menuView->addAction(actionHideOnClose);

    }

    QMenuBar* menuBar;
    QMenu* menuFile;
    QMenu* menuView;
    QAction* actionAlwaysOnTop;
    QAction* actionHideOnClose;
    QAction* actionLog;
    QAction* actionExit;
};

} // namespace Ui {}

} // namespace Private {}

} // namespace Erc {}
