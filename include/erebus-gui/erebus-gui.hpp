#pragma once

#include <erebus/erebus.hxx>


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
    
    
} // namespace Erc {}