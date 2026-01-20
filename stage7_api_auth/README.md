# Stage 7: API Key Authentication

This stage builds on Stage 6 and adds optional API key authentication for securing JSON endpoints.

## New Features in Stage 7

- **API Key Authentication**: Optional authentication for all JSON API endpoints
- **UUID v4 Keys**: Cryptographically secure keys generated using ESP32 hardware RNG
- **Dual Auth Methods**: Support for both HTTP header (`X-API-Key`) and query parameter (`?apiKey=`)
- **Rate Limiting**: Lockout after 5 failed attempts (5-minute cooldown)
- **Security Logging**: Authentication failures logged to event system
- **Web UI Management**: Generate, view, and manage keys from Settings page
- **Serial Commands**: New 'a' command for auth management

## Prerequisites

### Libraries Required

Same as Stage 6:
```bash
arduino-cli lib install "ArduinoJson"
```

### Hardware

Same as previous stages:
- ESP32-WROOM-32
- RCWL-0516 sensor (VIN→VIN, GND→GND, OUT→GPIO13)

## API Authentication

### Overview

Authentication is **disabled by default** for backwards compatibility. The API key feature is intended for securing external integrations (scripts, Home Assistant, etc.) and does not restrict web UI access. The local web interface always functions fully, allowing administrators to manage settings without needing an API key.

### API Key Authentication Model

The API key feature is designed for **external integrations** (curl, scripts, Home Assistant, etc.). The local web UI does not require API key authentication to function, allowing administrators to always access and modify settings through the browser.

**All endpoints are accessible without API key:**

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/status` | Sensor status (JSON) |
| GET | `/config` | Configuration (JSON) |
| GET | `/logs` | Event log (JSON) |
| GET | `/diagnostics` | System diagnostics (JSON) |
| POST | `/config` | Save configuration |
| POST | `/test-notification` | Test notification |
| POST | `/reset` | Factory reset |
| GET | `/` | Dashboard page |
| GET | `/settings` | Settings page |
| GET | `/diag` | Diagnostics page |
| GET | `/api` | API documentation |
| POST | `/generate-key` | Generate new key (preview) |

**Note:** A separate password protection mechanism for web access is planned for a future release.

### Authentication Methods

**1. HTTP Header (Recommended)**
```bash
curl -H "X-API-Key: your-api-key-here" http://192.168.1.100/status
```

**2. Query Parameter**
```bash
curl "http://192.168.1.100/status?apiKey=your-api-key-here"
```

### Error Responses

**401 Unauthorized** - Missing or invalid API key
```json
{"error": "Authentication required", "code": 401}
```

**429 Too Many Requests** - Rate limited after 5 failed attempts
```json
{"error": "Too many failed attempts", "code": 429, "retryAfter": 300}
```

## Configuration

### Enabling Authentication

**Via Web Interface:**
1. Navigate to Settings page (`/settings`)
2. Scroll to "API Authentication" section
3. Check "Require API key for JSON endpoints"
4. Click "Generate New Key" to create a key
5. Copy the key (shown once) and store it securely
6. Click "Save Settings"

**Via Serial Console:**
1. Connect to serial monitor (115200 baud)
2. Press 'a' to view auth status
3. Press 'A' (within 3 seconds) to toggle auth on/off
4. Press 'a' again (within 3 seconds) to generate a new key

### Key Format

API keys use UUID v4 format:
```
a1b2c3d4-e5f6-4789-abcd-ef1234567890
```

Keys are generated using ESP32's hardware random number generator for cryptographic security.

## Serial Commands

All Stage 6 commands, plus:

| Command | Description |
|---------|-------------|
| `a` | Show API auth status and key |
| `a` + `a` | Generate new API key |
| `a` + `A` | Toggle auth on/off |

The 'a' command waits 3 seconds for a follow-up command.

## REST API

### GET /diagnostics (Updated)

Now includes authentication status:

```json
{
  "uptime": 3600000,
  "freeHeap": 250000,
  "authEnabled": true,
  "authFailedAttempts": 2,
  "authLockoutRemaining": 0
}
```

### POST /generate-key

Generate a new API key without saving (for preview).

**Response:**
```json
{"key": "a1b2c3d4-e5f6-4789-abcd-ef1234567890"}
```

## Event Types

New events added in Stage 7:

| Event | Data | Description |
|-------|------|-------------|
| `AUTH_FAILED` | attempt # | Failed authentication attempt |
| `AUTH_LOCKOUT` | duration (s) | Rate limit lockout activated |

## Compilation and Upload

```bash
# Compile
arduino-cli compile --fqbn esp32:esp32:esp32 stage7_api_auth

