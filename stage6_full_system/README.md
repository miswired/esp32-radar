# Stage 6: Full Diagnostics and System Integration

This stage builds on Stage 4 and adds comprehensive monitoring, logging, and system stability features.

## New Features in Stage 6

- **Event Logging**: Circular buffer storing last 50 system events
- **Watchdog Timer**: 30-second hardware watchdog for system stability
- **Memory Monitoring**: Heap tracking with min/max history over time
- **Diagnostics API**: New `/diagnostics` and `/logs` endpoints
- **Enhanced Serial Commands**: New `l` (logs) and `d` (diagnostics) commands

## Prerequisites

### Libraries Required

Same as Stage 4:
```bash
arduino-cli lib install "ArduinoJson"
```

### Hardware

Same as previous stages:
- ESP32-WROOM-32
- RCWL-0516 sensor (VIN→VIN, GND→GND, OUT→GPIO13)

## Event Logging

The system logs significant events to a 50-entry circular buffer:

| Event | Description |
|-------|-------------|
| `BOOT` | System boot (includes initial heap size) |
| `WIFI_CONNECTED` | WiFi connection established (includes RSSI) |
| `WIFI_DISCONNECTED` | WiFi connection lost |
| `WIFI_AP_MODE` | Fallback to AP mode |
| `ALARM_TRIGGERED` | Alarm activated (includes alarm count) |
| `ALARM_CLEARED` | Alarm cleared (includes duration in seconds) |
| `NOTIFY_SENT` | Notification sent successfully (includes HTTP code) |
| `NOTIFY_FAILED` | Notification failed (includes error code) |
| `CONFIG_SAVED` | Configuration saved to NVS |
| `FACTORY_RESET` | Factory reset initiated |
| `WDT_RESET` | Watchdog triggered reset |
| `HEAP_LOW` | Free heap below threshold (includes heap size) |

## Watchdog Timer

A 30-second hardware watchdog timer is enabled to ensure system stability:

- Resets the ESP32 if the main loop stops responding
- Automatically fed every loop iteration
- Triggers a panic and restart if not fed within 30 seconds

## Memory Monitoring

The system tracks heap memory usage:

- **Heap Samples**: Recorded every 60 seconds (up to 60 samples = 1 hour)
- **Min/Max Tracking**: Records lowest and highest heap values seen
- **Low Heap Warning**: Logs an event if heap drops below 50KB

## REST API

### New Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/logs` | Full event log as JSON |
| GET | `/diagnostics` | System diagnostics as JSON |

---

#### GET `/logs`

Returns all logged events.

**Response:**
```json
{
  "events": [
    {
      "time": 1234,
      "uptime": "00:00:01",
      "event": "BOOT",
      "data": 285000
    },
    {
      "time": 5678,
      "uptime": "00:00:05",
      "event": "WIFI_CONNECTED",
      "data": -45
    }
  ],
  "count": 2,
  "maxSize": 50
}
```

---

#### GET `/diagnostics`

Returns comprehensive system diagnostics.

**Response:**
```json
{
  "uptime": 3600000,
  "uptimeFormatted": "01:00:00",
  "freeHeap": 250000,
  "minHeap": 240000,
  "maxHeap": 285000,
  "heapFragmentation": 5,
  "watchdogEnabled": true,
  "watchdogTimeout": 30,
  "wifiMode": "Station",
  "wifiConnected": true,
  "rssi": -45,
  "ssid": "MyNetwork",
  "ip": "192.168.1.100",
  "alarmEvents": 5,
  "motionEvents": 100,
  "notificationsSent": 5,
  "notificationsFailed": 0,
  "state": "IDLE",
  "motionFiltered": false,
  "filterLevel": 0,
  "heapHistory": [285000, 284000, 283000],
  "recentEvents": [
    {"uptime": "00:00:01", "event": "BOOT"},
    {"uptime": "00:00:05", "event": "WIFI_CONNECTED"}
  ]
}
```

## Serial Commands

All Stage 4 commands, plus:

| Command | Description |
|---------|-------------|
| `t` | Run self-test |
| `s` | Print statistics |
| `c` | Print configuration |
| `l` | **Print event log** (new) |
| `d` | **Print diagnostics** (new) |
| `h` | Help menu |
| `r` | Restart ESP32 |
| `i` | System info |
| `f` | Factory reset |

## Web Interface

All pages include navigation bar with links to:
- **Dashboard** (`/`) - Real-time status
- **Settings** (`/settings`) - Configuration
- **API** (`/api`) - API documentation

New JSON endpoints available:
- **Logs** (`/logs`) - Event log
- **Diagnostics** (`/diagnostics`) - System health

## Compilation and Upload

```bash
# Compile
arduino-cli compile --fqbn esp32:esp32:esp32 stage6_full_system

# Upload
arduino-cli upload --fqbn esp32:esp32:esp32 -p /dev/ttyUSB0 stage6_full_system

# Monitor
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

## Expected Serial Output

### Boot Sequence

```
==========================================
ESP32 Microwave Motion Sensor - Stage 6
==========================================
Full Diagnostics and System Integration

Reset reason: POWERON_RESET

GPIO initialized
Loading configuration from NVS... OK
Motion filter initialized

Connecting to WiFi: MyNetwork
.....
Connected! IP: 192.168.1.100
RSSI: -45 dBm
[LOG] WIFI_CONNECTED (-45)

Web server started on port 80
Watchdog timer initialized (30s timeout)
[LOG] BOOT (285000)

------------------------------------------
Dashboard:    http://192.168.1.100
Settings:     http://192.168.1.100/settings
Diagnostics:  http://192.168.1.100/diagnostics
------------------------------------------

=== STAGE 6 SELF TEST ===
  GPIO (pin 13): PASS
  LED (pin 2): PASS
  Heap (278 KB): PASS
  NVS: PASS
  WiFi: PASS
  Web Server: PASS

Overall: ALL TESTS PASSED
=========================
```

### Heap Monitoring

```
[HEAP] 278 KB (min: 275 KB) | State: IDLE | Alarms: 0
```

### Event Logging

```
[LOG] ALARM_TRIGGERED (1)
[LOG] NOTIFY_SENT (200)
[LOG] ALARM_CLEARED (45)
```

## Memory Usage

| Component | Approximate Size |
|-----------|--------------------|
| Base ESP32 | ~320 KB free |
| WiFi stack | ~40 KB |
| Web server | ~20-40 KB |
| HTTPClient | ~10 KB |
| Event log buffer | ~1 KB |
| Heap history | ~240 bytes |
| **Available** | **~220-260 KB** |

## Troubleshooting

### Watchdog Reset

If the ESP32 is resetting unexpectedly:
1. Check serial output for "WDT_RESET" in boot reason
2. Event log will show last events before reset
3. Look for blocking operations or infinite loops

### Memory Issues

If heap is dropping over time:
1. Check `/diagnostics` endpoint for heap history
2. Look for memory leak patterns
3. Event log will show HEAP_LOW warnings below 50KB

### Missing Events

The event log is a circular buffer:
- Only last 50 events are stored
- Oldest events are overwritten
- Events are lost on reboot (not persisted)

## Next Steps

Stage 6 focuses on stability and monitoring. Future enhancements could include:

- Persistent event log (store to SPIFFS/LittleFS)
- Web-based diagnostics dashboard
- OTA (Over-The-Air) firmware updates
- MQTT integration for remote monitoring
