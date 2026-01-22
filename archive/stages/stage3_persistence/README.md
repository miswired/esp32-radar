# Stage 3: Persistent Configuration Storage

This stage builds on Stage 2 and adds persistent storage using the ESP32's NVS (Non-Volatile Storage).

## New Features in Stage 3

- **NVS Storage**: Configuration persists across reboots using ESP32 Preferences library
- **Persistent WiFi Credentials**: WiFi SSID and password stored in NVS
- **Persistent Settings**: Trip delay, clear timeout, and filter threshold saved to flash
- **Navigation Bar**: Sticky top navigation on all pages for easy access
- **Web Settings Page**: New `/settings` page for configuring the device
- **API Documentation Page**: New `/api` page with REST API documentation
- **POST /config API**: Save configuration via REST API
- **POST /reset API**: Factory reset endpoint
- **Serial Factory Reset**: Press 'f' in serial monitor for factory reset

## Prerequisites

### Libraries Required

Same as Stage 2:
```bash
arduino-cli lib install "ArduinoJson"
```

The Preferences library is built into the ESP32 Arduino core.

### Hardware

Same as Stage 1 and 2:
- ESP32-WROOM-32
- RCWL-0516 sensor (VIN→VIN, GND→GND, OUT→GPIO13)

## Configuration

**First boot behavior:**

1. If no configuration exists in NVS, defaults are loaded from `credentials.h`
2. Configuration is immediately saved to NVS
3. Subsequent boots load configuration from NVS

**Changing WiFi after first boot:**

1. Access the Settings page at `http://<ip>/settings`
2. Enter new WiFi SSID and password
3. Click "Save Settings"
4. Device will restart and connect to new network

**If WiFi connection fails:**

- Device falls back to AP mode (ESP32-Radar-Setup at 192.168.4.1)
- Connect to AP and access Settings page to configure correct WiFi

## Configurable Settings

| Setting | Range | Default | Description |
|---------|-------|---------|-------------|
| WiFi SSID | 1-63 chars | From credentials.h | WiFi network name |
| WiFi Password | 1-63 chars | From credentials.h | WiFi password |
| Trip Delay | 1-60 seconds | 3 | Motion duration before alarm triggers |
| Clear Timeout | 1-300 seconds | 10 | No-motion duration before alarm clears |
| Filter Threshold | 10-100% | 70 | Percentage of samples required for motion |

## Web Interface

All pages include a sticky navigation bar with links to Dashboard, Settings, and API documentation.

### Dashboard (`/`)

Real-time motion detection status display with:
- Current state (IDLE, MOTION_PENDING, ALARM_ACTIVE, ALARM_CLEARING)
- Motion indicator with filter level and threshold
- Statistics (alarm events, motion events, uptime, memory)
- Current configuration display
- WiFi connection info

### Settings Page (`/settings`)

- **WiFi Configuration**: Change WiFi SSID and password
- **Motion Detection**: Adjust trip delay, clear timeout, and filter threshold
- **Save Settings**: Saves to NVS (restarts if WiFi changed)
- **Factory Reset**: Clears all settings and restores defaults

### API Documentation Page (`/api`)

Built-in documentation for all REST API endpoints with:
- Endpoint descriptions and methods
- Request/response field tables
- Clickable links to GET endpoints (view live JSON)
- curl examples for POST endpoints

### REST API Endpoints

All Stage 2 endpoints are preserved:

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/` | Dashboard |
| GET | `/settings` | Settings page |
| GET | `/api` | API documentation |
| GET | `/status` | JSON status |
| GET | `/config` | JSON configuration |
| POST | `/config` | Save configuration |
| POST | `/reset` | Factory reset |

---

#### POST `/config`

Saves configuration settings to NVS.

**Request:**
```bash
curl -X POST http://192.168.1.100/config \
  -H "Content-Type: application/json" \
  -d '{
    "wifiSsid": "NewNetwork",
    "wifiPassword": "newpassword",
    "tripDelay": 5,
    "clearTimeout": 15,
    "filterThreshold": 80
  }'
