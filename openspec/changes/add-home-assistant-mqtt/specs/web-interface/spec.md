# web-interface Specification Delta

## ADDED Requirements

### Requirement: MQTT Settings UI Section
The Settings page SHALL include an MQTT / Home Assistant configuration section.

#### Scenario: MQTT section display
- **GIVEN** the user is on the Settings page
- **THEN** an "MQTT / Home Assistant" section SHALL be displayed
- **AND** the section SHALL appear after the "WLED Integration" section
- **AND** the section SHALL be visually consistent with other settings sections

#### Scenario: MQTT enable checkbox
- **GIVEN** the MQTT section is displayed
- **THEN** an "Enable MQTT" checkbox SHALL be present
- **AND** the checkbox label SHALL indicate "Connect to MQTT broker for Home Assistant integration"
- **AND** the checkbox SHALL reflect the current `mqttEnabled` setting

#### Scenario: MQTT broker hostname field
- **GIVEN** the MQTT section is displayed
- **THEN** a "Broker Host" text input field SHALL be present
- **AND** the field SHALL have a placeholder showing example: "192.168.1.100 or mqtt.local"
- **AND** the field SHALL support hostnames up to 64 characters
- **AND** the field SHALL display the current `mqttHost` setting

#### Scenario: MQTT broker port field
- **GIVEN** the MQTT section is displayed
- **THEN** a "Port" number input field SHALL be present
- **AND** the field SHALL default to 1883
- **AND** the field SHALL accept values between 1 and 65535

#### Scenario: MQTT username field
- **GIVEN** the MQTT section is displayed
- **THEN** a "Username" text input field SHALL be present
- **AND** the field SHALL be optional (can be empty)
- **AND** the field SHALL support usernames up to 32 characters

#### Scenario: MQTT password field
- **GIVEN** the MQTT section is displayed
- **THEN** a "Password" password input field SHALL be present
- **AND** the field SHALL be optional (can be empty)
- **AND** the field SHALL show placeholder "••••••••" if a password is configured
- **AND** the field SHALL support passwords up to 64 characters

#### Scenario: MQTT TLS checkbox
- **GIVEN** the MQTT section is displayed
- **THEN** a "Use TLS/SSL" checkbox SHALL be present
- **AND** when TLS is enabled, the port field SHALL suggest 8883
- **AND** a note SHALL indicate TLS encrypts the connection

#### Scenario: MQTT device name field
- **GIVEN** the MQTT section is displayed
- **THEN** a "Device Name" text input field SHALL be present
- **AND** the field SHALL have a placeholder showing "Microwave Motion Sensor"
- **AND** the field SHALL support names up to 32 characters
- **AND** a note SHALL indicate this name appears in Home Assistant

#### Scenario: MQTT connection status display
- **GIVEN** the MQTT section is displayed
- **AND** MQTT is enabled
- **THEN** the current connection status SHALL be displayed
- **AND** "Connected" SHALL be shown in green if connected
- **AND** "Disconnected" SHALL be shown in red if not connected

### Requirement: MQTT Broker Setup Documentation Link
The Settings page SHALL provide guidance for MQTT broker setup.

#### Scenario: Setup help link
- **GIVEN** the MQTT section is displayed
- **THEN** a help link or expandable section SHALL be present
- **AND** the help content SHALL explain how to set up Mosquitto broker
- **AND** the help content SHALL reference both Home Assistant add-on and Docker options

### Requirement: Diagnostics Page MQTT Status
The Diagnostics page SHALL display MQTT connection information.

#### Scenario: MQTT status in diagnostics
- **GIVEN** the user is on the Diagnostics page
- **THEN** an MQTT section SHALL display connection status
- **AND** the section SHALL show broker hostname and port
- **AND** the section SHALL show connected/disconnected state
- **AND** the section SHALL show message counts (published, received) if connected
