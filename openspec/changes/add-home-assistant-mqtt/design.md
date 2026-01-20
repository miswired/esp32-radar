# Design: Home Assistant MQTT Integration

## Context

The ESP32 motion sensor needs to integrate with Home Assistant for home automation workflows. Home Assistant supports multiple integration methods, but MQTT Discovery provides the best combination of automatic discovery, push-based updates, and local-only operation.

### Stakeholders
- End users running Home Assistant
- Users with standalone MQTT brokers (Mosquitto)
- Users preferring local-only operation (no cloud)

### Constraints
- Must work alongside existing HTTP server and WLED integration
- Memory budget: ~278KB available RAM
- WiFi stability required for concurrent HTTP + MQTT connections

## Goals / Non-Goals

### Goals
- Automatic device discovery in Home Assistant
- Instant motion state updates (push-based)
- Expose diagnostic sensors for monitoring
- Allow remote tuning of detection parameters
- Support authenticated MQTT connections
- Optional TLS encryption

### Non-Goals
- Cloud MQTT brokers (AWS IoT, etc.)
- ESPHome native API compatibility
- MQTT v5 features (v3.1.1 sufficient)
- Multiple broker failover

## Decisions

### Decision: Use PubSubClient Library
**Choice:** PubSubClient by Nick O'Leary
**Rationale:**
- Well-maintained, widely used (>6k GitHub stars)
- Small footprint (~15KB)
- Supports MQTT v3.1.1
- Works with WiFiClientSecure for TLS
- Already compatible with ESP32

**Alternatives considered:**
- AsyncMQTTClient: Better performance but more complex, less stable
- ESP-MQTT (IDF): Lower level, more code to write
- Arduino MQTT: Less mature ecosystem

### Decision: Topic Structure
**Choice:** `homeassistant/<component>/mw_motion_<chip_id>/<object>/config|state`

Example for chip ID `ABC123`:
```
homeassistant/binary_sensor/mw_motion_abc123/motion/config
homeassistant/binary_sensor/mw_motion_abc123/motion/state
homeassistant/sensor/mw_motion_abc123/rssi/config
homeassistant/sensor/mw_motion_abc123/rssi/state
homeassistant/number/mw_motion_abc123/trip_delay/config
homeassistant/number/mw_motion_abc123/trip_delay/state
homeassistant/number/mw_motion_abc123/trip_delay/set
```

**Rationale:**
- Chip ID ensures uniqueness for multiple devices
- `mw_motion_` prefix identifies device type
- Follows Home Assistant discovery conventions

### Decision: Availability via LWT
**Choice:** Use MQTT Last Will and Testament for availability tracking

**Topics:**
- Availability topic: `homeassistant/binary_sensor/mw_motion_<chip_id>/availability`
- Birth message: `online`
- Will message: `offline`

**Rationale:** Allows HA to show device as unavailable if ESP32 disconnects unexpectedly.

### Decision: State Retention Policy
**Choice:**
- Motion state: NOT retained (prevents stale alerts)
- Diagnostic sensors: NOT retained (values update frequently)
- Config topics: RETAINED (allows discovery after HA restart)

### Decision: TLS Implementation
**Choice:** Optional TLS using WiFiClientSecure

**Configuration:**
- `mqttUseTls` boolean in config
- Port defaults: 1883 (plaintext), 8883 (TLS)
- Skip certificate validation option for self-signed certs

**Rationale:** Local networks often use self-signed certificates. Full certificate validation can be added later.

## Entity Design

### Binary Sensors
| Entity | device_class | State Source |
|--------|--------------|--------------|
| Motion | motion | ALARM_ACTIVE = ON, else OFF |

### Sensors (Diagnostics)
| Entity | device_class | Unit | Update |
|--------|--------------|------|--------|
| WiFi Signal | signal_strength | dBm | 60s |
| Uptime | duration | s | 60s |
| Free Heap | data_size | B | 60s |
| Alarm Count | - | - | on change |
| Motion Count | - | - | on change |
| Filter Level | - | % | 60s |
| IP Address | - | - | on change |

### Number Controls
| Entity | Min | Max | Step | Unit |
|--------|-----|-----|------|------|
| Trip Delay | 0 | 30 | 1 | s |
| Clear Timeout | 1 | 300 | 1 | s |
| Filter Threshold | 0 | 100 | 5 | % |

## MQTT Broker Setup Documentation

### Option A: Home Assistant Mosquitto Add-on
For users running Home Assistant OS or Supervised:
1. Install "Mosquitto broker" add-on from Add-on Store
2. Configure logins in add-on configuration (or use HA users)
3. Start add-on
4. Add MQTT integration in Settings > Devices & Services

### Option B: Docker Mosquitto (Standalone)
For users with separate MQTT broker:

```yaml
# docker-compose.yml
version: '3'
services:
  mosquitto:
    image: eclipse-mosquitto:2
    ports:
      - "1883:1883"
      - "9001:9001"
    volumes:
      - ./mosquitto/config:/mosquitto/config
      - ./mosquitto/data:/mosquitto/data
      - ./mosquitto/log:/mosquitto/log
    restart: unless-stopped
```

```conf
# mosquitto/config/mosquitto.conf
listener 1883
persistence true
persistence_location /mosquitto/data/
log_dest file /mosquitto/log/mosquitto.log
allow_anonymous false
password_file /mosquitto/config/pwfile
```

Create password file:
```bash
docker exec -it mosquitto mosquitto_passwd -c /mosquitto/config/pwfile mqtt_user
```

## Risks / Trade-offs

### Risk: WiFi Stability with Concurrent Connections
**Impact:** MQTT + HTTP + occasional WLED requests may strain WiFi
**Mitigation:**
- Use QoS 0 for most messages (fire and forget)
- Implement reconnection with exponential backoff
- Monitor heap and connection state

### Risk: Memory Pressure
**Impact:** PubSubClient + TLS adds memory overhead
**Mitigation:**
- TLS is optional, disabled by default
- Buffer sizes kept minimal (256 bytes for publish)
- Monitor free heap via diagnostics

### Risk: Discovery Message Size
**Impact:** Large JSON payloads for discovery config
**Mitigation:**
- Send discovery messages one entity at a time
- Use abbreviated keys where HA supports them (`~` for base topic)

## Migration Plan

Not applicable - this is a new feature with no existing MQTT functionality.

## Open Questions

1. **Should we support MQTT v5?** Currently targeting v3.1.1 which is sufficient for HA. v5 adds features like message expiry but increases complexity.

2. **Home Assistant birth message subscription?** Should ESP32 subscribe to `homeassistant/status` to re-announce discovery on HA restart? Recommended: Yes.
