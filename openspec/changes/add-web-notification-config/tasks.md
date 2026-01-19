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

## Stage 3: Persistent Configuration Storage [COMPLETED]
**Status:** ✅ Complete
**Directory:** `stage3_persistence/`

### Features Implemented
- [x] NVS/Preferences storage for configuration
- [x] Persistent WiFi credentials (SSID and password)
- [x] Persistent trip delay and clear timeout settings
- [x] Persistent filter threshold setting
- [x] Sticky navigation bar on all pages
- [x] Web UI form for configuration changes (`/settings` page)
- [x] API documentation page (`/api`)
- [x] `POST /config` endpoint to save settings
- [x] `POST /reset` endpoint for factory reset
- [x] Serial command 'f' for factory reset
- [x] Checksum validation for stored config
- [x] NVS self-test

### Configuration
| Parameter | Range | Default |
|-----------|-------|---------|
| WiFi SSID | 1-63 chars | From credentials.h |
| WiFi Password | 1-63 chars | From credentials.h |
| Trip Delay | 1-60 seconds | 3 |
| Clear Timeout | 1-300 seconds | 10 |
| Filter Threshold | 10-100% | 70 |

### Web Interface
- Navigation bar on all pages (Dashboard, Settings, API)
- Dashboard at `/` with real-time status
- Settings page at `/settings` with configuration form
- API documentation at `/api` with endpoint details
- REST API:
  - `GET /status` - Current status as JSON
  - `GET /config` - Current configuration as JSON
  - `POST /config` - Save configuration to NVS
  - `POST /reset` - Factory reset

---

## Stage 4: Notification System [COMPLETED]
**Status:** ✅ Complete
**Directory:** `stage4_notifications/`

### Features Implemented
- [x] HTTP GET notifications with query parameters
- [x] HTTP POST notifications with JSON payload
- [x] Notification configuration via web UI (/settings)
- [x] Test notification button with status feedback
- [x] Notifications on alarm state changes (triggered/cleared)
- [x] Notification statistics tracking (sent/failed)
- [x] POST /test-notification endpoint
- [x] Persistent notification settings in NVS

### Configuration
| Parameter | Type | Description |
|-----------|------|-------------|
| notifyEnabled | boolean | Enable/disable notifications |
| notifyUrl | string | Webhook URL (max 127 chars) |
| notifyMethod | 0 or 1 | 0 = GET, 1 = POST |

### Notification Events
- `alarm_triggered` - Sent when alarm activates
- `alarm_cleared` - Sent when alarm clears
- `test` - Sent via test button

---

## Stage 5: Advanced Detection Parameters [MERGED INTO STAGE 3]
**Status:** ✅ Merged
**Note:** These features were implemented as part of Stage 3

### Features (Implemented in Stage 3)
- [x] Configurable trip delay via web UI (1-60s)
- [x] Configurable clear timeout via web UI (1-300s)
- [x] Configurable filter threshold via web UI (10-100%)
- [x] Parameter validation (enforced in POST /config handler)

---

## Stage 6: Full Diagnostics and System Integration [COMPLETED]
**Status:** ✅ Complete
**Directory:** `stage6_full_system/`
**Goal:** Add comprehensive monitoring and logging

### Features Implemented
- [x] Event logging (50-entry circular buffer)
- [x] Watchdog timer (30s hardware watchdog)
- [x] Memory monitoring (heap history, min/max tracking)
- [x] `/diagnostics` JSON endpoint
- [x] `/logs` JSON endpoint
- [x] Serial commands: `l` (logs), `d` (diagnostics)
- [x] Event types: boot, wifi, alarm, notification, config, reset
- [x] Web-based diagnostics dashboard page (`/diag`)
- [x] Updated API documentation page
- [x] Navigation bar on all pages

### Optional Future Enhancements
- [ ] 24-hour stability testing
- [ ] Persistent event log (SPIFFS/LittleFS)
- [ ] OTA firmware updates

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
