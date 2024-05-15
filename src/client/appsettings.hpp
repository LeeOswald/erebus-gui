#pragma once

#include "settings.hpp"

#include <erebus/log.hxx>

namespace Erc
{
    
namespace Private
{
    
namespace AppSettings
{
    
namespace Log
{

constexpr std::string_view level("logger/mode");
constexpr Er::Log::Level defaultLevel = Er::Log::Level::Debug;


} // namespace Log {}

namespace Application
{

constexpr std::string_view singleInstance("application/allow_only_one_instance");
constexpr bool singleInstanceDefault = true;

constexpr std::string_view lastPluginDir("application/last_plugin_dir");
constexpr std::string_view pluginList("application/plugin_list");

} // Application {}

namespace MainWindow
{

constexpr std::string_view alwaysOnTop("mainwindow/always_on_top");
constexpr bool alwaysOnTopDefault = false;

constexpr std::string_view startHidden("mainwindow/start_hidden");
constexpr bool startHiddenDefault = false;

constexpr std::string_view hideOnClose("mainwindow/hide_on_close");
constexpr bool hideOnCloseDefault = false;

constexpr std::string_view geometry("mainwindow/geometry");
constexpr std::string_view state("mainwindow/state");

const int kMinLogViewHeight = 200;
constexpr std::string_view logPos("mainwindow/log_pos");
constexpr std::string_view mainPos("mainwindow/main_pos");


} // namespace MainWindow {}

namespace Connections
{

constexpr std::string_view recentConnections("connections/recent_connections");
constexpr std::string_view lastUseSsl("connections/ssl");
constexpr std::string_view lastRootCA("connections/rootCA");
constexpr std::string_view lastCertificate("connections/certificate");
constexpr std::string_view lastKey("connections/key");
constexpr size_t kMaxRecentConnections = 16;

} // namespace Connections {}

    
} // namespace AppSettings {}
    
} // namespace Private {}
    
} // namespace Erc {}
