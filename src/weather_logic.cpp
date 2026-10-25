#include "weather_logic.h"
#include "weather_data.h"

#include <esp_attr.h>

#include <cmath>

namespace weather_station {

  namespace {
    RTC_NOINIT_ATTR WeatherData data;
  }  // namespace

  void init_weather_data() {
    data.init();
  }

  void save_weather_data_backup() {
    data.save_backup();
  }

  int sample_temperature_trend(const float value) {
    return data.temperatureWindow.add(value);
  }

  int sample_humidity_trend(const float value) {
    return data.humidityWindow.add(value);
  }

  void record_weather_history(const float outside_temp, const float outside_humidity,
                              const float inside_temp, const float inside_humidity) {
    data.weatherHistory.record(outside_temp, outside_humidity, inside_temp, inside_humidity);
  }

  float history_minimum(const int series) {
    return data.weatherHistory.extreme(series, true);
  }

  float history_maximum(const int series) {
    return data.weatherHistory.extreme(series, false);
  }

  int history_point_x(const int point, const int width) {
    return data.weatherHistory.point_x(point, width);
  }

  int history_point_y(const int series, const int point, const int height) {
    return data.weatherHistory.point_y(series, point, height);
  }

  void check_ice_alert(const float outside_temp, esphome::light::LightState *led,
                       const bool disabled) {
    if (disabled) {
      auto call = led->turn_off();
      call.perform();
      return;
    }
    if (std::isnan(outside_temp)) {
      return;
    }

    if (outside_temp < 4.0) {
      // Ice alert! Start the blinking effect defined in the YAML.
      // We set the effect by passing the exact name string we defined in hw_official_build.yaml
      auto call = led->turn_on();
      call.set_effect("Blink");
      call.perform();
    } else {
      // Safe temperature, turn off the LED (this also cancels any running effect).
      auto call = led->turn_off();
      call.perform();
    }
  }

} // namespace weather_station
