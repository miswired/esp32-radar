# Project Context

## Purpose
ESP32-based human presence detection system using DFRobot SEN0610 C4001 24GHz mmWave FMCW radar sensor. The system provides:
- Real-time human presence detection (up to 8m range)
- Motion detection and ranging (up to 12m range)
- WiFi-enabled web interface for status monitoring and configuration
- Serial output for debugging and data logging

Target use cases: smart home automation, occupancy detection, energy management

## Tech Stack
- **Microcontroller**: ESP32-WROOM-32 (Dual-core Xtensa LX6, WiFi/BT)
- **Sensor**: DFRobot SEN0610 C4001 mmWave Presence Sensor (24GHz FMCW)
- **Communication**: I2C interface (GPIO 21=SDA, GPIO 22=SCL)
- **Development**: Arduino IDE with ESP32 board support
- **Programming Language**: C++ (Arduino framework)
- **Web Interface**: ESP32 async web server (ESPAsyncWebServer library recommended)
- **WiFi**: 2.4GHz 802.11 b/g/n
- **Libraries**:
  - DFRobot_C4001 (sensor interface)
  - Wire.h (I2C communication)
  - WiFi.h (ESP32 WiFi)
  - ESPAsyncWebServer.h (async web server)
  - AsyncTCP.h (async networking)

## Project Conventions

### Code Style
- Follow Arduino coding style conventions
- Use camelCase for variables and functions: `sensorData`, `readPresence()`
- Use PascalCase for classes: `PresenceSensor`
- Use UPPER_CASE for constants: `I2C_SDA_PIN`, `WIFI_TIMEOUT`
- Indentation: 2 spaces (Arduino IDE default)
- Max line length: 100 characters
- Always initialize variables at declaration
- Use descriptive names for functions and variables
- Add comments for complex logic and sensor-specific configurations

### Architecture Patterns
- **Modular Design**: Separate concerns into logical modules
  - Sensor reading module (I2C communication with C4001)
  - Web server module (HTTP endpoints, WebSocket for real-time updates)
  - WiFi management module (connection, reconnection handling)
  - Data processing module (filtering, state management)
- **Non-blocking Code**: Use async patterns to prevent blocking main loop
- **State Machine**: Manage sensor states (idle, detecting, presence confirmed)
- **Event-driven**: Trigger actions based on presence/motion events
- **Configuration Management**: Store WiFi credentials and sensor settings in EEPROM/NVS

### Testing Strategy
- **Serial Debugging**: Use Serial.print() extensively during development
- **Unit Testing**: Test individual sensor reading functions
- **Integration Testing**: Verify I2C communication, WiFi connectivity, web interface
- **Field Testing**: Validate detection accuracy at various distances and angles
- **Edge Cases**: Test sensor behavior with multiple people, static presence, no presence

### Git Workflow
- Main branch: `main` (stable, tested code)
- Feature branches: `feature/description` (e.g., `feature/web-interface`)
- Bug fixes: `fix/description` (e.g., `fix/i2c-timeout`)
- Commit message format: `type: brief description` (e.g., `feat: add presence detection`, `fix: resolve WiFi reconnection`)
- Test before committing to main branch

## Domain Context

### mmWave Radar Technology
- **FMCW (Frequency Modulated Continuous Wave)**: Enables distance and speed measurement without echo wait times
- **24GHz ISM Band**: Industrial, Scientific, Medical frequency band for radar sensors
- **Detection Range**:
  - Presence: 1.2m - 8m
  - Motion/Ranging: 1.2m - 12m
  - Beam angle: 100° horizontal
- **Environmental Resilience**: Unaffected by light, temperature, humidity, dust, or noise
- **Advantages over PIR**: Detects static presence (people sitting/sleeping), not just movement

### I2C Communication
- **Default Address**: 0x2A (alternative: 0x2B)
- **Pull-up Resistors**: Requires 4.7kΩ on SDA and SCL lines
- **Constraint**: Cannot use UART and I2C simultaneously
- **ESP32 Pins**: GPIO 21 (SDA), GPIO 22 (SCL) - default I2C pins

### ESP32 WiFi Patterns
- Async web servers for non-blocking operation
- WebSocket for real-time data streaming
- HTML/CSS/JS served from SPIFFS or embedded strings
- RESTful API endpoints for sensor data (JSON format)
- Common patterns: sliders, toggles, gauges, real-time charts

## Important Constraints
- **Memory**: ESP32-WROOM-32 has 520KB SRAM, 4MB Flash - monitor heap usage
- **I2C Speed**: Standard 100kHz or Fast Mode 400kHz
- **Power**: 3.3V logic level for ESP32; sensor accepts 3.3V-5.5V
- **WiFi Range**: 2.4GHz only, typical indoor range ~50m
- **Processing**: Avoid blocking operations in main loop - use FreeRTOS tasks if needed
- **Sensor Limitations**:
  - Detection range: 1.2m minimum, 12m maximum
  - Cannot simultaneously detect through walls (metal/concrete attenuates signal)
  - Sensitivity affected by target size and distance

## External Dependencies

### Required Arduino Libraries
- **DFRobot_C4001**: Official sensor library ([GitHub](https://github.com/cdjq/DFRobot_C4001))
- **Wire**: I2C communication (built-in)
- **WiFi**: ESP32 WiFi (built-in ESP32 core)
- **ESPAsyncWebServer**: Async web server ([GitHub](https://github.com/me-no-dev/ESPAsyncWebServer))
- **AsyncTCP**: Async TCP library for ESP32 ([GitHub](https://github.com/me-no-dev/AsyncTCP))

### Documentation Resources
- DFRobot Wiki: [SEN0610 Documentation](https://wiki.dfrobot.com/SKU_SEN0610_Gravity_C4001_mmWave_Presence_Sensor_12m_I2C_UART)
- ESP32 Examples: [Random Nerd Tutorials](https://randomnerdtutorials.com/projects-esp32/)
- ESP32 Wiring Guide: [ESPBoards C4001 Guide](https://www.espboards.dev/sensors/c4001/)

### Hardware Connections
- **VCC** → 3.3V (or 5V)
- **GND** → GND
- **SDA** → GPIO 21 (with 4.7kΩ pull-up to 3.3V)
- **SCL** → GPIO 22 (with 4.7kΩ pull-up to 3.3V)
