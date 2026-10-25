#include "weather_data.h"

#include <nvs.h>

#include <cstring>
#include <limits>
#include <memory>
#include <vector>

#include "esphome/core/log.h"

namespace weather_station {
namespace {
constexpr char TAG[] = "weather_backup";
constexpr char NVS_NAMESPACE[] = "weather_backup";
constexpr char NVS_KEY[] = "snapshot";
constexpr uint32_t BACKUP_MAGIC = 0x57454154;  // WEAT
static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559);

// Explicit little-endian encoding avoids object padding and size_t/bool ABI
// dependencies.
void append_u32(std::vector<uint8_t>& bytes, uint32_t value) {
  for (int shift = 0; shift < 32; shift += 8) bytes.push_back(static_cast<uint8_t>(value >> shift));
}

uint32_t read_u32(const std::vector<uint8_t>& bytes, size_t& offset) {
  uint32_t value = 0;
  for (int shift = 0; shift < 32; shift += 8) value |= static_cast<uint32_t>(bytes[offset++]) << shift;
  return value;
}

void append_float(std::vector<uint8_t>& bytes, float value) {
  uint32_t bits;
  std::memcpy(&bits, &value, sizeof(bits));
  append_u32(bytes, bits);
}

float read_float(const std::vector<uint8_t>& bytes, size_t& offset) {
  const uint32_t bits = read_u32(bytes, offset);
  float value;
  std::memcpy(&value, &bits, sizeof(value));
  return value;
}

uint32_t checksum(const std::vector<uint8_t>& bytes, size_t length) {
  uint32_t crc = 0xFFFFFFFF;
  for (size_t i = 0; i < length; ++i) {
    crc ^= bytes[i];
    for (int bit = 0; bit < 8; ++bit) crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320U : 0);
  }
  return ~crc;
}

bool log_result(esp_err_t error, const char* operation) {
  if (error == ESP_OK) return true;
  ESP_LOGW(TAG, "%s failed: %s", operation, esp_err_to_name(error));
  return false;
}
}  // namespace

bool WeatherData::valid() const {
  const auto valid_trend = [](const TrendWindow& window) {
    return window.next_ < window.samples_.size() && window.count_ <= window.samples_.size() &&
           (window.count_ == window.samples_.size() || window.next_ == window.count_);
  };
  return magic == RTC_MAGIC_NUMBER && valid_trend(temperatureWindow) && valid_trend(humidityWindow) &&
         weatherHistory.next_ < WeatherHistory::CAPACITY &&
         weatherHistory.count_ <= WeatherHistory::CAPACITY &&
         (weatherHistory.count_ == WeatherHistory::CAPACITY || weatherHistory.next_ == weatherHistory.count_);
}

void WeatherData::init() {
  temperatureWindow.set_minimum_change(0.5f);
  humidityWindow.set_minimum_change(2.0f);
  if (!valid()) {
    temperatureWindow.reset();
    humidityWindow.reset();
    weatherHistory.reset();
    magic = RTC_MAGIC_NUMBER;
    ESP_LOGI(TAG, "Initialized weather history");
  } else {
    weatherHistory.resetTimestamp();
    ESP_LOGI(TAG, "Preserved weather history in RTC memory");
  }
  load_backup();
}

bool WeatherData::save_backup() const {
  if (!valid()) {
    ESP_LOGW(TAG, "Refusing to save invalid weather history");
    return false;
  }
  std::vector<uint8_t> bytes;
  bytes.reserve(BACKUP_SIZE);
  append_u32(bytes, BACKUP_MAGIC);
  append_u32(bytes, BACKUP_VERSION);
  append_u32(bytes, BACKUP_SIZE);
  for (const auto* window : {&temperatureWindow, &humidityWindow}) {
    append_u32(bytes, window->next_);
    append_u32(bytes, window->count_);
    for (float value : window->samples_) append_float(bytes, value);
  }
  append_u32(bytes, weatherHistory.next_);
  append_u32(bytes, weatherHistory.count_);
  for (const auto& series : weatherHistory.values_)
    for (float value : series) append_float(bytes, value);
  append_u32(bytes, checksum(bytes, bytes.size()));

  nvs_handle_t handle;
  if (!log_result(nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle), "Opening NVS")) return false;
  const bool saved =
      log_result(nvs_set_blob(handle, NVS_KEY, bytes.data(), bytes.size()), "Saving snapshot") &&
      log_result(nvs_commit(handle), "Committing snapshot");
  nvs_close(handle);
  if (saved) ESP_LOGI(TAG, "Saved OTA weather backup (%u bytes)", static_cast<unsigned>(bytes.size()));
  return saved;
}

bool WeatherData::load_backup() {
  nvs_handle_t handle;
  if (!log_result(nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle), "Opening NVS")) return false;
  size_t length = 0;
  esp_err_t error = nvs_get_blob(handle, NVS_KEY, nullptr, &length);
  if (error == ESP_ERR_NVS_NOT_FOUND) {
    nvs_close(handle);
    return false;
  }
  if (!log_result(error, "Reading snapshot size")) {
    nvs_close(handle);
    return false;
  }

  // Bound the allocation even if a damaged or future snapshot reports a huge
  // size.
  std::vector<uint8_t> bytes;
  bool compatible = length == BACKUP_SIZE;
  if (compatible) {
    bytes.resize(length);
    compatible = log_result(nvs_get_blob(handle, NVS_KEY, bytes.data(), &length), "Reading snapshot") &&
                 length == BACKUP_SIZE;
  }

  // Consume before applying. Failed consumption leaves the current RTC state
  // intact.
  const bool consumed = log_result(nvs_erase_key(handle, NVS_KEY), "Consuming snapshot") &&
                        log_result(nvs_commit(handle), "Committing consumption");
  nvs_close(handle);
  if (!consumed) return false;

  size_t offset = 0;
  if (!compatible || read_u32(bytes, offset) != BACKUP_MAGIC || read_u32(bytes, offset) != BACKUP_VERSION ||
      read_u32(bytes, offset) != BACKUP_SIZE) {
    ESP_LOGW(TAG, "Discarded incompatible weather backup");
    return false;
  }
  size_t crc_offset = bytes.size() - 4;
  if (read_u32(bytes, crc_offset) != checksum(bytes, bytes.size() - 4)) {
    ESP_LOGW(TAG, "Discarded corrupt weather backup");
    return false;
  }

  // Keep the ~5 KiB staging object off ESPHome's main task stack.
  auto staged = std::make_unique<WeatherData>();
  auto& restored = *staged;
  restored.magic = RTC_MAGIC_NUMBER;
  restored.temperatureWindow.set_minimum_change(0.5f);
  restored.humidityWindow.set_minimum_change(2.0f);
  for (auto* window : {&restored.temperatureWindow, &restored.humidityWindow}) {
    window->next_ = read_u32(bytes, offset);
    window->count_ = read_u32(bytes, offset);
    for (auto& value : window->samples_) value = read_float(bytes, offset);
  }
  restored.weatherHistory.next_ = read_u32(bytes, offset);
  restored.weatherHistory.count_ = read_u32(bytes, offset);
  for (auto& series : restored.weatherHistory.values_)
    for (auto& value : series) value = read_float(bytes, offset);
  restored.weatherHistory.resetTimestamp();
  if (!restored.valid()) {
    ESP_LOGW(TAG, "Discarded weather backup with invalid ring indices");
    return false;
  }
  *this = restored;
  ESP_LOGI(TAG, "Restored and consumed OTA weather backup");
  return true;
}

}  // namespace weather_station
