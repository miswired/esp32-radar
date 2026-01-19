# Implementation Tasks

## Project Status

**Sensor:** RCWL-0516 Microwave Radar Motion Sensor (simple GPIO interface)
**Previous Sensor:** DFRobot C4001 mmWave (archived due to reliability issues)

## Stage Transition Strategy

Each stage is implemented as a separate directory with its own `.ino` file to preserve working versions.

**Directory Structure:**
```
esp32-radar/
├── stage1_sensor_basic/    # Basic GPIO motion detection
├── stage2_wifi_web/        # WiFi + Web interface
├── stage3_persistence/     # NVS storage (future)
├── stage4_notifications/   # HTTP/Socket notifications (future)
├── stage5_advanced/        # Advanced parameters (future)
├── stage6_full_system/     # Full diagnostics (future)
└── archive/
    ├── c4001_sensor/       # Archived C4001 code
    └── stages/             # Archived stage snapshots
```

**Arduino CLI Commands:**
```bash
# Compile
arduino-cli compile --fqbn esp32:esp32:esp32 <stage_directory>

# Upload
arduino-cli upload --fqbn esp32:esp32:esp32 -p /dev/ttyUSB0 <stage_directory>

# Monitor
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

**Library Requirements:**
- ESP32 Arduino Core: 3.x
- ArduinoJson: 7.x (for web interface JSON APIs)
- WiFi, WebServer: Built-in ESP32 libraries

---

## Stage 1: Basic Sensor Interface [COMPLETED]
**Status:** ✅ Complete
**Directory:** `stage1_sensor_basic/`
**Archived:** `archive/stages/stage1_complete/`

### Features Implemented
- [x] RCWL-0516 GPIO communication (10Hz polling)
- [x] Trip delay state machine (motion must sustain 3s before alarm)
- [x] Clear timeout (30s no-motion clears alarm)
- [x] 4 states: IDLE → MOTION_PENDING → ALARM_ACTIVE → ALARM_CLEARING
- [x] LED feedback (ON when alarm active)
- [x] Serial commands: t/s/c/h/i/r/l
- [x] Self-tests (GPIO, LED, sensor, heap)
- [x] Reset reason detection
- [x] Comprehensive README documentation

### Configuration
| Parameter | Value |
|-----------|-------|
| Sensor Pin | GPIO 13 |
| LED Pin | GPIO 2 |
| Trip Delay | 3 seconds |
| Clear Timeout | 30 seconds |
| Poll Interval | 100ms (10Hz) |

---

## Stage 2: WiFi and Web Interface [COMPLETED]
**Status:** ✅ Complete
**Directory:** `stage2_wifi_web/`

### Features Implemented
- [x] WiFi Station mode connection with 15s timeout
- [x] AP fallback mode (ESP32-Radar-Setup, 192.168.4.1)
- [x] Built-in ESP32 WebServer (not async for stability)
- [x] Real-time web dashboard with auto-refresh
- [x] REST API endpoints (/status, /config)
- [x] JSON responses using ArduinoJson
- [x] Motion filter (70% threshold over 1s window to reduce noise)
- [x] WiFi self-tests
- [x] All Stage 1 features preserved

### Configuration
| Parameter | Value |
|-----------|-------|
| Sensor Pin | GPIO 13 |
| LED Pin | GPIO 2 |
| Trip Delay | 3 seconds |
| Clear Timeout | 10 seconds |
| Filter Threshold | 70% |
| Filter Window | 1000ms (10 samples) |

### Web Interface
- Dashboard at `/` with:
  - Current state display (color-coded)
  - Motion indicator (filtered)
  - Filter level vs threshold (e.g., "30 / 70%")
  - Raw sensor state (HIGH/LOW)
  - Statistics (alarm events, motion events, uptime, heap)
  - Configuration display (trip delay, clear timeout)
  - WiFi info (mode, IP, RSSI)
- REST API:
  - `GET /status` - Current status as JSON (includes filter stats)
  - `GET /config` - Current configuration as JSON (includes filter config)

### Libraries
- ArduinoJson (install: `arduino-cli lib install "ArduinoJson"`)
- WiFi, WebServer (built-in)

---

## Stage 3: Persistent Configuration Storage [PENDING]
**Status:** ⏳ Not Started
**Goal:** Add NVS storage for WiFi credentials and settings

### Planned Features
- [ ] NVS/Preferences storage for configuration
- [ ] Persistent WiFi credentials
- [ ] Persistent trip delay and clear timeout settings
- [ ] Web UI form for configuration changes
- [ ] `/config` POST endpoint to save settings
- [ ] Factory reset endpoint

---

## Stage 4: Notification System [PENDING]
**Status:** ⏳ Not Started
**Goal:** Implement HTTP and socket notifications

### Planned Features
- [ ] HTTP GET notifications with query parameters
- [ ] HTTP POST notifications with JSON payload
- [ ] Raw TCP socket notifications
- [ ] Notification configuration via web UI
- [ ] Test notification button
- [ ] Notification on alarm state changes only

---

## Stage 5: Advanced Detection Parameters [PENDING]
**Status:** ⏳ Not Started
**Goal:** Add configurable timing parameters

### Planned Features
- [ ] Configurable trip delay via web UI
- [ ] Configurable clear timeout via web UI
- [ ] Debounce settings
- [ ] Parameter validation

---

## Stage 6: Full Diagnostics and System Integration [PENDING]
**Status:** ⏳ Not Started
**Goal:** Add comprehensive monitoring and logging

### Planned Features
- [ ] Event logging (circular buffer)
- [ ] Watchdog timer
- [ ] Comprehensive diagnostics dashboard
- [ ] Memory leak monitoring
- [ ] I2C/GPIO health monitoring
- [ ] 24-hour stability testing

---

## Hardware Setup

### RCWL-0516 Wiring
```
RCWL-0516          ESP32
---------          -----
VIN            →   VIN (5V from USB)
GND            →   GND
OUT            →   GPIO 13
3V3            →   DO NOT CONNECT (output!)
CDS            →   DO NOT CONNECT (optional LDR)
```

### Notes
- Sensor needs 4-28V on VIN (use ESP32 VIN, not 3.3V)
- Simple digital output: HIGH = motion, LOW = no motion
- No pull-up resistors or libraries required
- Detection range: 5-7 meters, 360° omnidirectional

---

## Testing Commands

### Serial Commands (all stages)
| Command | Description |
|---------|-------------|
| `t` | Run self-test |
| `s` | Print statistics |
| `c` | Print configuration |
| `h` | Help menu |
| `r` | Restart ESP32 |
| `i` | System info |

### Quick Test Procedure
1. Compile and upload
2. Open serial monitor (115200 baud)
3. Verify self-test passes
4. Wave hand near sensor
5. Verify state transitions in serial output
6. (Stage 2+) Access web dashboard at displayed IP
