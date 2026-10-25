#include "weather_history.h"

#include <algorithm>
#include <cmath>
#include <ranges>

#include "esphome/core/hal.h"

namespace weather_station {

void WeatherHistory::record(const float outside_temp, const float outside_humidity, const float inside_temp,
                            const float inside_humidity) {
  const uint32_t now = esphome::millis();
  if (has_sampled_ && static_cast<uint32_t>(now - last_sample_ms_) < SAMPLE_INTERVAL_MS) {
    return;
  }
  last_sample_ms_ = now;
  has_sampled_ = true;
  values_[0][next_] = outside_temp;
  values_[1][next_] = outside_humidity;
  values_[2][next_] = inside_temp;
  values_[3][next_] = inside_humidity;
  next_ = (next_ + 1) % CAPACITY;
  if (count_ < CAPACITY) {
    count_++;
  }
}

float WeatherHistory::extreme(int series, bool minimum) const {
  if (series < 0 || series >= 4 || count_ == 0) {
    return NAN;
  }
  float result = minimum ? INFINITY : -INFINITY;
  bool found = false;
  const size_t oldest = count_ == CAPACITY ? next_ : 0;
  for (size_t i = 0; i < count_; i++) {
    const float value = values_[series][(oldest + i) % CAPACITY];
    if (!std::isfinite(value)) {
      continue;
    }
    result = minimum ? std::min(result, value) : std::max(result, value);
    found = true;
  }
  return found ? result : NAN;
}

int WeatherHistory::point_x(int point, const int width) const {
  if (width <= 1) return 0;
  point = std::clamp(point, 0, 24);
  return point * (width - 1) / 24;
}

int WeatherHistory::point_y(const int series, int point, const int height) const {
  if (height <= 1 || series < 0 || series >= 4 || count_ == 0) {
    return height > 1 ? height / 2 : 0;
  }
  point = std::clamp(point, 0, 24);
  const size_t oldest = count_ == CAPACITY ? next_ : 0;
  const size_t sample_index = (static_cast<size_t>(point) * (count_ - 1) + 12) / 24;
  const float value = values_[series][(oldest + sample_index) % CAPACITY];
  const float minimum = extreme(series, true);
  const float maximum = extreme(series, false);
  if (!std::isfinite(value) || !std::isfinite(minimum) || !std::isfinite(maximum) ||
      maximum - minimum < 0.001f) {
    return (height - 1) / 2;
  }
  const float normalized = (value - minimum) / (maximum - minimum);
  // Screen Y grows downwards, so invert the normalized value.
  return static_cast<int>((1.0f - normalized) * (height - 1) + 0.5f);
}

void WeatherHistory::reset() {
  for (auto& a : values_) std::ranges::fill(a, 0);

  next_ = 0;
  count_ = 0;
  has_sampled_ = false;

  resetTimestamp();
}

void WeatherHistory::resetTimestamp() { last_sample_ms_ = 0; }

}  // namespace weather_station
