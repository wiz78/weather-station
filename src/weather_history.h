#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace weather_station {

class WeatherHistory {
 public:
  static constexpr size_t CAPACITY = 288;  // 24 hours at 5-minute intervals
  static constexpr uint32_t SAMPLE_INTERVAL_MS = 5UL * 60UL * 1000UL;

  void record(const float outside_temp, const float outside_humidity, const float inside_temp,
              const float inside_humidity);

  float extreme(int series, bool minimum) const;

  int point_x(int point, const int width) const;

  int point_y(const int series, int point, const int height) const;

  void reset();

  void resetTimestamp();

 private:
  friend class WeatherData;
  std::array<std::array<float, CAPACITY>, 4> values_;
  size_t next_;
  size_t count_;
  uint32_t last_sample_ms_;
  bool has_sampled_;
};

}  // namespace weather_station
