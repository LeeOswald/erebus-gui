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

} // Application {}

    
} // namespace AppSettings {}
    
} // namespace Private {}
    
} // namespace Erc {}