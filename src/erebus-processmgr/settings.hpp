#pragma once

#include <erebus-gui/settings.hpp>

namespace Erp
{

namespace Settings
{

constexpr std::string_view columns("processtab/columns");
constexpr std::string_view refreshRate("processtab/refresh_rate");
constexpr unsigned RefreshRateDefault = 1000; // 1 sec

const unsigned MinColumnWidth = 30;
const unsigned MinLabelColumnWidth = 200;

} // namespace Settings {}

} // namespace Erp {}
