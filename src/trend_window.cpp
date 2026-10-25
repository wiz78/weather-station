#include "trend_window.h"

#include <algorithm>
#include <cmath>
#include <ranges>

namespace weather_station {

void TrendWindow::set_minimum_change(const float minimum_change) { minimum_change_ = minimum_change; }

int TrendWindow::add(const float value) {
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
  static std::array<float, 31 * 30 / 2> slopes{};
  size_t slope_count = 0;
  for (size_t i = 0; i < count_; i++) {
    for (size_t j = i + 1; j < count_; j++) {
      slopes[slope_count++] =
          (samples_[(oldest + j) % samples_.size()] - samples_[(oldest + i) % samples_.size()]) /
          static_cast<float>(j - i);
    }
  }

  std::sort(slopes.begin(), slopes.begin() + slope_count);
  const float slope = slope_count % 2 == 0 ? (slopes[slope_count / 2 - 1] + slopes[slope_count / 2]) / 2.0f
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

void TrendWindow::reset() {
  std::ranges::fill(samples_, 0);
  next_ = 0;
  count_ = 0;
}

}  // namespace weather_station
