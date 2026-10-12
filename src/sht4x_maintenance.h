#pragma once

#include "esphome/components/sht4x/sht4x.h"

namespace weather_station {

// Run one SHT4x high-power, 1-second heater command through the configured
// sensor component, pausing polling and allowing a 60-second cooldown.
bool start_sht4x_maintenance(esphome::sht4x::SHT4XComponent *sensor);

}  // namespace weather_station
