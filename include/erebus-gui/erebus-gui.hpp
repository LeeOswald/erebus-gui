#pragma once

#include <erebus/erebus.hxx>
#include <erebus/log.hxx>
#include <erebus/util/exceptionutil.hxx>


#include <QMessageBox>
#include <QString>
#include <QWidget>

#if defined(_WIN32) || defined(__CYGWIN__)
    #ifdef EREBUSGUI_EXPORTS
        #define EREBUSGUI_EXPORT __declspec(dllexport)
    #else
        #define EREBUSGUI_EXPORT __declspec(dllimport)
    #endif
#else
    #define EREBUSGUI_EXPORT __attribute__((visibility("default")))
#endif


namespace Erc
{
    
inline std::string toUtf8(const QString& s)
{
    return std::string(s.toUtf8().constData());
}

inline QString fromUtf8(const std::string& s)
{
    return QString::fromUtf8(s.data(), s.length());
}


namespace Ui
{

EREBUSGUI_EXPORT void errorBoxLite(const QString& title, const QString& message, QWidget* parent = nullptr);
EREBUSGUI_EXPORT void errorBox(const QString& title, const QString& message, QWidget* parent = nullptr);


template <typename ResultT, typename ParentT, typename WorkT, typename... Args>
ResultT protectedCall(Er::Log::ILog* log, const QString& title, ParentT* parent, WorkT work, Args&&... args)
{
    try
    {
        return work(std::forward<Args>(args)...);
    }
    catch (Er::Exception& e)
    {
        if (log)
        {
            auto msg = Er::Util::formatException(e);
            log->write(Er::Log::Level::Error, ErLogNowhere(), "%s", msg.c_str());
        }

        Erc::Ui::errorBoxLite(title, QString::fromUtf8(e.what()), parent);
    }
    catch (std::exception& e)
    {
        if (log)
        {
            auto msg = Er::Util::formatException(e);
            log->write(Er::Log::Level::Error, ErLogNowhere(), "%s", msg.c_str());
        }

        Erc::Ui::errorBoxLite(title, QString::fromUtf8(e.what()), parent);
    }
    catch (...)
    {
        if (log)
            log->write(Er::Log::Level::Error, ErLogNowhere(), "Unexpected exception");

        QMessageBox::critical(parent, title, QObject::tr("Unexpected exception"));
    }

    return ResultT();
}

} // namespace Ui {}
    
} // namespace Erc {}
