# Implementation Tasks

## Add Home Assistant MQTT Integration

**Status:** Pending
**Goal:** Enable automatic Home Assistant discovery via MQTT with push-based motion updates and remote tuning controls

## 1. MQTT Configuration Storage

- [ ] 1.1 Add MQTT fields to Config struct:
  - `mqttEnabled` (bool)
  - `mqttHost` (char[64])
  - `mqttPort` (uint16_t)
  - `mqttUser` (char[32])
  - `mqttPass` (char[64])
  - `mqttUseTls` (bool)
  - `mqttDeviceName` (char[32])
- [ ] 1.2 Update `setDefaultConfig()` with MQTT defaults
- [ ] 1.3 Update `loadConfig()` to read MQTT settings from NVS
- [ ] 1.4 Update `saveConfig()` to persist MQTT settings to NVS
- [ ] 1.5 Update factory reset to clear MQTT settings

## 2. PubSubClient Integration

- [ ] 2.1 Add PubSubClient library dependency
- [ ] 2.2 Create WiFiClient and WiFiClientSecure instances for MQTT
- [ ] 2.3 Initialize PubSubClient with appropriate client based on TLS setting
- [ ] 2.4 Implement `mqttConnect()` function with authentication support
- [ ] 2.5 Implement `mqttDisconnect()` for graceful disconnect
- [ ] 2.6 Implement `mqttLoop()` to be called in main loop
- [ ] 2.7 Implement reconnection logic with exponential backoff

## 3. MQTT Topic Structure

- [ ] 3.1 Create function to get ESP32 chip ID as hex string
- [ ] 3.2 Define base topic prefix: `homeassistant`
- [ ] 3.3 Define device identifier format: `mw_motion_<chip_id>`
- [ ] 3.4 Create helper functions for topic construction:
  - `getDiscoveryTopic(component, objectId)`
  - `getStateTopic(objectId)`
  - `getCommandTopic(objectId)`
  - `getAvailabilityTopic()`

## 4. Home Assistant Discovery Messages

- [ ] 4.1 Create device info JSON block (shared by all entities)
- [ ] 4.2 Implement `publishDiscoveryMotion()` - binary_sensor for motion
- [ ] 4.3 Implement `publishDiscoveryRssi()` - sensor for WiFi signal
- [ ] 4.4 Implement `publishDiscoveryUptime()` - sensor for uptime
- [ ] 4.5 Implement `publishDiscoveryHeap()` - sensor for free heap
- [ ] 4.6 Implement `publishDiscoveryAlarmCount()` - sensor for alarm events
- [ ] 4.7 Implement `publishDiscoveryMotionCount()` - sensor for motion events
- [ ] 4.8 Implement `publishDiscoveryFilterLevel()` - sensor for filter %
- [ ] 4.9 Implement `publishDiscoveryIpAddress()` - sensor for IP
- [ ] 4.10 Implement `publishDiscoveryTripDelay()` - number control
- [ ] 4.11 Implement `publishDiscoveryClearTimeout()` - number control
- [ ] 4.12 Implement `publishDiscoveryFilterThreshold()` - number control
- [ ] 4.13 Implement `publishAllDiscovery()` to send all discovery messages
- [ ] 4.14 Subscribe to `homeassistant/status` for HA restart detection

## 5. MQTT State Publishing

- [ ] 5.1 Implement `publishMotionState()` - called on alarm state change
- [ ] 5.2 Implement `publishDiagnostics()` - called every 60 seconds
- [ ] 5.3 Implement `publishAvailability(bool online)` - for LWT
- [ ] 5.4 Call `publishMotionState()` when entering/exiting ALARM_ACTIVE
- [ ] 5.5 Set up 60-second timer for diagnostic publishing
- [ ] 5.6 Publish initial states after discovery messages

## 6. MQTT Command Handling

- [ ] 6.1 Implement MQTT callback function for incoming messages
- [ ] 6.2 Subscribe to number control command topics
- [ ] 6.3 Handle trip delay set commands
- [ ] 6.4 Handle clear timeout set commands
- [ ] 6.5 Handle filter threshold set commands
- [ ] 6.6 Validate incoming values against min/max bounds
- [ ] 6.7 Update config and save to NVS on valid commands
- [ ] 6.8 Publish updated state after command processing

