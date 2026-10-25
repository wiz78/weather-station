#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace weather_station {

// Thirty-one one-minute samples; Theil-Sen median slope resists outliers.
class TrendWindow {
 public:
  void set_minimum_change(const float minimum_change);

  int add(const float value);

  void reset();

 private:
  friend class WeatherData;
  std::array<float, 31> samples_;
  size_t next_;
  size_t count_;
  float minimum_change_;
};

}  // namespace weather_station
