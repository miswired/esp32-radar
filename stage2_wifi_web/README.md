# Stage 2: WiFi and Web Interface

This stage builds on Stage 1 and adds WiFi connectivity with AP fallback and a web-based dashboard.

## New Features in Stage 2

- **WiFi Station Mode**: Connects to your existing WiFi network
- **AP Fallback Mode**: Creates its own network if WiFi connection fails
- **Web Dashboard**: Real-time motion status display
- **REST API**: JSON endpoints for status and configuration
- **Motion Filter**: Noise reduction requiring sustained sensor readings
- **WiFi Self-Tests**: Verify WiFi and web server functionality

## Prerequisites

### Libraries Required

Install via Arduino CLI:
```bash
arduino-cli lib install "ArduinoJson"
```

The built-in ESP32 WebServer library is used (no additional web server library needed).

### Hardware

Same as Stage 1:
- ESP32-WROOM-32
- RCWL-0516 sensor (VIN→VIN, GND→GND, OUT→GPIO13)

## Configuration

**Before uploading**, create your credentials file:

```bash
# Copy the example file
cp credentials.h.example credentials.h

# Edit with your WiFi credentials
nano credentials.h
```

Edit `credentials.h` with your network settings:

```cpp
const char* WIFI_SSID = "YOUR_WIFI_SSID";      // Your WiFi network name
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";  // Your WiFi password
```

**Note:** `credentials.h` is in `.gitignore` and will never be committed to the repository.
This keeps your WiFi credentials private when sharing the code.

If you don't configure WiFi (or it fails to connect), the device will start in AP mode.

## WiFi Modes

### Station Mode (STA)
- Connects to your existing WiFi network
- Gets IP address from your router (shown in serial output)
- Access dashboard at: `http://<assigned-ip>/`

### Access Point Mode (AP) - Fallback
- Creates network: **ESP32-Radar-Setup**
- Password: **12345678**
- IP Address: **192.168.4.1**
- Access dashboard at: `http://192.168.4.1/`

AP mode activates automatically if:
- WiFi credentials are not configured
- WiFi connection fails after 15 seconds
- WiFi network is not available

## Motion Filter

The RCWL-0516 sensor can be noisy, triggering on small movements or interference. The motion filter reduces false positives by requiring a **percentage of positive readings** over a sliding time window before reporting motion.

### How It Works

1. Sensor is sampled at 10Hz (every 100ms)
2. Last 10 samples are kept in a circular buffer (1 second window)
3. Motion is only reported if **70% or more** of samples are HIGH
4. This filters out brief noise spikes while still detecting sustained motion

### Filter Configuration

Edit these values in `stage2_wifi_web.ino` to tune sensitivity:

```cpp
#define FILTER_SAMPLE_COUNT 10          // Number of samples in window
#define FILTER_THRESHOLD_PERCENT 70     // % of samples that must be HIGH
#define FILTER_WINDOW_MS 1000           // Time window in milliseconds
```

### Tuning Guide

| Scenario | Adjustment |
|----------|------------|
| Too many false triggers | Increase `FILTER_THRESHOLD_PERCENT` (e.g., 80-90%) |
| Missing real motion | Decrease `FILTER_THRESHOLD_PERCENT` (e.g., 50-60%) |
| Need faster response | Decrease `FILTER_WINDOW_MS` (e.g., 500ms) |
| Need more stability | Increase `FILTER_WINDOW_MS` (e.g., 2000ms) |

### Detection Pipeline

The full detection pipeline with filter:

```
Raw Sensor → Motion Filter → Trip Delay → Alarm State
   10Hz        70% threshold    3 seconds    Active/Clearing
```

1. **Raw Sensor**: Reads HIGH/LOW at 10Hz
2. **Motion Filter**: Requires 70% HIGH over 1 second
3. **Trip Delay**: Filtered motion must sustain 3 seconds
4. **Alarm State**: Triggers notifications (future stages)

## Web Interface

### Dashboard (`/`)

The main dashboard shows:
- **Current State**: IDLE, MOTION_PENDING, ALARM_ACTIVE, ALARM_CLEARING
- **Motion Indicator**: Real-time sensor status
- **Statistics**: Alarm events, motion events, uptime, free memory
- **Configuration**: Trip delay, clear timeout
- **WiFi Info**: Mode, IP address, signal strength

The dashboard auto-refreshes every second.

### REST API Endpoints

The ESP32 provides a REST API for integration with home automation systems, scripts, or custom applications.

---

#### GET `/status`

Returns the current motion detection status and system metrics.