## 7. Availability (LWT) Implementation

- [ ] 7.1 Configure LWT in MQTT connect: topic, message `offline`, QoS 1, retain
- [ ] 7.2 Publish `online` to availability topic on successful connect
- [ ] 7.3 Publish `offline` before intentional disconnect/reboot
- [ ] 7.4 Include availability configuration in all discovery messages

## 8. TLS Support

- [ ] 8.1 Create WiFiClientSecure instance for TLS connections
- [ ] 8.2 Configure client to skip certificate verification (for self-signed)
- [ ] 8.3 Switch between WiFiClient and WiFiClientSecure based on config
- [ ] 8.4 Auto-suggest port 8883 when TLS is enabled in UI

## 9. Settings Page MQTT Section

- [ ] 9.1 Add "MQTT / Home Assistant" section to SETTINGS_HTML
- [ ] 9.2 Add Enable MQTT checkbox
- [ ] 9.3 Add Broker Host text input with placeholder
- [ ] 9.4 Add Port number input (default 1883)
- [ ] 9.5 Add Username text input (optional)
- [ ] 9.6 Add Password input with placeholder for existing password
- [ ] 9.7 Add Use TLS checkbox with port suggestion
- [ ] 9.8 Add Device Name text input with default
- [ ] 9.9 Add connection status indicator (connected/disconnected)
- [ ] 9.10 Add help text explaining MQTT broker setup options

## 10. Config API Updates

- [ ] 10.1 Update `handleGetConfig()` to include MQTT fields (except password)
- [ ] 10.2 Update `handleGetConfig()` to include `mqttConnected` status
- [ ] 10.3 Update `handlePostConfig()` to accept MQTT fields
- [ ] 10.4 Implement password handling (update only if non-empty, `__CLEAR__` to remove)
- [ ] 10.5 Trigger MQTT reconnect if settings changed

## 11. Settings Page JavaScript

- [ ] 11.1 Add `loadConfig()` handling for MQTT fields
- [ ] 11.2 Add form submission handling for MQTT fields
- [ ] 11.3 Add TLS checkbox change handler to suggest port
- [ ] 11.4 Add connection status polling/display
- [ ] 11.5 Handle password field placeholder behavior

## 12. Diagnostics Page Updates

- [ ] 12.1 Add MQTT section to diagnostics display
- [ ] 12.2 Show broker host and port
- [ ] 12.3 Show connection state
- [ ] 12.4 Show message counts (optional enhancement)

## 13. Event Logging

- [ ] 13.1 Add MQTT_CONNECTED event type
- [ ] 13.2 Add MQTT_DISCONNECTED event type
- [ ] 13.3 Log connection/disconnection events
- [ ] 13.4 Update event type strings for display

## 14. Documentation

- [ ] 14.1 Create stage9_homeassistant/README.md
- [ ] 14.2 Document MQTT broker setup - Home Assistant add-on
- [ ] 14.3 Document MQTT broker setup - Docker Mosquitto
- [ ] 14.4 Document all exposed entities and their device classes
- [ ] 14.5 Document number control ranges and behavior
- [ ] 14.6 Add troubleshooting section for common MQTT issues
- [ ] 14.7 Add Home Assistant automation examples

## 15. Testing

- [ ] 15.1 Test MQTT connection without authentication
- [ ] 15.2 Test MQTT connection with authentication
- [ ] 15.3 Test TLS connection
- [ ] 15.4 Test discovery messages appear in Home Assistant
- [ ] 15.5 Test motion state updates in real-time
- [ ] 15.6 Test diagnostic sensor updates every 60s
- [ ] 15.7 Test number controls from Home Assistant
- [ ] 15.8 Test availability (disconnect device, verify offline in HA)
- [ ] 15.9 Test reconnection after broker restart
- [ ] 15.10 Test re-discovery after Home Assistant restart
- [ ] 15.11 Test factory reset clears MQTT settings
- [ ] 15.12 Test settings persistence across power cycles

## 16. Compile and Deploy

- [ ] 16.1 Compile with no errors
- [ ] 16.2 Verify memory usage acceptable (document heap impact)
- [ ] 16.3 Upload to ESP32
- [ ] 16.4 Full integration test with Home Assistant
