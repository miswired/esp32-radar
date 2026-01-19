# Stage 4: Notification System

This stage builds on Stage 3 and adds HTTP notifications when alarm events occur.

## New Features in Stage 4

- **HTTP Notifications**: Send HTTP requests when alarm triggers or clears
- **GET and POST Methods**: Choose between query parameters (GET) or JSON body (POST)
- **Configurable URL**: Set your webhook, Home Assistant, or any HTTP endpoint
- **Test Notification Button**: Test your configuration without triggering an alarm
- **Notification Statistics**: Track sent and failed notifications
- **Persistent Settings**: Notification settings saved to NVS

## Prerequisites

### Libraries Required

Same as Stage 3:
```bash
arduino-cli lib install "ArduinoJson"
```

HTTPClient is built into the ESP32 Arduino core.

### Hardware

Same as previous stages:
- ESP32-WROOM-32
- RCWL-0516 sensor (VIN→VIN, GND→GND, OUT→GPIO13)

## Notification Configuration

### Via Web Interface

1. Navigate to Settings page (`/settings`)
2. Scroll to Notifications section
3. Check "Enable HTTP notifications"
4. Enter your webhook URL
5. Select method (GET or POST)
6. Click "Test Notification" to verify
7. Click "Save Settings"

### Notification Events

Notifications are sent on two events:

| Event | When | State |
|-------|------|-------|
| `alarm_triggered` | Alarm activates (motion sustained past trip delay) | `ALARM_ACTIVE` |
| `alarm_cleared` | Alarm clears (no motion past clear timeout) | `IDLE` |

### GET Method

Appends query parameters to your URL:

```
http://your-url.com/webhook?event=alarm_triggered&state=ALARM_ACTIVE&ip=192.168.1.100
```

**Parameters:**
- `event` - Either `alarm_triggered`, `alarm_cleared`, or `test`
- `state` - Current state (IDLE, MOTION_PENDING, ALARM_ACTIVE, ALARM_CLEARING)
- `ip` - Device IP address

### POST Method

Sends JSON body to your URL:

```json
{
  "event": "alarm_triggered",
  "state": "ALARM_ACTIVE",
  "ip": "192.168.1.100",
  "uptime": 3600,
  "alarmEvents": 5
}
```

**Fields:**
- `event` - Either `alarm_triggered`, `alarm_cleared`, or `test`
- `state` - Current state
- `ip` - Device IP address
- `uptime` - Seconds since boot
- `alarmEvents` - Total alarm count

## Integration Examples

### Home Assistant Webhook

1. Create an automation in Home Assistant with webhook trigger
2. Note the webhook ID
3. Configure ESP32 with URL: `http://homeassistant.local:8123/api/webhook/YOUR_WEBHOOK_ID`
4. Use POST method for JSON data

### IFTTT

1. Create an IFTTT applet with Webhooks trigger
2. Get your webhook key from IFTTT
3. Configure ESP32 with URL: `https://maker.ifttt.com/trigger/motion_alarm/with/key/YOUR_KEY`
4. Use GET or POST method

### Node-RED

1. Add HTTP In node (POST method, URL: `/motion-alert`)
2. Configure ESP32 with URL: `http://node-red-ip:1880/motion-alert`
3. Use POST method
4. Parse JSON in Node-RED for automation

### Simple Logging Server

```python
from flask import Flask, request
app = Flask(__name__)

@app.route('/motion', methods=['POST'])
def motion():
    data = request.json
    print(f"Motion event: {data['event']} - State: {data['state']}")
    return 'OK', 200

app.run(host='0.0.0.0', port=5000)
```

## Web Interface

### Navigation

All pages include navigation bar with links to:
- **Dashboard** (`/`) - Real-time status
- **Settings** (`/settings`) - Configuration
- **API** (`/api`) - API documentation

### Settings Page Additions

The Settings page now includes a Notifications section:
- Enable/disable checkbox
- URL input field
- Method dropdown (GET/POST)
- Test Notification button with status feedback

## REST API

### New/Modified Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| POST | `/test-notification` | Send test notification |
| GET | `/status` | Now includes notification stats |
| GET | `/config` | Now includes notification settings |
| POST | `/config` | Now accepts notification settings |

---

#### POST `/test-notification`

Sends a test notification using current URL and method settings.

**Request:**
```bash
curl -X POST http://192.168.1.100/test-notification
```

**Response:**
```json
{
  "success": true,
  "httpCode": 200,
  "message": "Notification sent"
}
```

**Error Response:**
```json
{
  "success": false,
  "message": "No notification URL configured"
}
```

---

#### GET `/status` (Updated)

Now includes notification statistics:

```json
{
  "state": "IDLE",
  "...": "...",
  "notifyEnabled": true,
  "notificationsSent": 5,
  "notificationsFailed": 0,
  "lastNotifyStatus": 200
}
```

---

#### GET `/config` (Updated)

Now includes notification configuration:

```json
{
  "...": "...",
  "notifyEnabled": true,
  "notifyUrl": "http://example.com/webhook",
  "notifyMethod": 1
}
```

- `notifyMethod`: 0 = GET, 1 = POST

---

#### POST `/config` (Updated)

Now accepts notification settings:

```bash
curl -X POST http://192.168.1.100/config \
  -H "Content-Type: application/json" \
  -d '{
    "notifyEnabled": true,
    "notifyUrl": "http://example.com/webhook",
    "notifyMethod": 1
  }'
```

## Serial Commands

Same as Stage 3:

| Command | Description |
|---------|-------------|
| `t` | Run self-test |
| `s` | Print statistics |
| `c` | Print configuration |
| `h` | Help menu |
| `r` | Restart ESP32 |
| `i` | System info |
| `f` | Factory reset |

## Compilation and Upload

```bash
# Compile
arduino-cli compile --fqbn esp32:esp32:esp32 stage4_notifications

# Upload
arduino-cli upload --fqbn esp32:esp32:esp32 -p /dev/ttyUSB0 stage4_notifications

# Monitor
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

## Expected Serial Output

### Notification Sent

```
[NOTIFY] Sending POST to http://example.com/webhook
[NOTIFY] Response: 200
```

### Notification Failed

```
[NOTIFY] Sending POST to http://example.com/webhook
[NOTIFY] Error: connection refused
```

### Alarm Trigger with Notification

```
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
[120s] *** ALARM TRIGGERED *** (#1)
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

[NOTIFY] Sending POST to http://example.com/webhook
[NOTIFY] Response: 200
```

## Memory Usage

| Component | Approximate Size |
|-----------|-----------------|
| Base ESP32 | ~320 KB free |
| WiFi stack | ~40 KB |
| Web server | ~20-40 KB |
| HTTPClient | ~10 KB |
| **Available** | **~230-270 KB** |

## Troubleshooting

### Notifications Not Sending

1. Check "Enable HTTP notifications" is checked
2. Verify URL is correct and accessible
3. Test with Test Notification button
4. Check serial output for error messages

### Connection Refused

- Verify target server is running
- Check firewall settings
- Ensure ESP32 can reach the target (same network)

### Timeout Errors

- Default timeout is 5 seconds
- Target server may be slow or unreachable
- Check network connectivity

### HTTPS Not Working

The ESP32 HTTPClient supports HTTPS but may fail with some certificates. For local webhooks, use HTTP. For external services, ensure the certificate is valid.

## Next Steps

Stage 4 complete? Proceed to:

**Stage 6: Full Diagnostics and System Integration**
- Event logging (circular buffer)
- Watchdog timer
- Comprehensive diagnostics dashboard
- Memory monitoring
- 24-hour stability testing
