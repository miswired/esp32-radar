# Configuration Capability

## ADDED Requirements

### Requirement: Authentication Configuration Storage
The system SHALL persist API authentication settings in NVS.

#### Scenario: Auth settings stored in NVS
- **WHEN** the user saves authentication settings
- **THEN** the following SHALL be stored in NVS:
  - `authEnabled` (boolean) - Whether authentication is enabled
  - `apiKey` (string, max 36 chars) - The UUID format API key
- **AND** settings SHALL persist across power cycles

#### Scenario: Auth defaults on factory reset
- **WHEN** a factory reset is performed
- **THEN** `authEnabled` SHALL be set to false
- **AND** `apiKey` SHALL be cleared (empty string)
- **AND** the failed attempt counter SHALL be reset

### Requirement: Auth Fields in Config API
The configuration API endpoints SHALL include authentication settings.

#### Scenario: GET /config includes auth settings
- **WHEN** a GET request is made to /config
- **THEN** the response SHALL include `authEnabled` (boolean)
- **AND** the response SHALL include `authConfigured` (boolean, true if key exists)
- **AND** the response SHALL NOT include the actual API key (security)

#### Scenario: POST /config accepts auth settings
- **WHEN** a POST request to /config includes `authEnabled`
- **THEN** the system SHALL update the authentication enabled state
- **AND** the system SHALL save the setting to NVS

#### Scenario: POST /config accepts new API key
- **WHEN** a POST request to /config includes `apiKey`
- **THEN** the system SHALL validate the key is UUID format (36 chars)
- **AND** the system SHALL save the new key to NVS
- **AND** the system SHALL reset the failed attempt counter
