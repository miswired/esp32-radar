# configuration Specification Delta

## ADDED Requirements

### Requirement: WLED Configuration Storage
The system SHALL persist WLED integration settings in NVS.

#### Scenario: WLED settings stored in NVS
- **WHEN** the user saves WLED settings
- **THEN** the following SHALL be stored in NVS:
  - `wledEnabled` (boolean) - Whether WLED integration is enabled
  - `wledUrl` (string, max 128 chars) - Full URL to WLED JSON API endpoint
  - `wledPayload` (string, max 512 chars) - JSON payload to send
- **AND** settings SHALL persist across power cycles

#### Scenario: WLED defaults on initialization
- **WHEN** the device boots with no saved WLED settings
- **THEN** `wledEnabled` SHALL be set to false
- **AND** `wledUrl` SHALL be empty
- **AND** `wledPayload` SHALL contain the default payload: `{"on":true,"bri":255,"seg":[{"col":[[255,0,0]]}]}`

#### Scenario: WLED cleared on factory reset
- **WHEN** a factory reset is performed
- **THEN** `wledEnabled` SHALL be set to false
- **AND** `wledUrl` SHALL be cleared
- **AND** `wledPayload` SHALL be reset to the default payload

### Requirement: WLED Fields in Config API
The configuration API endpoints SHALL include WLED settings.

#### Scenario: GET /config includes WLED settings
- **WHEN** a GET request is made to /config
- **THEN** the response SHALL include `wledEnabled` (boolean)
- **AND** the response SHALL include `wledUrl` (string)
- **AND** the response SHALL include `wledPayload` (string)

#### Scenario: POST /config accepts WLED settings
- **WHEN** a POST request to /config includes WLED fields
- **THEN** the system SHALL update `wledEnabled`, `wledUrl`, and `wledPayload`
- **AND** the system SHALL validate that `wledPayload` is valid JSON
- **AND** the system SHALL save the settings to NVS

#### Scenario: POST /config rejects invalid JSON payload
- **WHEN** a POST request to /config includes an invalid `wledPayload`
- **THEN** the system SHALL return an error response
- **AND** the response SHALL include a message indicating invalid JSON
- **AND** the settings SHALL NOT be saved

## MODIFIED Requirements

### Requirement: Notification Method Configuration
The system SHALL store notification methods as independent boolean flags instead of a single selection.

#### Scenario: Notification method fields in Config
- **WHEN** notification settings are stored
- **THEN** the following SHALL be stored in NVS:
  - `notifyGet` (boolean) - Whether to send GET request on alarm
  - `notifyPost` (boolean) - Whether to send POST request on alarm
- **AND** both flags MAY be enabled simultaneously

#### Scenario: Notification method defaults
- **WHEN** the device boots with no saved notification method settings
- **THEN** `notifyGet` SHALL be set to false
- **AND** `notifyPost` SHALL be set to false

#### Scenario: GET /config includes notification method flags
- **WHEN** a GET request is made to /config
- **THEN** the response SHALL include `notifyGet` (boolean)
- **AND** the response SHALL include `notifyPost` (boolean)
- **AND** the response SHALL NOT include the deprecated `notifyMethod` field

#### Scenario: POST /config accepts notification method flags
- **WHEN** a POST request to /config includes `notifyGet` and/or `notifyPost`
- **THEN** the system SHALL update the respective flags
- **AND** the system SHALL save the settings to NVS

#### Scenario: Migration from legacy notifyMethod
- **WHEN** the device loads configuration with legacy `notifyMethod` field
- **THEN** the system SHALL convert `notifyMethod=0` (GET) to `notifyGet=true, notifyPost=false`
- **AND** the system SHALL convert `notifyMethod=1` (POST) to `notifyGet=false, notifyPost=true`
- **AND** the system SHALL save the migrated settings in the new format
