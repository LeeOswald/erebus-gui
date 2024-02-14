#include "client-version.h"

#include "settings.hpp"

#include <QCoreApplication>
#include <QMessageBox>

#if defined(_MSC_VER) && ER_DEBUG
    #include <crtdbg.h>
#endif



int main(int argc, char *argv[])
{
#if defined(_MSC_VER) && ER_DEBUG
    int tmpFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    tmpFlag |= _CRTDBG_LEAK_CHECK_DF;
    _CrtSetDbgFlag(tmpFlag);
#endif

    QCoreApplication a(argc, argv);
    a.setApplicationName(EREBUS_APPLICATION_NAME);
    a.setApplicationVersion(QString("%1.%2.%3").arg(EREBUS_VERSION_MAJOR).arg(EREBUS_VERSION_MINOR).arg(EREBUS_VERSION_PATCH));
    a.setOrganizationName(EREBUS_ORGANIZATION_NAME);
    
    try
    {

        return a.exec();
    }
    catch (std::exception& e)
    {
        QMessageBox::critical(nullptr, QString::fromLocal8Bit(EREBUS_APPLICATION_NAME), QLatin1String("Unexpected error"), QMessageBox::Ok);
    }

    return EXIT_FAILURE;
}


