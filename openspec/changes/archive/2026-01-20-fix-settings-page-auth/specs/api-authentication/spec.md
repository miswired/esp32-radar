## MODIFIED Requirements

### Requirement: Protected Endpoints
The system SHALL require authentication only for external API access. Web UI operations SHALL NOT require API key authentication to allow local administration.

#### Scenario: No endpoints require API key authentication
- **WHEN** API authentication is enabled
- **THEN** no endpoints SHALL require API key authentication
- **AND** the API key feature serves as an optional security layer for external integrations
- **AND** the web UI SHALL function fully regardless of API key settings

#### Scenario: Unprotected endpoints
- **WHEN** API authentication is enabled or disabled
- **THEN** all endpoints SHALL remain accessible without API key authentication:
  - `GET /status` (sensor status)
  - `GET /config` (current configuration)
  - `GET /logs` (event log)
  - `GET /diagnostics` (system diagnostics)
  - `POST /config` (save configuration)
  - `POST /test-notification` (send test notification)
  - `POST /reset` (factory reset)
- **AND** this allows both the web UI and external tools to function

#### Scenario: Unprotected HTML pages
- **WHEN** API authentication is enabled
- **THEN** the following pages SHALL remain accessible without authentication:
  - `GET /` (Dashboard)
  - `GET /settings` (Settings page)
  - `GET /diag` (Diagnostics page)
  - `GET /api` (API documentation)

#### Scenario: Future web interface password protection
- **GIVEN** the API key authentication does not protect web UI operations
- **WHEN** web interface security is required
- **THEN** a separate password protection mechanism SHOULD be implemented for web access
