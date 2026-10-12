#include "sht4x_maintenance.h"

#include "esphome/core/application.h"
#include "esphome/core/log.h"

namespace weather_station {
namespace {

constexpr char TAG[] = "sht4x_maintenance";
constexpr uint32_t COOLDOWN_MS = 60000;
constexpr uint8_t HIGH_POWER_LONG_HEATER_COMMAND = 0x39;
bool cycle_active = false;

}  // namespace

bool start_sht4x_maintenance(esphome::sht4x::SHT4XComponent *sensor) {
  if (sensor == nullptr || cycle_active || sensor->is_failed()) {
    return false;
  }

  sensor->stop_poller();
  cycle_active = true;
  // Let an in-flight 10 ms measurement callback finish before issuing the
  // heater command through this same sensor component and I2C device.
  esphome::App.scheduler.set_timeout(sensor, "manual_heater_start", 20, [sensor]() {
    if (!sensor->write_command(HIGH_POWER_LONG_HEATER_COMMAND)) {
      cycle_active = false;
      sensor->start_poller();
      ESP_LOGW(TAG, "Failed to start the heater command");
      return;
    }

    ESP_LOGI(TAG, "High-power heater started; allowing a 60-second cooldown");
    esphome::App.scheduler.set_timeout(sensor, "manual_heater_cooldown",
                                       1100 + COOLDOWN_MS, [sensor]() {
      cycle_active = false;
      sensor->start_poller();
      ESP_LOGI(TAG, "Cooldown complete; SHT4x polling resumed");
    });
  });
  return true;
}

}  // namespace weather_station
