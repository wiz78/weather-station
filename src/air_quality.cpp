#include "air_quality.h"

#include <cmath>
#include <cstdint>

namespace weather_station {
namespace {

constexpr float ALERT_INDEX = 150.0f;
constexpr float CLEAR_INDEX = 120.0f;
constexpr uint8_t SAMPLES_TO_CHANGE = 5;

bool warning_active = false;
uint8_t elevated_samples = 0;
uint8_t clear_samples = 0;

}  // namespace

bool update_air_quality_warning(float voc_index, float nox_index) {
  if (!std::isfinite(voc_index) || !std::isfinite(nox_index)) {
    return warning_active;
  }

  const bool elevated = voc_index >= ALERT_INDEX || nox_index >= ALERT_INDEX;
  const bool clear = voc_index < CLEAR_INDEX && nox_index < CLEAR_INDEX;

  if (!warning_active) {
    clear_samples = 0;
    elevated_samples = elevated ? elevated_samples + 1 : 0;
    if (elevated_samples >= SAMPLES_TO_CHANGE) {
      warning_active = true;
      elevated_samples = 0;
    }
  } else {
    elevated_samples = 0;
    clear_samples = clear ? clear_samples + 1 : 0;
    if (clear_samples >= SAMPLES_TO_CHANGE) {
      warning_active = false;
      clear_samples = 0;
    }
  }

  return warning_active;
}

}  // namespace weather_station
