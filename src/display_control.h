#pragma once

#include "esphome.h"

namespace weather_station {

using DisplaySpiDevice = esphome::spi::SPIDevice<
    esphome::spi::BIT_ORDER_MSB_FIRST,
    esphome::spi::CLOCK_POLARITY_LOW,
    esphome::spi::CLOCK_PHASE_LEADING,
    esphome::spi::DATA_RATE_1MHZ>;

// Send one command byte to the ST7796 over the display's SPI bus.
void send_st7796_command(DisplaySpiDevice *display,
                         esphome::output::BinaryOutput *dc_pin,
                         uint8_t command);

void update_display_control(
    bool presence, float illuminance, float maximum_brightness,
    int timeout_steps, uint32_t &last_activity_ms, bool &is_asleep,
    bool transitioning, esphome::light::LightState *backlight,
    esphome::script::SingleScript<> *sleep_script,
    esphome::script::SingleScript<> *wake_script);

}  // namespace weather_station
