## MODIFIED Requirements

### Requirement: Protected Endpoints
The system SHALL require API key authentication for write operations (POST endpoints) when API authentication is enabled. This protects external API access while web interface uses separate password authentication.

#### Scenario: Protected endpoint list (write operations)
- **WHEN** API authentication is enabled
- **THEN** the following endpoints SHALL require API key authentication:
  - `POST /config` (save configuration)
  - `POST /test-notification` (send test notification)
  - `POST /reset` (factory reset)

#### Scenario: Unprotected GET endpoints (read operations)
- **WHEN** API authentication is enabled
- **THEN** the following JSON endpoints SHALL remain accessible without API key:
  - `GET /status` (sensor status)
  - `GET /config` (current configuration)
  - `GET /logs` (event log)
  - `GET /diagnostics` (system diagnostics)

#### Scenario: Unprotected HTML pages
- **WHEN** API authentication is enabled
- **THEN** HTML pages SHALL remain accessible without API key authentication
- **AND** the web interface uses separate password authentication

#### Scenario: Dual authentication model
- **GIVEN** the system has two separate authentication mechanisms
- **WHEN** configuring security
- **THEN** API key SHALL be used for external integrations (curl, scripts, Home Assistant)
- **AND** web password SHALL be used for browser-based administration
- **AND** these credentials SHALL be completely independent
