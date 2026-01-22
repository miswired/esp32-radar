# ESP32 Microwave Motion Sensor Firmware

The latest firmware with all features including Home Assistant integration via MQTT Discovery protocol, enabling automatic device discovery and real-time motion updates.

## Features

- **MQTT Discovery**: Automatic device registration in Home Assistant
- **Motion Binary Sensor**: Real-time motion detection state
- **Diagnostic Sensors**: WiFi signal, uptime, heap, alarm/motion counts, filter level, IP address
- **Number Controls**: Adjust trip delay, clear timeout, and filter threshold from Home Assistant
- **Availability Tracking**: LWT (Last Will and Testament) for online/offline status
- **Authentication Support**: Username/password authentication
- **TLS Support**: Encrypted connections (port 8883)

## Home Assistant Entities

When connected, the following entities appear under a single device in Home Assistant:

### Binary Sensor
| Entity | Device Class | Description |
|--------|--------------|-------------|
| Motion | `motion` | ON when alarm active, OFF otherwise |

### Sensors (Diagnostic)
| Entity | Device Class | Unit | Description |
|--------|--------------|------|-------------|
| WiFi Signal | `signal_strength` | dBm | WiFi RSSI |
| Uptime | `duration` | s | Time since boot |
| Free Heap | `data_size` | B | Available memory |
| Alarm Count | - | - | Total alarm events (counter) |
| Motion Count | - | - | Total motion events (counter) |
| Filter Level | - | % | Current adaptive filter level |
| IP Address | - | - | Device IP address |

### Number Controls
| Entity | Min | Max | Step | Unit | Description |
|--------|-----|-----|------|------|-------------|
| Trip Delay | 0 | 30 | 1 | s | Delay before triggering alarm |
| Clear Timeout | 1 | 300 | 1 | s | Time before clearing alarm |
| Filter Threshold | 0 | 100 | 5 | % | Adaptive filter trigger level |

## MQTT Broker Setup

### Option 1: Home Assistant Mosquitto Add-on (Recommended)

1. In Home Assistant, go to **Settings → Add-ons → Add-on Store**
2. Search for "Mosquitto broker" and install it
3. Go to the add-on's **Configuration** tab
4. Add a user under `logins`:
   ```yaml
   logins:
     - username: mqtt_user
       password: your_secure_password
   ```
5. Start the add-on
6. In the sensor's Settings page, configure:
   - **Broker Host**: Your Home Assistant IP (e.g., `192.168.1.100`)
   - **Port**: `1883` (or `8883` for TLS)
   - **Username**: `mqtt_user`
   - **Password**: Your password

### Option 2: Docker Mosquitto (Standalone Server)

1. Create a directory for Mosquitto configuration:
   ```bash
   mkdir -p ~/mosquitto/config ~/mosquitto/data ~/mosquitto/log
   ```

2. Create the configuration file `~/mosquitto/config/mosquitto.conf`:
   ```
   listener 1883
   allow_anonymous false
   password_file /mosquitto/config/password.txt
   persistence true
   persistence_location /mosquitto/data/
   log_dest file /mosquitto/log/mosquitto.log
   ```

3. Create a password file:
   ```bash
   docker run -it --rm -v ~/mosquitto/config:/mosquitto/config eclipse-mosquitto \
     mosquitto_passwd -c /mosquitto/config/password.txt mqtt_user
   ```

4. Run Mosquitto:
   ```bash
   docker run -d \
     --name mosquitto \
     --restart unless-stopped \
     -p 1883:1883 \
     -v ~/mosquitto/config:/mosquitto/config \
     -v ~/mosquitto/data:/mosquitto/data \
     -v ~/mosquitto/log:/mosquitto/log \
     eclipse-mosquitto
   ```

5. Add the MQTT integration in Home Assistant:
   - Go to **Settings → Devices & Services → Add Integration**
   - Search for "MQTT"
   - Enter your Docker host IP, port 1883, and credentials

## Configuration

### Settings Page

Navigate to the device's web interface and open the **Settings** page. The MQTT section includes:

- **Enable MQTT**: Toggle MQTT integration on/off
- **Broker Host**: MQTT broker hostname or IP address
- **Port**: Broker port (1883 for plain, 8883 for TLS)
- **Username**: MQTT username (optional)
- **Password**: MQTT password (optional)
- **Use TLS**: Enable encrypted connection
- **Device Name**: Name shown in Home Assistant (default: "Microwave Motion Sensor")

### API Endpoints

