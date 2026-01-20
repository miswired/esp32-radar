# mqtt-integration Specification Delta

## ADDED Requirements

### Requirement: MQTT Client Connection
The system SHALL maintain a persistent MQTT connection to a configured broker.

#### Scenario: MQTT connection established
- **GIVEN** MQTT is enabled in configuration
- **AND** broker hostname, port, and optional credentials are configured
- **WHEN** the device boots or WiFi connects
- **THEN** the system SHALL connect to the MQTT broker
- **AND** the system SHALL publish `online` to the availability topic
- **AND** the system SHALL set LWT to publish `offline` on unexpected disconnect

#### Scenario: MQTT connection with authentication
- **GIVEN** MQTT username and password are configured
- **WHEN** connecting to the broker
- **THEN** the system SHALL authenticate with the provided credentials
- **AND** connection SHALL fail if credentials are invalid

#### Scenario: MQTT connection with TLS
- **GIVEN** MQTT TLS is enabled in configuration
- **WHEN** connecting to the broker
- **THEN** the system SHALL use WiFiClientSecure for encrypted connection
- **AND** the default port SHALL be 8883

#### Scenario: MQTT reconnection
- **GIVEN** an active MQTT connection
- **WHEN** the connection is lost
- **THEN** the system SHALL attempt to reconnect with exponential backoff
- **AND** the system SHALL re-publish discovery messages on successful reconnect

### Requirement: Home Assistant MQTT Discovery
The system SHALL announce itself to Home Assistant using MQTT Discovery protocol.

#### Scenario: Discovery message format
- **WHEN** the system connects to the MQTT broker
- **THEN** the system SHALL publish discovery config messages for each entity
- **AND** discovery topics SHALL follow the format `homeassistant/<component>/mw_motion_<chip_id>/<object>/config`
- **AND** discovery payloads SHALL be valid JSON with required HA fields

#### Scenario: Device grouping
- **GIVEN** multiple entities are published
- **THEN** all entities SHALL include the same `device` block
- **AND** the device block SHALL contain `identifiers` with the chip ID
- **AND** the device block SHALL contain the configurable device name
- **AND** all entities SHALL appear under a single device in Home Assistant

#### Scenario: Discovery on Home Assistant restart
- **GIVEN** the system is connected to MQTT
- **WHEN** Home Assistant publishes `online` to `homeassistant/status`
- **THEN** the system SHALL re-publish all discovery messages

### Requirement: Motion Binary Sensor Entity
The system SHALL expose motion detection as a Home Assistant binary sensor.

#### Scenario: Motion sensor discovery
- **WHEN** discovery messages are published
- **THEN** a binary_sensor entity SHALL be announced
- **AND** the device_class SHALL be `motion`
- **AND** the unique_id SHALL be `mw_motion_<chip_id>_motion`

#### Scenario: Motion state updates
- **GIVEN** the motion binary sensor is discovered
- **WHEN** the alarm state changes to ALARM_ACTIVE
- **THEN** the system SHALL publish `ON` to the motion state topic
- **WHEN** the alarm state changes from ALARM_ACTIVE to any other state
- **THEN** the system SHALL publish `OFF` to the motion state topic

#### Scenario: Motion state not retained
- **WHEN** publishing motion state
- **THEN** the message SHALL NOT be retained
- **AND** this prevents stale motion alerts after broker restart

### Requirement: Diagnostic Sensor Entities
The system SHALL expose diagnostic information as Home Assistant sensors.

#### Scenario: WiFi signal sensor
- **WHEN** discovery messages are published
- **THEN** a sensor entity for WiFi signal strength SHALL be announced
- **AND** the device_class SHALL be `signal_strength`
- **AND** the unit_of_measurement SHALL be `dBm`
- **AND** the entity_category SHALL be `diagnostic`

#### Scenario: Uptime sensor
- **WHEN** discovery messages are published
- **THEN** a sensor entity for uptime SHALL be announced
- **AND** the device_class SHALL be `duration`
- **AND** the unit_of_measurement SHALL be `s`
- **AND** the entity_category SHALL be `diagnostic`

#### Scenario: Free heap sensor
- **WHEN** discovery messages are published
- **THEN** a sensor entity for free heap memory SHALL be announced
- **AND** the device_class SHALL be `data_size`
- **AND** the unit_of_measurement SHALL be `B`
- **AND** the entity_category SHALL be `diagnostic`

#### Scenario: Alarm count sensor
- **WHEN** discovery messages are published
- **THEN** a sensor entity for alarm event count SHALL be announced
- **AND** the state_class SHALL be `total_increasing`

#### Scenario: Motion count sensor
- **WHEN** discovery messages are published
- **THEN** a sensor entity for motion event count SHALL be announced
- **AND** the state_class SHALL be `total_increasing`

#### Scenario: Filter level sensor
- **WHEN** discovery messages are published
- **THEN** a sensor entity for current filter level SHALL be announced
- **AND** the unit_of_measurement SHALL be `%`

#### Scenario: IP address sensor
- **WHEN** discovery messages are published
- **THEN** a sensor entity for IP address SHALL be announced
- **AND** the entity_category SHALL be `diagnostic`

#### Scenario: Diagnostic sensor update interval
- **GIVEN** diagnostic sensors are discovered
- **THEN** the system SHALL publish updated values every 60 seconds
- **AND** alarm and motion counts SHALL also update immediately on change

### Requirement: Number Control Entities
The system SHALL expose tuning parameters as Home Assistant number entities.

#### Scenario: Trip delay control
- **WHEN** discovery messages are published
- **THEN** a number entity for trip delay SHALL be announced
- **AND** the min value SHALL be 0
- **AND** the max value SHALL be 30
- **AND** the step SHALL be 1
- **AND** the unit_of_measurement SHALL be `s`

#### Scenario: Clear timeout control
- **WHEN** discovery messages are published
- **THEN** a number entity for clear timeout SHALL be announced
- **AND** the min value SHALL be 1
- **AND** the max value SHALL be 300
- **AND** the step SHALL be 1
- **AND** the unit_of_measurement SHALL be `s`

#### Scenario: Filter threshold control
- **WHEN** discovery messages are published
- **THEN** a number entity for filter threshold SHALL be announced
- **AND** the min value SHALL be 0
- **AND** the max value SHALL be 100
- **AND** the step SHALL be 5
- **AND** the unit_of_measurement SHALL be `%`

#### Scenario: Number control commands
- **GIVEN** a number control entity is discovered
- **WHEN** Home Assistant publishes a new value to the entity's command topic
- **THEN** the system SHALL update the corresponding configuration value
- **AND** the system SHALL save the configuration to NVS
- **AND** the system SHALL publish the new value to the state topic

### Requirement: Availability Tracking
The system SHALL report availability status via MQTT LWT.

#### Scenario: Online status on connect
- **WHEN** the MQTT connection is established
- **THEN** the system SHALL publish `online` to the availability topic
- **AND** the message SHALL be retained

#### Scenario: Offline status on disconnect
- **GIVEN** the MQTT LWT is configured
- **WHEN** the device disconnects unexpectedly
- **THEN** the broker SHALL publish `offline` to the availability topic

#### Scenario: Graceful disconnect
- **WHEN** the device is intentionally disconnecting (e.g., reboot)
- **THEN** the system SHALL publish `offline` to the availability topic before disconnecting

#### Scenario: Entity availability reference
- **WHEN** publishing discovery messages
- **THEN** each entity SHALL reference the availability topic
- **AND** each entity SHALL specify `payload_available` as `online`
- **AND** each entity SHALL specify `payload_not_available` as `offline`
