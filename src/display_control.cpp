#include "display_control.h"

#include <cmath>

namespace weather_station {

void send_st7796_command(DisplaySpiDevice *display,
                         esphome::output::BinaryOutput *dc_pin,
                         uint8_t command) {
  display->enable();
  dc_pin->turn_off();  // D/C low marks the byte as a command.
  display->write_byte(command);
  dc_pin->turn_on();
  display->disable();
}

void update_display_control(
    bool presence, float illuminance, float maximum_brightness,
    int timeout_steps, uint32_t &last_activity_ms, bool &is_asleep,
    bool transitioning, esphome::light::LightState *backlight,
    esphome::script::SingleScript<> *sleep_script,
    esphome::script::SingleScript<> *wake_script) {
  const uint32_t now = esphome::millis();
  if (presence) last_activity_ms = now;
  if (last_activity_ms == 0) last_activity_ms = now;
  if (transitioning) return;

  const bool should_be_awake = timeout_steps == 0 || presence ||
      static_cast<uint32_t>(now - last_activity_ms) <
          static_cast<uint32_t>(timeout_steps) * 30000UL;
  if (should_be_awake) {
    if (is_asleep) {
      wake_script->execute();
      return;
    }

    float brightness;
	static constexpr float minBrightness = 0.25f;
    if (illuminance < 3.0f) {
      brightness = maximum_brightness * 0.3f;
    } else if (illuminance < 10.0f) {
      brightness = maximum_brightness * 0.4f;
    } else if (illuminance < 30.0f) {
      brightness = maximum_brightness * 0.5f;
    } else if (illuminance < 45.0f) {
      brightness = maximum_brightness * 0.6f;
    } else if (illuminance < 60.0f) {
      brightness = maximum_brightness * 0.7f;
    } else if (illuminance < 90.0f) {
      brightness = maximum_brightness * 0.8f;
    } else if (illuminance < 120.0f) {
      brightness = maximum_brightness * 0.9f;
    } else {
      brightness = maximum_brightness;
    }
	brightness = std::min(std::max(brightness, minBrightness), 1.0f);
    const bool backlight_is_off = !backlight->current_values.is_on();
    const float current_brightness = backlight->current_values.get_brightness();
    if (backlight_is_off ||
        std::fabs(current_brightness - brightness) >= 0.1f) {
      auto call = backlight->turn_on();
      call.set_brightness(brightness);
      call.perform();
    }
  } else if (!is_asleep) {
    sleep_script->execute();
  }
}

}  // namespace weather_station