```

**Request Fields:**

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `wifiSsid` | string | No | New WiFi SSID (max 63 chars) |
| `wifiPassword` | string | No | New WiFi password (max 63 chars). Leave empty to keep current |
| `tripDelay` | integer | No | Trip delay in seconds (1-60) |
| `clearTimeout` | integer | No | Clear timeout in seconds (1-300) |
| `filterThreshold` | integer | No | Filter threshold percentage (10-100) |

**Response:** `200 OK` with `Content-Type: application/json`

```json
{
  "success": true,
  "message": "Settings saved.",
  "restart": true
}
```

**Response Fields:**

| Field | Type | Description |
|-------|------|-------------|
| `success` | boolean | `true` if settings were saved |
| `message` | string | Status message |
| `restart` | boolean | `true` if device will restart (WiFi changed) |

**Notes:**
- If WiFi SSID or password is changed, the device restarts to apply changes
- Other settings (trip delay, clear timeout, filter threshold) take effect immediately
- Only include fields you want to change; others remain unchanged

---

#### POST `/reset`

Performs a factory reset, clearing all stored configuration.

**Request:**
```bash
curl -X POST http://192.168.1.100/reset
```

**Response:** `200 OK` with `Content-Type: application/json`

```json
{
  "success": true,
  "message": "Factory reset initiated"
}
```

After a factory reset:
1. All NVS data is cleared
2. Default settings from `credentials.h` are restored
3. Configuration is saved to NVS
4. Device restarts

---

## Serial Commands

All Stage 2 commands, plus:

| Command | Description |
|---------|-------------|
| `t` | Run self-test (includes NVS test) |
| `s` | Print statistics |
| `c` | Print configuration |
| `h` | Help menu |
| `r` | Restart ESP32 |
| `i` | System info |
| `f` | **Factory reset** (new in Stage 3) |

## NVS Storage Details

Configuration is stored in the `radar-config` namespace with key `config`.

**Storage structure:**
- WiFi SSID: 64 bytes
- WiFi Password: 64 bytes
- Trip Delay: 2 bytes
- Clear Timeout: 2 bytes
- Filter Threshold: 1 byte
- Checksum: 4 bytes

**Total: ~137 bytes**

The checksum validates data integrity. If checksum fails on load, defaults are restored.

## Compilation and Upload

```bash
# Compile
arduino-cli compile --fqbn esp32:esp32:esp32 stage3_persistence

# Upload
arduino-cli upload --fqbn esp32:esp32:esp32 -p /dev/ttyUSB0 stage3_persistence

# Monitor
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

Or all-in-one:
```bash
arduino-cli compile --fqbn esp32:esp32:esp32 stage3_persistence && \
arduino-cli upload --fqbn esp32:esp32:esp32 -p /dev/ttyUSB0 stage3_persistence && \
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

## Expected Serial Output

### First Boot (No Stored Config)

```
==========================================
ESP32 Microwave Motion Sensor - Stage 3
==========================================
Persistent Configuration Storage

Reset reason: POWERON_RESET

GPIO initialized
Loading configuration from NVS... No valid config found
Configuration set to defaults
Saving configuration to NVS... OK
Motion filter initialized: 10 samples, 70% threshold, 1000ms window

Connecting to WiFi: MyNetwork
.....
Connected! IP: 192.168.1.100
RSSI: -45 dBm

Web server started on port 80

------------------------------------------
Dashboard: http://192.168.1.100
Settings:  http://192.168.1.100/settings
------------------------------------------

=== STAGE 3 SELF TEST ===
  GPIO (pin 13): PASS (LOW)
  LED (pin 2): PASS (blinked)
  Heap (282 KB): PASS
  NVS: PASS (config stored)
  WiFi: PASS (STA, 192.168.1.100, RSSI: -45 dBm)
  Web Server: PASS (port 80)

Overall: ALL TESTS PASSED
=========================
```

### Subsequent Boots (Config Loaded)

```
Loading configuration from NVS... OK
  WiFi SSID: MyNetwork
  Trip Delay: 3s
  Clear Timeout: 10s
  Filter Threshold: 70%
```

## Memory Usage

| Component | Approximate Size |
|-----------|-----------------|
| Base ESP32 | ~320 KB free |
| WiFi stack | ~40 KB |
| Web server | ~20-40 KB |
| Preferences/NVS | ~5 KB |
| **Available** | **~235-275 KB** |

## Troubleshooting

### Configuration Not Saving

1. Check serial output for "Saving configuration to NVS... OK"
2. If it says "FAILED", NVS partition may be corrupted
3. Try factory reset via serial command 'f'

### WiFi Not Connecting After Change

1. Connect to ESP32-Radar-Setup AP
2. Access http://192.168.4.1/settings
3. Verify SSID and password are correct
4. Save and wait for restart

### Settings Reset Unexpectedly

1. Checksum validation may have failed
2. Check if brown-out resets are occurring
3. Ensure stable power supply

### NVS Partition Issues

If NVS becomes corrupted, you can erase it via esptool:
```bash
esptool.py --port /dev/ttyUSB0 erase_region 0x9000 0x5000
```

Then re-upload the sketch. First boot will create fresh defaults.

## Next Steps

Stage 3 complete? Proceed to:

**Stage 4: Notification System**
- HTTP GET/POST notifications
- Raw TCP socket notifications
- Notification configuration via web UI
- Test notification button