**Request:**
```bash
curl http://192.168.1.100/status
```

**Response:** `200 OK` with `Content-Type: application/json`

```json
{
  "state": "IDLE",
  "rawMotion": false,
  "filteredMotion": false,
  "filterPercent": 20,
  "alarmEvents": 5,
  "motionEvents": 12,
  "uptime": 3600,
  "freeHeap": 245000,
  "tripDelay": 3,
  "clearTimeout": 30,
  "filterThreshold": 70,
  "wifiMode": "Station",
  "ipAddress": "192.168.1.100",
  "rssi": -45
}
```

**Response Fields:**

| Field | Type | Description |
|-------|------|-------------|
| `state` | string | Current state machine state (see below) |
| `rawMotion` | boolean | `true` if raw sensor currently reads HIGH |
| `filteredMotion` | boolean | `true` if filtered motion detected (used by state machine) |
| `filterPercent` | integer | Current percentage of positive samples (0-100) |
| `alarmEvents` | integer | Total number of alarm triggers since boot |
| `motionEvents` | integer | Total motion detection events since boot |
| `uptime` | integer | Seconds since ESP32 boot |
| `freeHeap` | integer | Available heap memory in bytes |
| `tripDelay` | integer | Seconds motion must sustain before alarm |
| `clearTimeout` | integer | Seconds without motion before alarm clears |
| `filterThreshold` | integer | Percentage threshold for filtered motion |
| `wifiMode` | string | `"Station"` or `"AP Mode"` |
| `ipAddress` | string | Current IP address |
| `rssi` | integer | WiFi signal strength in dBm (0 in AP mode) |

**State Values:**

| State | Description |
|-------|-------------|
| `IDLE` | No motion detected, system waiting |
| `MOTION_PENDING` | Motion detected, waiting for trip delay |
| `ALARM_ACTIVE` | Alarm triggered, motion sustained past trip delay |
| `ALARM_CLEARING` | Motion stopped, waiting for clear timeout |

---

#### GET `/config`

Returns the current system configuration.

**Request:**
```bash
curl http://192.168.1.100/config
```

**Response:** `200 OK` with `Content-Type: application/json`

```json
{
  "sensorPin": 13,
  "ledPin": 2,
  "tripDelaySeconds": 3,
  "clearTimeoutSeconds": 30,
  "pollIntervalMs": 100,
  "filterSampleCount": 10,
  "filterThresholdPercent": 70,
  "filterWindowMs": 1000,
  "wifiSsid": "MyNetwork",
  "wifiMode": "Station",
  "apSsid": "ESP32-Radar-Setup",
  "ipAddress": "192.168.1.100"
}
```

**Response Fields:**

| Field | Type | Description |
|-------|------|-------------|
| `sensorPin` | integer | GPIO pin connected to RCWL-0516 OUT |
| `ledPin` | integer | GPIO pin for status LED |
| `tripDelaySeconds` | integer | Motion duration before alarm triggers |
| `clearTimeoutSeconds` | integer | No-motion duration before alarm clears |
| `pollIntervalMs` | integer | Sensor polling rate in milliseconds |
| `filterSampleCount` | integer | Number of samples in filter window |
| `filterThresholdPercent` | integer | Percentage threshold for motion detection |
| `filterWindowMs` | integer | Filter time window in milliseconds |
| `wifiSsid` | string | Connected WiFi network name |
| `wifiMode` | string | `"Station"` or `"AP Mode"` |
| `apSsid` | string | Access point SSID (when in AP mode) |
| `ipAddress` | string | Current IP address |

---

### API Usage Examples

#### Poll status every 5 seconds (bash)
```bash
while true; do
  curl -s http://192.168.1.100/status | jq '.state, .rawMotion'
  sleep 5
done
```

#### Check if alarm is active (bash)
```bash
STATE=$(curl -s http://192.168.1.100/status | jq -r '.state')
if [ "$STATE" = "ALARM_ACTIVE" ]; then
  echo "ALARM!"
fi
```

#### Python integration example
```python
import requests
import time

ESP32_IP = "192.168.1.100"

def get_motion_status():
    response = requests.get(f"http://{ESP32_IP}/status")
    return response.json()

# Monitor for alarm events
last_alarm_count = 0
while True:
    status = get_motion_status()
    if status["alarmEvents"] > last_alarm_count:
        print(f"New alarm triggered! Total: {status['alarmEvents']}")
        last_alarm_count = status["alarmEvents"]
    time.sleep(1)
```

