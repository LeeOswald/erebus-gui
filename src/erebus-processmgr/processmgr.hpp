#pragma once

#include <erebus-gui/plugin.hpp>


#if defined(_WIN32) || defined(__CYGWIN__)
    #ifdef EREBUSPROCMGR_EXPORTS
        #define EREBUSPROCMGR_EXPORT __declspec(dllexport)
    #else
        #define EREBUSPROCMGR_EXPORT __declspec(dllimport)
    #endif
#else
    #define EREBUSPROCMGR_EXPORT __attribute__((visibility("default")))
#endif


extern "C"
{

EREBUSPROCMGR_EXPORT Erc::IPlugin* createUiPlugin(const Erc::PluginParams&);
EREBUSPROCMGR_EXPORT void disposeUiPlugin(Erc::IPlugin*);



} // extern "C" {}