**GET /config** returns MQTT settings (password excluded):
```json
{
  "mqttEnabled": true,
  "mqttHost": "192.168.1.100",
  "mqttPort": 1883,
  "mqttUser": "mqtt_user",
  "mqttUseTls": false,
  "mqttDeviceName": "Microwave Motion Sensor",
  "mqttConnected": true
}
```

**POST /config** accepts MQTT settings:
```json
{
  "mqttEnabled": true,
  "mqttHost": "192.168.1.100",
  "mqttPort": 1883,
  "mqttUser": "mqtt_user",
  "mqttPass": "new_password",
  "mqttUseTls": false,
  "mqttDeviceName": "Living Room Motion"
}
```

Password handling:
- Empty `mqttPass` field: Password unchanged
- `"mqttPass": "__CLEAR__"`: Clears the password

## MQTT Topic Structure

All topics use the device's unique chip ID for namespacing:

```
homeassistant/binary_sensor/mw_motion_<chip_id>/motion/config    # Discovery
homeassistant/sensor/mw_motion_<chip_id>/rssi/config             # Discovery
homeassistant/number/mw_motion_<chip_id>/trip_delay/config       # Discovery

mw_motion_<chip_id>/motion/state         # Motion state (ON/OFF)
mw_motion_<chip_id>/diagnostics          # JSON with all diagnostic values
mw_motion_<chip_id>/trip_delay/state     # Current trip delay value
mw_motion_<chip_id>/trip_delay/set       # Command topic for setting value
mw_motion_<chip_id>/availability         # Online/offline status
```

## Home Assistant Automation Examples

### Turn on lights when motion detected
```yaml
automation:
  - alias: "Motion Detected - Turn On Lights"
    trigger:
      - platform: state
        entity_id: binary_sensor.mw_motion_abc123_motion
        to: "on"
    action:
      - service: light.turn_on
        target:
          entity_id: light.living_room
```

### Send notification on motion
```yaml
automation:
  - alias: "Motion Alert"
    trigger:
      - platform: state
        entity_id: binary_sensor.mw_motion_abc123_motion
        to: "on"
    condition:
      - condition: time
        after: "22:00:00"
        before: "06:00:00"
    action:
      - service: notify.mobile_app
        data:
          message: "Motion detected in living room!"
```

### Adjust sensitivity based on time of day
```yaml
automation:
  - alias: "Increase Sensitivity at Night"
    trigger:
      - platform: time
        at: "22:00:00"
    action:
      - service: number.set_value
        target:
          entity_id: number.mw_motion_abc123_filter_threshold
        data:
          value: 30

  - alias: "Decrease Sensitivity During Day"
    trigger:
      - platform: time
        at: "08:00:00"
    action:
      - service: number.set_value
        target:
          entity_id: number.mw_motion_abc123_filter_threshold
        data:
          value: 60
```

## Troubleshooting

### Device not appearing in Home Assistant

1. Verify MQTT broker is running:
   ```bash
   mosquitto_sub -h localhost -t "#" -v
   ```

2. Check the sensor's Diagnostics page for MQTT connection status

3. Verify credentials match between sensor and broker

4. Check Home Assistant logs for MQTT discovery messages

### Connection keeps dropping

1. Ensure stable WiFi connection (check RSSI on diagnostics page)
2. Verify broker allows the configured keepalive interval (60s)
3. Check for firewall rules blocking MQTT ports

### Entities show "Unavailable"

1. The device may be offline - check power and WiFi
2. MQTT broker may have restarted - device will reconnect automatically
3. Check the availability topic: `mw_motion_<chip_id>/availability`

### Number controls not working

1. Ensure the sensor is subscribed to command topics
2. Check that values are within the allowed range
3. Verify Home Assistant is publishing to the correct `/set` topic

## Memory Usage

- Program storage: ~1.2 MB (92% of 1.3 MB)
- Dynamic memory: ~50 KB (15% of 328 KB)

The MQTT integration adds approximately 100 KB to program storage due to the PubSubClient library and discovery message formatting.

## Compilation and Upload

```bash
# Compile
arduino-cli compile --fqbn esp32:esp32:esp32 firmware

# Upload
arduino-cli upload --fqbn esp32:esp32:esp32 -p /dev/ttyUSB0 firmware

# Monitor
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

## Event Types

New events added in Stage 9:

| Event | Data | Description |
|-------|------|-------------|
| `MQTT_CONNECTED` | - | Connected to MQTT broker |
| `MQTT_DISCONNECTED` | - | Disconnected from MQTT broker |

## Dependencies

- PubSubClient library (v2.8.0+)
- ArduinoJson library (already included from previous stages)

Install PubSubClient:
```bash
arduino-cli lib install PubSubClient
```