#### Home Assistant REST sensor
```yaml
# configuration.yaml
sensor:
  - platform: rest
    name: "Motion Sensor State"
    resource: http://192.168.1.100/status
    value_template: "{{ value_json.state }}"
    scan_interval: 1

  - platform: rest
    name: "Motion Detected"
    resource: http://192.168.1.100/status
    value_template: "{{ value_json.rawMotion }}"
    scan_interval: 1

binary_sensor:
  - platform: template
    sensors:
      radar_motion:
        friendly_name: "Radar Motion"
        value_template: "{{ states('sensor.motion_detected') == 'true' }}"
```

#### Node-RED HTTP request
```
[Inject] -> [HTTP Request: GET http://192.168.1.100/status] -> [JSON Parser] -> [Switch: msg.payload.state]
```

---

## Serial Commands

Same as Stage 1, plus WiFi info:

| Command | Description |
|---------|-------------|
| `t` | Run self-test (includes WiFi tests) |
| `s` | Print statistics |
| `c` | Print configuration |
| `h` | Help menu |
| `r` | Restart ESP32 |
| `i` | System info |

## Compilation and Upload

```bash
# Compile
arduino-cli compile --fqbn esp32:esp32:esp32 stage2_wifi_web

# Upload
arduino-cli upload --fqbn esp32:esp32:esp32 -p /dev/ttyUSB0 stage2_wifi_web

# Monitor
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

Or all-in-one:
```bash
arduino-cli compile --fqbn esp32:esp32:esp32 stage2_wifi_web && \
arduino-cli upload --fqbn esp32:esp32:esp32 -p /dev/ttyUSB0 stage2_wifi_web && \
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

## Expected Serial Output

### Station Mode (WiFi Connected)
```
==========================================
ESP32 Microwave Motion Sensor - Stage 2
==========================================
WiFi + Web Interface

Reset reason: POWERON_RESET

GPIO initialized

Connecting to WiFi: MyNetwork
.....
Connected! IP: 192.168.1.100
RSSI: -45 dBm

Web server started on port 80

------------------------------------------
Dashboard: http://192.168.1.100
------------------------------------------

=== STAGE 2 SELF TEST ===
  GPIO (pin 13): PASS (LOW)
  LED (pin 2): PASS (blinked)
  Heap (289 KB): PASS
  WiFi: PASS (STA, 192.168.1.100, RSSI: -45 dBm)
  Web Server: PASS (port 80)

Overall: ALL TESTS PASSED
=========================
```

### AP Mode (Fallback)
```
Connecting to WiFi: YOUR_WIFI_SSID
...............
WiFi connection timeout!
Starting AP Mode...

=================================
  AP MODE ACTIVE
=================================
  SSID: ESP32-Radar-Setup
  Password: 12345678
  IP: 192.168.4.1
=================================

Connect to this network and open
http://192.168.4.1 in your browser
```

## Self-Test Results

Stage 2 adds these tests:

| Test | Checks |
|------|--------|
| WiFi | Connection mode (STA/AP), IP address, signal strength |
| Web Server | Server started on port 80 |

## Troubleshooting

### WiFi Won't Connect

1. **Check credentials**: Verify SSID and password in code
2. **Check network**: Ensure 2.4GHz network (ESP32 doesn't support 5GHz)
3. **Check signal**: Move closer to router
4. **Use AP mode**: Connect to ESP32-Radar-Setup for initial testing

### Can't Access Web Dashboard

1. **Check IP address**: Shown in serial output
2. **Same network**: Ensure your device is on same network as ESP32
3. **Firewall**: Check if firewall is blocking port 80
4. **Try AP mode**: Connect directly to ESP32's network

### Web Page Doesn't Update

1. **Check browser console**: Look for JavaScript errors
2. **Hard refresh**: Ctrl+Shift+R or Cmd+Shift+R
3. **Try different browser**: Some browsers cache aggressively

### High Memory Usage

The web server uses more memory than Stage 1. Expected free heap:
- Stage 1: ~305 KB
- Stage 2: ~240-280 KB

If heap drops below 50KB, there may be a memory leak.

## Memory Usage

| Component | Approximate Size |
|-----------|-----------------|
| Base ESP32 | ~320 KB free |
| WiFi stack | ~40 KB |
| Web server | ~20-40 KB |
| **Available** | **~240-280 KB** |

## Next Steps

Stage 2 complete? Proceed to:

**Stage 3: Persistent Configuration**
- Save WiFi credentials to NVS
- Save trip delay/clear timeout settings
- Web interface for configuration changes
