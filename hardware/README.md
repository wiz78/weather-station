# Weather station KiCad schematic

Open `weather-station.kicad_pro` in KiCad. The schematic contains a custom ESP32-S3 DevKitC-1 symbol with both 22-pin headers, connector symbols for the TFT, touch controller, sensors and radar, plus the LED, resistor and optional capacitors. Routed wires connect the used pins; unused DevKitC header pins are shown without labels or wires. The GPIO assignments follow `../hw_official_build.yaml`.

Confirm supply input and logic-level requirements for the exact SHT45, SGP41, BH1750, FT6336U, LD2410C, and TFT breakout boards before connecting power. The I²C bus is shared on GPIO4 (SDA) and GPIO5 (SCL); all grounds must be common. The touch controller shares that bus and uses GPIO2 for INT and GPIO1 for reset. The LD2410C UART is crossed (sensor TX to ESP RX/GPIO18, sensor RX to ESP TX/GPIO17).

The optional 100 nF and 10 µF capacitors are parallel from 5V to GND near the ESP32 power input. The LED circuit is GPIO16 → LED anode → 220 Ω → GND.
