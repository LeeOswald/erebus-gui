#pragma once

#include <erebus-gui/settings.hpp>

namespace Erp
{

namespace ProcessMgr
{

namespace Settings
{

constexpr std::string_view columns("processtab/columns");

constexpr std::string_view autoRefresh("processtab/auto_refresh");

constexpr std::string_view refreshRate("processtab/refresh_rate");
constexpr unsigned RefreshRateDefault = 1000; // 1 sec

constexpr std::string_view trackDuration("processtab/track_duration");
constexpr unsigned TrackDurationDefault = 5000; // 5 sec


} // namespace Settings {}

} // namespace ProcessMgr {}

} // namespace Erp {}
