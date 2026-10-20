#include "air_quality.h"

#include <cmath>

namespace weather_station {
namespace {

bool warning_active = false;

}  // namespace

bool update_air_quality_warning(float voc_index, float nox_index,
                                float voc_alert_index, float voc_clear_index,
                                float nox_alert_index, float nox_clear_index) {
  if (!std::isfinite(voc_index) || !std::isfinite(nox_index)) {
    return warning_active;
  }

  const bool elevated = voc_index >= voc_alert_index || nox_index >= nox_alert_index;
  const bool clear = voc_index < voc_clear_index && nox_index < nox_clear_index;

  if (!warning_active && elevated)
    warning_active = true;
  else if (warning_active && clear)
    warning_active = false;

  return warning_active;
}

}  // namespace weather_station
