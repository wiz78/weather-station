#pragma once

namespace weather_station {

// Returns the ventilation advisory state using configurable alert and clear
// thresholds. The lower clear thresholds prevent flicker.
bool update_air_quality_warning(float voc_index, float nox_index,
                                float voc_alert_index, float voc_clear_index,
                                float nox_alert_index, float nox_clear_index);

}  // namespace weather_station
