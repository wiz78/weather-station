#pragma once

#include <iterator>

#include "trend_window.h"
#include "weather_history.h"

namespace weather_station {

class WeatherData {
 public:
  // Increment whenever the serialized fields, units, or interpretation change.
  static constexpr uint32_t BACKUP_VERSION = 1;

  int32_t magic;
  TrendWindow temperatureWindow;
  TrendWindow humidityWindow;
  WeatherHistory weatherHistory;

  void init();
  bool save_backup() const;
  bool load_backup();

 private:
  static constexpr size_t BACKUP_SIZE = [] {
    constexpr TrendWindow trend{};
    constexpr WeatherHistory history{};
    constexpr size_t header_size = 3 * sizeof(uint32_t);   // Magic, version, length.
    constexpr size_t indices_size = 2 * sizeof(uint32_t);  // Next index, sample count.
    constexpr size_t trend_size = indices_size + std::size(trend.samples_) * sizeof(float);
    constexpr size_t history_size =
        indices_size + std::size(history.values_) * std::size(history.values_[0]) * sizeof(float);
    return header_size + 2 * trend_size + history_size + sizeof(uint32_t);  // CRC32.
  }();
  static constexpr int32_t RTC_MAGIC_NUMBER = static_cast<int32_t>(0xCAFECA00U + BACKUP_VERSION);
  bool valid() const;
};

}  // namespace weather_station
