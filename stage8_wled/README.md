# Stage 8: WLED Integration

This stage builds on Stage 7 and adds WLED device integration for visual alarm feedback, plus refactors notification methods to use checkboxes.

## New Features in Stage 8

### WLED Integration
- **WLED Device Support**: Send custom JSON payloads to WLED devices when alarms trigger
- **Configurable URL**: Full URL field for any WLED device endpoint
- **Custom JSON Payload**: Editable textarea for user-defined payloads
- **Default Payload**: Pre-populated example that sets WLED to solid red at full brightness
- **JSON Validation**: Client and server-side validation prevents invalid payloads
- **Test Button**: Verify WLED configuration without triggering motion

### Notification Method Refactor
- **Checkbox-Based Selection**: Replaced dropdown with independent checkboxes
- **Multi-Method Support**: Enable GET only, POST only, both, or neither
- **Consistent UI Pattern**: Matches WLED checkbox pattern for future extensibility

## Prerequisites

### Libraries Required

Same as Stage 7:
```bash
arduino-cli lib install "ArduinoJson"
```

### Hardware

Same as previous stages:
- ESP32-WROOM-32
- RCWL-0516 sensor (VIN->VIN, GND->GND, OUT->GPIO13)
- (Optional) WLED-enabled LED controller on network

## WLED Configuration

### Overview

WLED integration allows the motion sensor to trigger visual effects on WLED-compatible LED controllers when an alarm triggers. This is completely separate from the HTTP notification system, allowing you to have both visual alerts AND webhook notifications.

### Configuration Fields

| Field | Description | Max Length |
|-------|-------------|------------|
| Enable WLED | Checkbox to enable/disable WLED integration | - |
| WLED URL | Full URL to WLED JSON API endpoint | 128 chars |
| JSON Payload | Custom JSON payload to send | 512 chars |

### Default Payload

The default payload sets WLED to solid red at full brightness:
```json
{"on":true,"bri":255,"seg":[{"col":[[255,0,0]]}]}
```

### Common Payload Examples

**Solid Red (Default):**
```json
{"on":true,"bri":255,"seg":[{"col":[[255,0,0]]}]}
```

**Flash Effect:**
```json
{"on":true,"bri":255,"seg":[{"fx":25,"col":[[255,0,0]]}]}
```

**Blue Police Lights:**
```json
{"on":true,"bri":255,"seg":[{"fx":1,"col":[[0,0,255],[255,0,0]]}]}
```

**Low Brightness Warning:**
```json
{"on":true,"bri":100,"seg":[{"col":[[255,165,0]]}]}
```

### URL Format

The WLED URL should point to the JSON state endpoint:
```
http://192.168.1.50/json/state
```

Replace `192.168.1.50` with your WLED device's IP address.

## Notification Methods

### Checkbox Selection

The notification method selection has been refactored from a dropdown to checkboxes:

| Option | Description |
|--------|-------------|
| Send GET request | Appends `?event=...&state=...` to URL |
| Send POST request | Sends JSON body with event data |

You can now:
- Enable GET only
- Enable POST only
- Enable both GET and POST (sends two requests)
- Disable both (no notifications sent)

### POST Request Body

When POST is enabled, the following JSON is sent:
```json
{
  "event": "alarm_triggered",
  "state": "ALARM_ACTIVE",
  "ip": "192.168.1.100",
  "uptime": 3600,
  "alarmEvents": 5
}
```

### GET Request Parameters

When GET is enabled, these query parameters are appended:
```
?event=alarm_triggered&state=ALARM_ACTIVE&ip=192.168.1.100
```

## REST API

### GET /config (Updated)

Now includes WLED and notification method fields:

```json
{
  "wifiSsid": "MyNetwork",
  "tripDelaySeconds": 3,
  "clearTimeoutSeconds": 10,
  "filterThresholdPercent": 70,
  "notifyEnabled": true,
  "notifyUrl": "http://example.com/webhook",
  "notifyGet": true,
  "notifyPost": false,
  "wledEnabled": true,
  "wledUrl": "http://192.168.1.50/json/state",
  "wledPayload": "{\"on\":true,\"bri\":255,\"seg\":[{\"col\":[[255,0,0]]}]}",
  "authEnabled": false,
  "authConfigured": true
}
```

### POST /config (Updated)

Accepts new WLED and notification method fields:

```json
{
  "notifyGet": true,
  "notifyPost": true,
  "wledEnabled": true,
  "wledUrl": "http://192.168.1.50/json/state",
  "wledPayload": "{\"on\":true,\"bri\":255,\"seg\":[{\"col\":[[255,0,0]]}]}"
}
```

**Error Response (Invalid JSON Payload):**
```json
{"success": false, "message": "Invalid WLED JSON payload"}
```

### POST /test-notification (Updated)

Now accepts checkbox-based method selection:

**Request:**
```json
{
  "url": "http://example.com/webhook",
  "notifyGet": true,
  "notifyPost": true
}
```

**Response (both methods):**
```json
{
  "success": true,
  "httpCode": 200,
  "message": "GET: 200, POST: 200"
}
```

### POST /test-wled (New)

Test WLED configuration without triggering motion.

**Request (using form values):**
```json
{
  "url": "http://192.168.1.50/json/state",
  "payload": "{\"on\":true,\"bri\":255,\"seg\":[{\"col\":[[255,0,0]]}]}"
}
```

