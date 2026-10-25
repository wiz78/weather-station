# ESPHome Weather Station

A beautiful, ESPHome-powered weather station dashboard on a 4.0" touchscreen.

## Hardware Requirements (Official Build)
- **Board**: ESP32-S3 DevKitC-1 N16R8
- **Display**: Hosyond 4.0" 320x480 Capacitive Touch (ST7796S SPI)
- **Temp/Humidity**: SHT45 (I2C)
- **Air Quality**: SGP41 (I2C)
- **Presence**: HLK-LD2410C (UART)
- **Light Sensor**: GY-302 BH1750 (I2C)
- **Ice Alert**: 1x LED attached to GPIO (with resistor)

## Features
- Auto-dimming and screen-off based on room presence (LD2410) and ambient light (BH1750)
- "Ice Alert" blinking LED when outside temperature drops below 4°C
- Highly customizable via ESPHome packages

## Installation

You do NOT need to fork this repository or download any files manually. Simply use ESPHome's package feature!

1. Open your ESPHome Dashboard.
2. Click **New Device** and create a new project.
3. Open the YAML file for your new device and replace/integrate the contents with that of `example_user_config.yaml`
4. Click **Install**. ESPHome will automatically download the UI, compile the custom C++ logic, and flash your ESP32.

## Custom Hardware?
If you are using different sensors (e.g., an AHT10 instead of the SHT45), you can easily override the official hardware build. Simply remove the `hardware:` import from your YAML, copy the content of `hw_official_build.yaml` in your YAML, and edit it to define your sensors locally.

## History across OTA updates

The weather history and temperature/humidity trend windows stay in RTC RAM during
normal operation. After a successful native ESPHome or web-server OTA upload,
`WeatherData::save_backup()` synchronously writes a binary snapshot to
the existing NVS partition (namespace `weather_backup`, key `snapshot`).

At the end of `WeatherData::init()`, `load_backup()` checks the format version,
length, CRC32, and ring indices. It deletes and commits the backup before applying
it, so subsequent reboots cannot replay stale history. Invalid snapshots are
consumed without replacing valid RTC data. Sampling timestamps are not restored.
Increment `WeatherData::BACKUP_VERSION` whenever serialized fields or their
meaning change; incompatible snapshots are discarded. The NVS key stays stable
across firmware/configuration changes.

Protection starts with the **next update after this feature has been installed**.
Failed uploads do not create a backup. Storage errors are logged and do not stop
the firmware update. This is an OTA handoff, not continuous power-loss protection:
a power interruption before saving or between consuming and applying a snapshot
can still lose history. Full flash erasure or changes to the NVS partition can
also remove the backup. OTA downtime is not backfilled with historical samples.

Run the host tests (C++20 compiler required) with:

```sh
sh tests/run_weather_backup_tests.sh
```
