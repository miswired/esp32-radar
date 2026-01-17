# Change: Add Web-Based Configuration and Notification System

## Why

The ESP32 mmWave presence sensor needs a complete, production-ready interface for configuration and external system integration. Currently, the project has no sensor integration, web interface, or notification capabilities. This change implements the full feature set required for practical deployment in smart home and automation scenarios.

Users need to:
- Configure detection parameters (sensitivity, zones, timeouts) without recompiling code
- Receive notifications when presence is detected to trigger external automation systems
- Monitor sensor health and debug detection issues through a web interface
- Maintain configuration across power cycles and reboots

## What Changes

This change implements a complete ESP32 presence detection system with the following capabilities:

- **Sensor Integration**: I2C communication with DFRobot C4001 mmWave sensor (with proper pull-up resistors)
- **Web Interface**: Responsive HTML interface with AP fallback mode for initial setup
- **Notification System**: Configurable HTTP GET/POST or raw socket notifications on presence detection
- **Persistent Configuration**: Non-volatile storage (NVS) for all settings
- **Advanced Detection Parameters**: Sensitivity, detection zones, minimum distance, debounce timing, presence timeout
- **Full Diagnostics**: Live sensor data, event logs, notification status, I2C health, memory usage, uptime
- **Watchdog Timer**: Automatic recovery from system hangs for long-running stability
- **Built-in Self-Tests**: Automated test functions at each stage with serial and web interfaces for immediate verification
- **Incremental Development**: Six distinct build stages, each producing a working, testable demo with clear acceptance criteria

All configuration is managed through the web interface. AP fallback mode ensures device is always accessible for configuration even without pre-configured WiFi credentials. Self-tests run automatically on boot and can be triggered via serial commands or web endpoint, providing immediate feedback on system health.

## Impact

### Affected Capabilities
- **NEW**: `sensor-interface` - I2C communication with C4001 mmWave sensor
- **NEW**: `web-interface` - Async web server with configuration UI
- **NEW**: `configuration` - NVS persistent storage system
- **NEW**: `notification-system` - HTTP and socket-based notifications
- **NEW**: `diagnostics` - Real-time monitoring and debugging interface
- **NEW**: `testing` - Built-in self-test functions with serial and web interfaces

### Affected Code
- New files (6 build stages):
  - `stage1_sensor_basic/` - Sensor reading with serial output
  - `stage2_wifi_web/` - WiFi connectivity and basic web interface
  - `stage3_persistence/` - Configuration storage (NVS)
  - `stage4_notifications/` - HTTP/socket notification system
  - `stage5_advanced_params/` - Advanced detection parameters
  - `stage6_full_system/` - Complete integration with full diagnostics

### Dependencies

**Arduino Libraries (install via Library Manager):**
- DFRobot_C4001 - Sensor interface library
- ESPAsyncWebServer - Async HTTP server
- AsyncTCP - Async networking for ESP32
- ArduinoJson (v6.x) - JSON parsing and generation
- HTTPClient (built-in with ESP32 core) - HTTP client for notifications

**Built-in ESP32 Libraries:**
- Wire.h - I2C communication
- WiFi.h - WiFi connectivity
- Preferences.h - NVS storage
- esp_task_wdt.h - Watchdog timer

**Hardware Requirements:**
- 2x 4.7kΩ resistors (I2C pull-ups for SDA and SCL)

**ESP32 Board Support:**
- ESP32 Arduino Core 2.0.x or 3.x (tested on 2.0.x)
- Partition scheme: "Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)"

### Testing Requirements
Each build stage must:
- Compile successfully with 0 errors and 0 warnings
- Upload to ESP32-WROOM-32
- Sketch size must stay under 80% of program memory (monitor via Arduino IDE)
- Free heap memory must remain above 100KB during operation
- Demonstrate new features added in that stage
- Maintain functionality from previous stages
- Pass stage-specific acceptance criteria before proceeding
