#pragma once

#include <erebus/erebus.hxx>

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

EREBUSGUI_EXPORT void errorBox(const QString& title, const QString& message, QWidget* parent = nullptr);

} // namespace Ui {}
    
} // namespace Erc {}
