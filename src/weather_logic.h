#pragma once

#include "esphome.h"

namespace weather_station {

  void init_weather_data();
  void save_weather_data_backup();

  // Simple function to blink the ice alert LED if temperature drops below 4C
  // This is called from the `on_value` trigger of the outside temp sensor.
  void check_ice_alert(float outside_temp, esphome::light::LightState *led,
                       bool disabled);

  // Trend estimators return -1 for falling, 0 for stable/insufficient history,
  // and 1 for rising. Call once per minute to maintain the rolling window.
  int sample_temperature_trend(float value);
  int sample_humidity_trend(float value);

  // Four 24-hour series, sampled every five minutes into a volatile ring
  // buffer. Series order: outside temperature/humidity, indoor temperature/humidity.
  void record_weather_history(float outside_temp, float outside_humidity,
                              float inside_temp, float inside_humidity);
  float history_minimum(int series);
  float history_maximum(int series);
  int history_point_x(int point, int width);
  int history_point_y(int series, int point, int height);

} // namespace weather_station
