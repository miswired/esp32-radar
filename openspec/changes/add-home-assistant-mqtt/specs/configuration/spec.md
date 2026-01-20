# configuration Specification Delta

## ADDED Requirements

### Requirement: MQTT Configuration Storage
The system SHALL persist MQTT integration settings in NVS.

#### Scenario: MQTT settings stored in NVS
- **WHEN** the user saves MQTT settings
- **THEN** the following SHALL be stored in NVS:
  - `mqttEnabled` (boolean) - Whether MQTT integration is enabled
  - `mqttHost` (string, max 64 chars) - MQTT broker hostname or IP
  - `mqttPort` (uint16) - MQTT broker port
  - `mqttUser` (string, max 32 chars) - MQTT username (optional)
  - `mqttPass` (string, max 64 chars) - MQTT password (optional)
  - `mqttUseTls` (boolean) - Whether to use TLS encryption
  - `mqttDeviceName` (string, max 32 chars) - Device name in Home Assistant
- **AND** settings SHALL persist across power cycles

#### Scenario: MQTT defaults on initialization
- **WHEN** the device boots with no saved MQTT settings
- **THEN** `mqttEnabled` SHALL be set to false
- **AND** `mqttHost` SHALL be empty
- **AND** `mqttPort` SHALL be set to 1883
- **AND** `mqttUser` SHALL be empty
- **AND** `mqttPass` SHALL be empty
- **AND** `mqttUseTls` SHALL be set to false
- **AND** `mqttDeviceName` SHALL be set to "Microwave Motion Sensor"

#### Scenario: MQTT cleared on factory reset
- **WHEN** a factory reset is performed
- **THEN** all MQTT settings SHALL be reset to defaults

### Requirement: MQTT Fields in Config API
The configuration API endpoints SHALL include MQTT settings.

#### Scenario: GET /config includes MQTT settings
- **WHEN** a GET request is made to /config
- **THEN** the response SHALL include `mqttEnabled` (boolean)
- **AND** the response SHALL include `mqttHost` (string)
- **AND** the response SHALL include `mqttPort` (number)
- **AND** the response SHALL include `mqttUser` (string)
- **AND** the response SHALL NOT include `mqttPass` (password hidden)
- **AND** the response SHALL include `mqttUseTls` (boolean)
- **AND** the response SHALL include `mqttDeviceName` (string)
- **AND** the response SHALL include `mqttConnected` (boolean, current state)

#### Scenario: POST /config accepts MQTT settings
- **WHEN** a POST request to /config includes MQTT fields
- **THEN** the system SHALL update the MQTT configuration
- **AND** the system SHALL save the settings to NVS
- **AND** the system SHALL reconnect to MQTT if settings changed and MQTT is enabled

#### Scenario: MQTT password handling
- **WHEN** a POST request to /config includes `mqttPass`
- **THEN** the password SHALL be updated only if the field is non-empty
- **AND** an empty `mqttPass` field SHALL NOT clear the existing password
- **AND** a special value `__CLEAR__` SHALL clear the password
