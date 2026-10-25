#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <vector>

#include "esphome/core/hal.h"
#include "nvs.h"
#include "weather_data.h"

namespace esphome {
uint32_t test_millis = 0;
}
namespace {
std::vector<uint8_t> stored;
bool fail_open = false, fail_write = false, fail_read = false, fail_erase = false, fail_commit = false;
void fresh_storage() {
  stored.clear();
  fail_open = fail_write = fail_read = fail_erase = fail_commit = false;
}
void populate(weather_station::WeatherData& data) {
  data.init();
  for (int i = 0; i < 310; ++i) {
    esphome::test_millis = i * 300000UL;
    data.weatherHistory.record(i, i + 10, i + 20, i + 30);
    data.temperatureWindow.add(i);
    data.humidityWindow.add(-i);
  }
}
void crc_repair(std::vector<uint8_t>& blob) {
  uint32_t crc = 0xFFFFFFFF;
  for (size_t i = 0; i < blob.size() - 4; ++i) {
    crc ^= blob[i];
    for (int bit = 0; bit < 8; ++bit) crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320U : 0);
  }
  crc = ~crc;
  std::memcpy(blob.data() + blob.size() - 4, &crc, 4);
}
}  // namespace
esp_err_t nvs_open(const char* name, int mode, nvs_handle_t* handle) {
  assert(std::strcmp(name, "weather_backup") == 0 && mode == NVS_READWRITE);
  *handle = 1;
  return fail_open ? 99 : ESP_OK;
}
esp_err_t nvs_get_blob(nvs_handle_t, const char* key, void* out, size_t* size) {
  assert(std::strcmp(key, "snapshot") == 0);
  if (fail_read) return 99;
  if (stored.empty()) return ESP_ERR_NVS_NOT_FOUND;
  if (out == nullptr) {
    *size = stored.size();
    return ESP_OK;
  }
  if (*size < stored.size()) return ESP_ERR_NVS_INVALID_LENGTH;
  *size = stored.size();
  std::memcpy(out, stored.data(), stored.size());
  return ESP_OK;
}
esp_err_t nvs_set_blob(nvs_handle_t, const char*, const void* data, size_t size) {
  if (fail_write) return 99;
  stored.assign(static_cast<const uint8_t*>(data), static_cast<const uint8_t*>(data) + size);
  return ESP_OK;
}
esp_err_t nvs_erase_key(nvs_handle_t, const char*) {
  if (fail_erase) return 99;
  stored.clear();
  return ESP_OK;
}
esp_err_t nvs_commit(nvs_handle_t) { return fail_commit ? 99 : ESP_OK; }
void nvs_close(nvs_handle_t) {}
const char* esp_err_to_name(esp_err_t) { return "test error"; }

int main() {
  using weather_station::WeatherData;
  fresh_storage();
  WeatherData original{};
  populate(original);
  assert(original.save_backup());
  const auto good = stored;
  assert(good.size() == 4896);
  WeatherData restored{};
  esphome::test_millis = 0;
  restored.init();
  assert(stored.empty());
  for (int series = 0; series < 4; ++series) {
    assert(restored.weatherHistory.extreme(series, true) == 22 + series * 10);
    assert(restored.weatherHistory.extreme(series, false) == 309 + series * 10);
    for (int point = 0; point <= 24; ++point)
      assert(restored.weatherHistory.point_y(series, point, 45) ==
             original.weatherHistory.point_y(series, point, 45));
  }
  assert(restored.temperatureWindow.add(310) == 1);
  assert(restored.humidityWindow.add(-310) == -1);
  restored.weatherHistory.record(1000, 1001, 1002, 1003);
  assert(restored.weatherHistory.extreme(0, false) == 1000);
  WeatherData next_boot{};
  next_boot.init();
  assert(std::isnan(next_boot.weatherHistory.extreme(0, true)));

  for (int invalid = 0; invalid < 6; ++invalid) {
    stored = good;
    if (invalid == 0) stored[30] ^= 1;  // payload corruption
    if (invalid == 1) {
      stored[4] ^= 1;
      crc_repair(stored);
    }  // valid CRC, wrong version
    if (invalid == 2) stored.pop_back();
    if (invalid == 3) {
      stored[0] ^= 1;
      crc_repair(stored);
    }  // wrong magic
    if (invalid == 5) {
      stored[8] ^= 1;
      crc_repair(stored);
    }  // wrong declared size
    if (invalid == 4) {  // invalid ring index with valid checksum
      uint32_t bad_index = 9999;
      std::memcpy(stored.data() + 12, &bad_index, 4);
      crc_repair(stored);
    }
    WeatherData rejected{};
    rejected.init();
    assert(stored.empty());
    assert(std::isnan(rejected.weatherHistory.extreme(0, true)));
  }
  // A rejected backup must not destroy valid data retained in RTC.
  stored = good;
  stored[30] ^= 1;
  restored.init();
  assert(restored.weatherHistory.extreme(0, false) == 1000);
  for (int failure = 0; failure < 3; ++failure) {
    fresh_storage();
    fail_open = failure == 0;
    fail_write = failure == 1;
    fail_commit = failure == 2;
    assert(!original.save_backup());
  }
  for (int failure = 0; failure < 4; ++failure) {
    fresh_storage();
    stored = good;
    fail_open = failure == 0;
    fail_read = failure == 1;
    fail_erase = failure == 2;
    fail_commit = failure == 3;
    WeatherData rejected{};
    rejected.init();
    assert(std::isnan(rejected.weatherHistory.extreme(0, true)));
  }
  // Partly filled rings and missing sensor readings survive the handoff too.
  fresh_storage();
  WeatherData partial{};
  partial.init();
  esphome::test_millis = 12345678;
  partial.weatherHistory.record(NAN, 45, 21, 50);
  partial.temperatureWindow.add(20);
  assert(partial.save_backup());
  WeatherData partial_restored{};
  partial_restored.init();
  assert(std::isnan(partial_restored.weatherHistory.extreme(0, true)));
  assert(partial_restored.weatherHistory.extreme(1, true) == 45);
  assert(partial_restored.temperatureWindow.add(21) == 0);
  esphome::test_millis = 0;
  partial_restored.weatherHistory.record(10, 46, 22, 51);
  assert(partial_restored.weatherHistory.extreme(0, true) == 10);

  fresh_storage();
  WeatherData empty{};
  empty.init();
  assert(empty.save_backup());
  WeatherData empty_restored{};
  empty_restored.init();
  assert(std::isnan(empty_restored.weatherHistory.extreme(0, false)));
  std::cout << "Weather backup tests passed\n";
}
