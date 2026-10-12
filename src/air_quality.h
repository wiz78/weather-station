#pragma once

namespace weather_station {

// Returns the ventilation advisory state. Elevation must persist for five
// samples; clearing uses lower thresholds to prevent flicker.
bool update_air_quality_warning(float voc_index, float nox_index);

}  // namespace weather_station
