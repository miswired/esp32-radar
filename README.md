# ESP32 Microwave Motion Sensor

A feature-rich motion detection system using ESP32 and RCWL-0516 microwave radar sensor, with Home Assistant integration via MQTT.

## Features

- **Microwave Radar Detection** - Detects motion through walls using Doppler radar
- **Home Assistant Integration** - MQTT Discovery for automatic device registration
- **Web Interface** - Dashboard, settings, diagnostics, and API documentation
- **Adaptive Filtering** - Reduces false positives with configurable sensitivity
- **HTTP Webhooks** - Trigger automations on external systems
- **WLED Integration** - Visual alerts on LED strips
- **API Authentication** - Secure access with API keys and web passwords

## Quick Start

### Hardware Required

- ESP32 development board (ESP32-WROOM-32 or similar)
- RCWL-0516 microwave radar sensor
- USB cable for power and programming

See [Bill of Materials](docs/hardware/BOM.md) for details.

### Wiring

```
RCWL-0516    ESP32
---------    -----
VIN     -->  VIN (5V)
GND     -->  GND
OUT     -->  GPIO 13
```

See [Wiring Guide](docs/hardware/WIRING.md) for detailed instructions.

### Installation

1. **Clone the repository**
   ```bash
   git clone https://github.com/miswired/esp32-radar.git
   cd esp32-radar
   ```

2. **Install dependencies**
   ```bash
   arduino-cli lib install ArduinoJson@7.4.2 PubSubClient@2.8
   ```

   Alternatively, use the archived libraries in [libs/](libs/) for offline installation.

3. **Configure WiFi credentials**
   ```bash
   cd stage9_homeassistant
   cp credentials.h.example credentials.h
   # Edit credentials.h with your WiFi details
   ```

4. **Compile and upload**
   ```bash
   arduino-cli compile --fqbn esp32:esp32:esp32 stage9_homeassistant
   arduino-cli upload --fqbn esp32:esp32:esp32 -p /dev/ttyUSB0 stage9_homeassistant
   ```

5. **Access web interface**
   - Connect to your WiFi network
   - Find the device IP in serial monitor or router
   - Open `http://<device-ip>/` in a browser

## Documentation

- [Getting Started Tutorial](https://github.com/miswired/esp32-radar/wiki/Getting-Started)
- [Home Assistant Setup](stage9_homeassistant/HOME_ASSISTANT_SETUP.md)
- [API Reference](https://github.com/miswired/esp32-radar/wiki/API-Reference)
- [Hardware Guide](docs/hardware/)
- [Full Documentation (Wiki)](https://github.com/miswired/esp32-radar/wiki)

## Web Interface

| Page | Description |
|------|-------------|
| Dashboard | Real-time motion status and quick stats |
| Settings | WiFi, notifications, MQTT, WLED configuration |
| Diagnostics | System health, memory, event logs |
| API | REST API documentation |
| About | License and device information |

## Home Assistant Entities

When connected via MQTT, the following entities are created:

| Entity Type | Name | Description |
|-------------|------|-------------|
| Binary Sensor | Motion | ON when motion detected |
| Sensor | WiFi Signal | RSSI in dBm |
| Sensor | Uptime | Time since boot |
| Sensor | Free Heap | Available memory |
| Number | Trip Delay | Seconds before triggering |
| Number | Clear Timeout | Seconds before clearing |
| Number | Filter Threshold | Sensitivity percentage |

## Project Structure

```
esp32-radar/
├── stage9_homeassistant/   # Latest firmware with all features
├── libs/                   # Archived Arduino libraries
├── docs/                   # Additional documentation
│   └── hardware/          # BOM and wiring guides
├── openspec/              # Feature specifications
├── LICENSE                # GPL v3
├── CONTRIBUTING.md        # Contribution guidelines
├── CODE_OF_CONDUCT.md     # Community standards
└── CHANGELOG.md           # Version history
```

## Contributing

Contributions are welcome! Please read [CONTRIBUTING.md](CONTRIBUTING.md) before submitting pull requests.

## License

This project is licensed under the **GNU General Public License v3.0** - see [LICENSE](LICENSE) for details.

Copyright (C) 2024-2026 Miswired

## Acknowledgments

- [ArduinoJson](https://arduinojson.org/) - JSON library for Arduino
- [PubSubClient](https://pubsubclient.knolleary.net/) - MQTT client library
- [ESP32 Arduino Core](https://github.com/espressif/arduino-esp32) - ESP32 support for Arduino
