# api-authentication Specification

## Purpose
TBD - created by archiving change add-api-key-auth. Update Purpose after archive.
## Requirements
### Requirement: API Key Format
The system SHALL use UUID version 4 format for API keys.

#### Scenario: Key generation format
- **WHEN** a new API key is generated
- **THEN** the key SHALL be in UUID v4 format (xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx)
- **AND** the key SHALL be 36 characters including hyphens
- **AND** the key SHALL be generated using ESP32 hardware random number generator

### Requirement: API Key Generation
The system SHALL generate cryptographically secure API keys on demand.

#### Scenario: Manual key generation
- **WHEN** the user clicks "Generate New Key" in settings
- **THEN** the system SHALL generate a new UUID v4 key
- **AND** the system SHALL display the new key in the settings form
- **AND** the system SHALL NOT save the key until the form is submitted

#### Scenario: Automatic key generation on enable
- **WHEN** API authentication is enabled for the first time
- **AND** no API key exists
- **THEN** the system SHALL automatically generate a new UUID v4 key
- **AND** the system SHALL save the key to NVS

### Requirement: Authentication Methods
The system SHALL support two methods for transmitting API keys.

#### Scenario: HTTP header authentication
- **WHEN** a request is made to a protected endpoint
- **AND** the request includes an `X-API-Key` header with the correct key
- **THEN** the system SHALL authenticate the request successfully

#### Scenario: Query parameter authentication
- **WHEN** a request is made to a protected endpoint
- **AND** the URL includes `?apiKey=<key>` with the correct key
- **THEN** the system SHALL authenticate the request successfully

#### Scenario: Header takes precedence
- **WHEN** a request includes both X-API-Key header and apiKey query parameter
- **THEN** the system SHALL use the header value for authentication

### Requirement: Protected Endpoints
The system SHALL require authentication for all JSON API endpoints when authentication is enabled.

#### Scenario: Protected endpoint list
- **WHEN** API authentication is enabled
- **THEN** the following endpoints SHALL require authentication:
  - `GET /status`
  - `GET /config`
  - `POST /config`
  - `GET /logs`
  - `GET /diagnostics`
  - `POST /test-notification`
  - `POST /reset`

#### Scenario: Unprotected HTML pages
- **WHEN** API authentication is enabled
- **THEN** the following pages SHALL remain accessible without authentication:
  - `GET /` (Dashboard)
  - `GET /settings` (Settings page)
  - `GET /diag` (Diagnostics page)
  - `GET /api` (API documentation)

### Requirement: Authentication Failure Response
The system SHALL return appropriate error responses for authentication failures.

#### Scenario: Missing API key
- **WHEN** a request is made to a protected endpoint without an API key
- **AND** API authentication is enabled
- **THEN** the system SHALL respond with HTTP 401 Unauthorized
- **AND** the response SHALL be JSON: `{"error": "Authentication required", "code": 401}`

#### Scenario: Invalid API key
- **WHEN** a request is made to a protected endpoint with an incorrect API key
- **THEN** the system SHALL respond with HTTP 401 Unauthorized
- **AND** the response SHALL be JSON: `{"error": "Invalid API key", "code": 401}`
- **AND** the system SHALL increment the failed attempt counter

### Requirement: Rate Limiting
The system SHALL implement rate limiting after repeated failed authentication attempts.

#### Scenario: Lockout after failures
- **WHEN** 5 consecutive authentication failures occur
- **THEN** the system SHALL activate a 5-minute lockout period
- **AND** all protected endpoints SHALL reject requests during lockout
- **AND** the system SHALL log the lockout event

#### Scenario: Lockout response
- **WHEN** a request is made during lockout period
- **THEN** the system SHALL respond with HTTP 429 Too Many Requests
- **AND** the response SHALL include remaining lockout time in seconds
- **AND** the response SHALL be JSON: `{"error": "Too many failed attempts", "code": 429, "retryAfter": <seconds>}`

#### Scenario: Lockout expiration
- **WHEN** the lockout period expires
- **THEN** the system SHALL clear the lockout state
- **AND** the system SHALL reset the failed attempt counter
- **AND** authentication attempts SHALL be accepted again

#### Scenario: Successful auth resets counter
- **WHEN** a successful authentication occurs
- **THEN** the system SHALL reset the failed attempt counter to zero

### Requirement: Authentication Toggle
The system SHALL allow users to enable or disable API authentication.

#### Scenario: Authentication disabled by default
- **WHEN** the system boots with factory defaults
- **THEN** API authentication SHALL be disabled
- **AND** all endpoints SHALL be accessible without a key

#### Scenario: Enabling authentication
- **WHEN** the user enables API authentication in settings
- **THEN** protected endpoints SHALL require a valid API key
- **AND** the setting SHALL persist across reboots

#### Scenario: Disabling authentication
- **WHEN** the user disables API authentication in settings
- **THEN** all endpoints SHALL be accessible without a key
- **AND** the existing API key SHALL be preserved (not deleted)

