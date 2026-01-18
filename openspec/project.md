# Project Context

## Purpose
ESP32-based motion detection system using RCWL-0516 Microwave Radar sensor. The system provides:
- Real-time motion detection (5-7m range)
- Simple digital output (HIGH when motion detected)
- WiFi-enabled web interface for status monitoring and configuration
- Serial output for debugging and data logging

Target use cases: smart home automation, occupancy detection, security, energy management

## Sensor Change Notice

**Previous Sensor:** DFRobot SEN0610 C4001 24GHz mmWave (archived due to reliability issues)
**Current Sensor:** RCWL-0516 Microwave Radar Motion Sensor

See `archive/c4001_sensor/ARCHIVE_README.md` for details on why the sensor was changed.

## Tech Stack
- **Microcontroller**: ESP32-WROOM-32 (Dual-core Xtensa LX6, WiFi/BT)
- **Sensor**: RCWL-0516 Microwave Radar Motion Sensor (~3.2GHz Doppler radar)
- **Communication**: Simple digital GPIO (no protocol required)
- **Development**: Arduino IDE with ESP32 board support
- **Programming Language**: C++ (Arduino framework)
- **Web Interface**: ESP32 async web server (ESPAsyncWebServer library recommended)
- **WiFi**: 2.4GHz 802.11 b/g/n
- **Libraries**:
  - WiFi.h (ESP32 WiFi)
  - ESPAsyncWebServer.h (async web server)
  - AsyncTCP.h (async networking)

## Project Conventions

### Code Style
- Follow Arduino coding style conventions
- Use camelCase for variables and functions: `sensorData`, `readMotion()`
- Use PascalCase for classes: `MotionSensor`
- Use UPPER_CASE for constants: `SENSOR_PIN`, `WIFI_TIMEOUT`
- Indentation: 2 spaces (Arduino IDE default)
- Max line length: 100 characters
- Always initialize variables at declaration
- Use descriptive names for functions and variables
- Add comments for complex logic

### Architecture Patterns
- **Modular Design**: Separate concerns into logical modules
  - Sensor reading module (GPIO digital read)
  - Web server module (HTTP endpoints, WebSocket for real-time updates)
  - WiFi management module (connection, reconnection handling)
  - Data processing module (debouncing, state management)
- **Non-blocking Code**: Use async patterns to prevent blocking main loop
- **State Machine**: Manage sensor states (idle, motion detected)
- **Event-driven**: Trigger actions based on motion events
- **Configuration Management**: Store WiFi credentials and settings in EEPROM/NVS

### Testing Strategy
- **Serial Debugging**: Use Serial.print() extensively during development
- **Unit Testing**: Test individual sensor reading functions
- **Integration Testing**: Verify GPIO reading, WiFi connectivity, web interface
- **Field Testing**: Validate detection accuracy at various distances and angles
- **Edge Cases**: Test sensor behavior with multiple people, static presence, no motion

### Git Workflow
- Main branch: `main` (stable, tested code)
- Feature branches: `feature/description` (e.g., `feature/web-interface`)
- Bug fixes: `fix/description` (e.g., `fix/wifi-reconnect`)
- Commit message format: `type: brief description` (e.g., `feat: add motion detection`, `fix: resolve WiFi reconnection`)
- Test before committing to main branch

## Domain Context

### RCWL-0516 Microwave Radar Technology
- **Doppler Radar**: Detects motion by measuring frequency shift of reflected microwaves
- **Operating Frequency**: ~3.18 GHz (different from WiFi 2.4GHz, no interference)
- **Detection Range**: 5-7 meters (typical indoor environment)
- **Detection Angle**: 360° omnidirectional
- **Output Signal**: Digital HIGH (3.3V) when motion detected, LOW when no motion
- **Trigger Duration**: ~2-3 seconds HIGH after motion (retriggerable)
- **Advantages**:
  - Detects through non-metallic materials (walls, doors, plastic enclosures)
  - Works in any lighting condition (unlike PIR)
  - Works in any temperature (more reliable than PIR in hot environments)
  - Detects any moving object (not just warm bodies)

