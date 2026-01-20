# Change: Add Home Assistant MQTT Integration

## Why

Users want the ESP32 motion sensor to integrate seamlessly with Home Assistant for home automation. Currently, HA integration requires manual REST sensor configuration with polling delays. MQTT Discovery enables automatic device discovery with instant push-based state updates.

## What Changes

- Add MQTT client functionality with PubSubClient library
- Implement Home Assistant MQTT Discovery protocol
- Expose motion sensor as binary_sensor with device_class: motion
- Expose diagnostic sensors (WiFi signal, uptime, heap, alarm count, etc.)
- Expose number controls for tuning parameters (trip delay, clear timeout, filter threshold)
- Add MQTT configuration section to Settings page (broker, port, credentials, TLS)
- Support both plaintext (1883) and TLS (8883) connections
- Implement availability tracking via MQTT Last Will and Testament (LWT)
- Document MQTT broker setup (Docker and Home Assistant add-on)

## Impact

- Affected specs: mqtt-integration (new), configuration, web-interface
- Affected code: stage9_homeassistant/ (new stage)
- New dependency: PubSubClient library
- Memory: Additional ~10-15KB for MQTT client

## User Decisions

| Decision | Choice |
|----------|--------|
| Motion entity | Alarm state (ON when ALARM_ACTIVE) |
| Device name | Configurable, default "Microwave Motion Sensor" |
| Availability | LWT enabled |
| State retention | No retention for motion |
| HA Controls | Trip delay, clear timeout, filter threshold |
| Update frequency | 60 seconds for diagnostics |
| Topic structure | Auto-generated from ESP32 chip ID |
| TLS/SSL | Support both, configurable |