# Upload
arduino-cli upload --fqbn esp32:esp32:esp32 -p /dev/ttyUSB0 stage7_api_auth

# Monitor
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

## Expected Serial Output

### Boot Sequence

```
==========================================
ESP32 Microwave Motion Sensor - Stage 7
==========================================
API Key Authentication

Reset reason: POWERON_RESET

GPIO initialized
Loading configuration from NVS... OK
API Authentication: Disabled
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
Diagnostics:  http://192.168.1.100/diag
------------------------------------------

=== STAGE 7 SELF TEST ===
  GPIO (pin 13): PASS
  LED (pin 2): PASS
  Heap (278 KB): PASS
  NVS: PASS
  WiFi: PASS
  Web Server: PASS

Overall: ALL TESTS PASSED
=========================
```

### Authentication Events

```
[AUTH] Failed attempt #1
[LOG] AUTH_FAILED (1)
[AUTH] Failed attempt #5
[AUTH] Lockout activated for 300 seconds
[LOG] AUTH_LOCKOUT (300)
```

## Memory Usage

| Component | Approximate Size |
|-----------|------------------|
| Program Storage | 1,145 KB (87%) |
| Global Variables | 48 KB (14%) |
| **Available RAM** | **~279 KB** |

## Security Considerations

1. **Key Storage**: API key is stored in NVS (flash memory). Physical access to the device could allow key extraction.

2. **Transport Security**: Communication is over HTTP (not HTTPS). For sensitive deployments, use a VPN or isolate the device network.

3. **Rate Limiting**: After 5 failed attempts, the device locks out for 5 minutes. This prevents brute-force attacks.

4. **Key Regeneration**: Regenerating a key immediately invalidates the old key. Ensure you update all clients.

5. **Factory Reset**: Factory reset clears the API key along with all other settings.

## Troubleshooting

### "Authentication required" error

1. Verify auth is enabled: Check `/diagnostics` or serial command 'd'
2. Check key format: Must be 36-character UUID
3. Try both methods: Header (`X-API-Key`) and query parameter (`?apiKey=`)
4. Check for lockout: Look for `authLockoutRemaining` in `/diagnostics`

### Locked out of device

1. Wait 5 minutes for lockout to expire, OR
2. Use serial console:
   - Press 'a', then 'A' to toggle auth off
   - Press 'f' for factory reset (clears all settings)

### Key not working after regeneration

1. Clear browser localStorage (stored key is cached)
2. Update any external integrations with the new key
3. Verify the new key was saved (check serial output)

## Integration Examples

### Home Assistant

```yaml
rest:
  - resource: "http://192.168.1.100/status"
    headers:
      X-API-Key: "your-api-key-here"
    sensor:
      - name: "ESP32 Radar State"
        value_template: "{{ value_json.state }}"
```

### curl with Authentication

```bash
# GET request
curl -H "X-API-Key: your-key" http://192.168.1.100/status

# POST request
curl -X POST http://192.168.1.100/config \
  -H "Content-Type: application/json" \
  -H "X-API-Key: your-key" \
  -d '{"tripDelay": 5}'
```

### Python

```python
import requests

API_KEY = "your-api-key-here"
BASE_URL = "http://192.168.1.100"

headers = {"X-API-Key": API_KEY}

# Get status
response = requests.get(f"{BASE_URL}/status", headers=headers)
print(response.json())

# Update config
response = requests.post(
    f"{BASE_URL}/config",
    headers={**headers, "Content-Type": "application/json"},
    json={"tripDelay": 5}
)
print(response.json())
```

## Next Steps

Stage 7 completes the core functionality. Future enhancements could include:

- HTTPS/TLS support (requires significant flash space)
- Multiple API keys with different permissions
- OAuth2 integration
- MQTT with authentication
- OTA (Over-The-Air) firmware updates