### Simple GPIO Interface
- **No protocol required**: Just digitalRead() on the output pin
- **No pull-up resistors needed**: Sensor has built-in circuitry
- **No library needed**: Standard Arduino GPIO functions work
- **No configuration required**: Sensor works out of the box

### Motion Detection Logic
The system implements a multi-stage detection pipeline:

1. **Raw Detection**: GPIO reads sensor state (HIGH = motion, LOW = no motion)
2. **Trip Delay**: Motion must be sustained for configurable duration before alarm triggers
3. **Alarm State**: Once tripped, system enters alarm state and sends notifications
4. **Clear Timeout**: No motion for configurable duration clears the alarm

**Trip Delay Feature**: Prevents false alarms from brief movements (e.g., pets, curtains, HVAC).
Motion must be continuously detected for the configured trip delay before the alarm activates.

**Configurable Parameters** (via web interface in later stages):
- `tripDelay`: Seconds of sustained motion before alarm triggers (default: 3 seconds)
- `clearTimeout`: Seconds of no motion before alarm clears (default: 30 seconds)

### Optional Hardware Modifications
The RCWL-0516 has solder pads on the back for optional modifications:
- **C-TM**: Add capacitor to extend trigger time (0.2µF = 50s, 1µF = 250s)
- **R-GN**: Add resistor to reduce range (1MΩ = 5m, 270kΩ = 1.5m)
- **R-CDS**: Add LDR to disable in bright conditions

### ESP32 WiFi Patterns
- Async web servers for non-blocking operation
- WebSocket for real-time data streaming
- HTML/CSS/JS served from SPIFFS or embedded strings
- RESTful API endpoints for sensor data (JSON format)
- Common patterns: status indicators, real-time charts

## Important Constraints
- **Memory**: ESP32-WROOM-32 has 520KB SRAM, 4MB Flash - monitor heap usage
- **Power**: RCWL-0516 requires 4-28V input (use ESP32 VIN pin, not 3.3V)
- **3.3V Output**: Sensor provides regulated 3.3V output (100mA max) - don't use for input
- **WiFi Range**: 2.4GHz only, typical indoor range ~50m
- **Processing**: Avoid blocking operations in main loop
- **Sensor Limitations**:
  - Detection range: 5-7m (not adjustable without hardware modification)
  - Sensitivity: Fixed (not adjustable via software)
  - Detects motion only (not static presence like mmWave sensors)
  - Metal objects behind sensor can reduce performance (keep >1cm clearance)
  - Mount rigidly to prevent vibration-induced false triggers

## External Dependencies

### Required Arduino Libraries
- **WiFi**: ESP32 WiFi (built-in ESP32 core)
- **ESPAsyncWebServer**: Async web server ([GitHub](https://github.com/me-no-dev/ESPAsyncWebServer))
- **AsyncTCP**: Async TCP library for ESP32 ([GitHub](https://github.com/me-no-dev/AsyncTCP))

### Documentation Resources
- Random Nerd Tutorials: [ESP32 RCWL-0516 Guide](https://randomnerdtutorials.com/esp32-rcwl-0516-arduino/)
- Instructables: [All About RCWL-0516](https://www.instructables.com/All-About-RCWL-0516-Microwave-Radar-Motion-Sensor/)
- GitHub: [RCWL-0516 Technical Info](https://github.com/jdesbonnet/RCWL-0516)

### Hardware Connections
```
RCWL-0516    ESP32
---------    -----
VIN      →   VIN (5V from USB)
GND      →   GND
OUT      →   GPIO 13 (or any digital input pin)
3V3      →   (do not connect - this is an OUTPUT)
CDS      →   (optional LDR connection)
```

**Important**: The sensor's VIN needs 4-28V. Use the ESP32's VIN pin (which provides 5V from USB) not the 3.3V pin.
