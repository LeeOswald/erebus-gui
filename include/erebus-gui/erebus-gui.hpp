#pragma once

#include <erebus/erebus.hxx>

#include <QString>

#if defined(_WIN32) || defined(__CYGWIN__)
    #ifdef ERC_EXPORTS
        #define ERC_EXPORT __declspec(dllexport)
    #else
        #define ERC_EXPORT __declspec(dllimport)
    #endif
#else
    #define ERC_EXPORT __attribute__((visibility("default")))
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

    
} // namespace Erc {}