**Request (using saved config):**
```json
{}
```

**Response (success):**
```json
{
  "success": true,
  "httpCode": 200,
  "message": "WLED payload sent successfully"
}
```

**Response (failure):**
```json
{
  "success": false,
  "httpCode": -1,
  "message": "Connection failed"
}
```

**Authentication:** Requires session cookie OR API key (same as other POST endpoints).

## Event Types

New events added in Stage 8:

| Event | Data | Description |
|-------|------|-------------|
| `WLED_SENT` | HTTP code | WLED payload sent successfully |
| `WLED_FAILED` | Error code | WLED payload failed to send |

## Settings Page UI

### Notifications Section
- Enable Notifications checkbox
- Notification URL text field
- **Send GET request** checkbox (new)
- **Send POST request** checkbox (new)
- Test Notification button

### WLED Integration Section (New)
- Enable WLED checkbox
- WLED URL text field with placeholder example
- JSON Payload textarea (monospace, 5 rows)
- JSON validation error display
- Test WLED button

## Trigger Behavior

When an alarm triggers (after trip delay, entering ALARM_ACTIVE state):

1. **Notification System**: If enabled, sends GET and/or POST requests based on checkbox selection
2. **WLED System**: If enabled, sends JSON payload to configured WLED device

WLED payload is only sent when alarm triggers, NOT when it clears (fire-and-forget).

## Compilation and Upload

```bash
# Compile
arduino-cli compile --fqbn esp32:esp32:esp32 stage8_wled

# Upload
arduino-cli upload --fqbn esp32:esp32:esp32 -p /dev/ttyUSB0 stage8_wled

# Monitor
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

## Expected Serial Output

### Boot Sequence

```
==========================================
ESP32 Microwave Motion Sensor - Stage 8
==========================================
WLED Integration

Reset reason: POWERON_RESET

GPIO initialized
Loading configuration from NVS... OK
WLED: Disabled
Notification: GET=false, POST=false
Motion filter initialized

Connecting to WiFi: MyNetwork
.....
Connected! IP: 192.168.1.100
RSSI: -45 dBm
[LOG] WIFI_CONNECTED (-45)

Web server started on port 80
```

### WLED Events

```
[WLED] Sending payload to http://192.168.1.50/json/state
[WLED] Response: 200
[LOG] WLED_SENT (200)
```

```
[WLED] Error: -1 (connection refused)
[LOG] WLED_FAILED (-1)
```

## Memory Usage

| Component | Approximate Size |
|-----------|------------------|
| Program Storage | 1,177 KB (89%) |
| Global Variables | 49 KB (14%) |
| **Available RAM** | **~278 KB** |

## Troubleshooting

### WLED not responding

1. Verify WLED device is powered on and connected to WiFi
2. Check the URL format: `http://IP_ADDRESS/json/state`
3. Test the URL directly in a browser or with curl
4. Use the "Test WLED" button to verify connectivity
5. Check WLED device firewall settings

### JSON validation errors

1. Ensure proper JSON syntax (matching braces, quoted strings)
2. Use a JSON validator to check your payload
3. Check for special characters that may need escaping
4. Verify payload is under 512 characters

### Notification method not working

1. Ensure at least one checkbox (GET or POST) is selected
2. Verify the notification URL is correct
3. Use the "Test Notification" button to verify
4. Check server logs for incoming requests

### Both GET and POST not sending

1. Both checkboxes must be checked
2. Test each method individually first
3. Some servers may reject rapid consecutive requests
4. Check the response message for individual status codes

## WLED Setup Guide

### Quick Start

1. Flash WLED firmware to an ESP8266/ESP32 with LED strip
2. Connect WLED to your WiFi network
3. Note the WLED device's IP address
4. In the motion sensor settings:
   - Enable WLED checkbox
   - Enter URL: `http://WLED_IP/json/state`
   - Customize payload or use default
5. Click "Test WLED" to verify
6. Save settings

### Finding WLED IP

- Check your router's DHCP client list
- WLED displays IP on connected LED strip briefly on boot
- Use network scanning tools like `nmap` or `arp-scan`

### WLED Payload Reference

For full payload options, see: https://kno.wled.ge/interfaces/json-api/

Common segment options:
- `on`: true/false - Turn segment on/off
- `bri`: 0-255 - Brightness
- `col`: RGB array - Colors
- `fx`: 0-117 - Effect ID
- `sx`: 0-255 - Effect speed
- `ix`: 0-255 - Effect intensity

## Integration Examples

### Home Assistant with Both Methods

```yaml
rest_command:
  radar_get:
    url: "http://192.168.1.100/test-notification"
    method: POST
    headers:
      X-API-Key: "your-api-key"
      Content-Type: "application/json"
    payload: '{"notifyGet": true, "notifyPost": false}'
```

### WLED Automation

Configure WLED to restore normal state after alarm:
- Set WLED transition time for gradual fade
- Use WLED presets for different alarm states
- Combine with Home Assistant for state restoration

## Next Steps

Stage 8 adds visual feedback capabilities. Future enhancements could include:

- Multiple WLED device support
- State restoration when alarm clears
- Different payloads for trigger vs clear events
- WLED preset selection UI
- Integration with WLED segments for multi-zone alerts
