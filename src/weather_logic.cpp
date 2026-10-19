#include "weather_logic.h"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cmath>

namespace weather_station {

  namespace {
    // Up to 31 one-minute samples cover a thirty-minute trend window. The
    // Theil-Sen median slope resists isolated bad readings.
    class TrendWindow {
     public:
      explicit TrendWindow(float minimum_change) : minimum_change_(minimum_change) {}

      int add(float value) {
        if (!std::isfinite(value)) {
          return 0;
        }

        samples_[next_] = value;
        next_ = (next_ + 1) % samples_.size();
        if (count_ < samples_.size()) {
          count_++;
        }
        if (count_ < 6) {
          return 0;
        }

        const size_t oldest = count_ == samples_.size() ? next_ : 0;
        // Compute every pairwise slope; their median is robust to outliers.
        std::array<float, 31 * 30 / 2> slopes{};
        size_t slope_count = 0;
        for (size_t i = 0; i < count_; i++) {
          for (size_t j = i + 1; j < count_; j++) {
            slopes[slope_count++] =
                (samples_[(oldest + j) % samples_.size()] -
                 samples_[(oldest + i) % samples_.size()]) /
                static_cast<float>(j - i);
          }
        }

        std::sort(slopes.begin(), slopes.begin() + slope_count);
        const float slope = slope_count % 2 == 0
                                ? (slopes[slope_count / 2 - 1] + slopes[slope_count / 2]) / 2.0f
                                : slopes[slope_count / 2];
        const float covered_minutes = static_cast<float>(count_ - 1);
        const float required_change = minimum_change_ * covered_minutes / 30.0f;
        const float estimated_change = slope * covered_minutes;
        if (estimated_change >= required_change) {
          return 1;
        }
        if (estimated_change <= -required_change) {
          return -1;
        }
        return 0;
      }

     private:
      std::array<float, 31> samples_{};
      size_t next_{0};
      size_t count_{0};
      float minimum_change_;
    };
  }  // namespace

  int sample_temperature_trend(float value) {
    static TrendWindow window(0.5f);  // 0.5 °C over a full 30-minute window.
    return window.add(value);
  }

  int sample_humidity_trend(float value) {
    static TrendWindow window(2.0f);  // 2 %RH over a full 30-minute window.
    return window.add(value);
  }

  namespace {
    class WeatherHistory {
     public:
      static constexpr size_t CAPACITY = 288;  // 24 hours at 5-minute intervals
      static constexpr uint32_t SAMPLE_INTERVAL_MS = 5UL * 60UL * 1000UL;

      void record(float outside_temp, float outside_humidity,
                  float inside_temp, float inside_humidity) {
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

      float extreme(int series, bool minimum) const {
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

      int point_x(int point, int width) const {
        if (width <= 1) return 0;
        point = std::max(0, std::min(point, 24));
        return point * (width - 1) / 24;
      }

      int point_y(int series, int point, int height) const {
        if (height <= 1 || series < 0 || series >= 4 || count_ == 0) {
          return height > 1 ? height / 2 : 0;
        }
        point = std::max(0, std::min(point, 24));
        const size_t oldest = count_ == CAPACITY ? next_ : 0;
        const size_t sample_index = (static_cast<size_t>(point) * (count_ - 1) + 12) / 24;
        const float value = values_[series][(oldest + sample_index) % CAPACITY];
        const float minimum = extreme(series, true);
        const float maximum = extreme(series, false);
        if (!std::isfinite(value) || !std::isfinite(minimum) ||
            !std::isfinite(maximum) || maximum - minimum < 0.001f) {
          return (height - 1) / 2;
        }
        const float normalized = (value - minimum) / (maximum - minimum);
        // Screen Y grows downwards, so invert the normalized value.
        return static_cast<int>((1.0f - normalized) * (height - 1) + 0.5f);
      }

     private:
      std::array<std::array<float, CAPACITY>, 4> values_{};
      size_t next_{0};
      size_t count_{0};
      uint32_t last_sample_ms_{0};
      bool has_sampled_{false};
    };

    WeatherHistory weather_history;
  }  // namespace

  void record_weather_history(float outside_temp, float outside_humidity,
                              float inside_temp, float inside_humidity) {
    weather_history.record(outside_temp, outside_humidity, inside_temp, inside_humidity);
  }

  float history_minimum(int series) {
    return weather_history.extreme(series, true);
  }

  float history_maximum(int series) {
    return weather_history.extreme(series, false);
  }

  int history_point_x(int point, int width) {
    return weather_history.point_x(point, width);
  }

  int history_point_y(int series, int point, int height) {
    return weather_history.point_y(series, point, height);
  }

  void check_ice_alert(float outside_temp, esphome::light::LightState *led,
                       bool disabled) {
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
