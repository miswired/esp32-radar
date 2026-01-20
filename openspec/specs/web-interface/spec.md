# web-interface Specification

## Purpose
TBD - created by archiving change add-api-key-auth. Update Purpose after archive.
## Requirements
### Requirement: API Authentication Settings UI
The settings page SHALL include controls for API authentication configuration.

#### Scenario: Authentication section display
- **WHEN** the user visits the settings page
- **THEN** an "API Authentication" section SHALL be displayed
- **AND** the section SHALL include an enable/disable checkbox
- **AND** the section SHALL include an API key display field
- **AND** the section SHALL include a "Generate New Key" button

#### Scenario: API key masking
- **WHEN** the settings page loads
- **THEN** the API key SHALL be displayed as masked (e.g., ****-****-****-****)
- **AND** a "Show" button SHALL be available to reveal the key
- **AND** clicking "Show" SHALL change the button to "Hide"

#### Scenario: Key generation confirmation
- **WHEN** the user clicks "Generate New Key"
- **THEN** a confirmation dialog SHALL appear
- **AND** the dialog SHALL warn that the old key will be invalidated
- **AND** the user must confirm before the new key is generated

#### Scenario: Lockout status display
- **WHEN** the system is in authentication lockout state
- **AND** the user visits the settings page
- **THEN** the lockout status SHALL be displayed
- **AND** the remaining lockout time SHALL be shown

### Requirement: Generate Key API Endpoint
The system SHALL provide an endpoint to generate a new API key without saving.

#### Scenario: Generate key request
- **WHEN** a POST request is made to `/generate-key`
- **THEN** the system SHALL generate a new UUID v4 key
- **AND** the response SHALL be JSON: `{"key": "<new-uuid>"}`
- **AND** the key SHALL NOT be saved to NVS

### Requirement: API Documentation Authentication Section
The API documentation page SHALL include a comprehensive authentication section with clear examples.

#### Scenario: Authentication overview
- **WHEN** the user visits the API documentation page
- **THEN** an "Authentication" section SHALL be prominently displayed near the top
- **AND** the section SHALL explain that authentication is optional and disabled by default
- **AND** the section SHALL list which endpoints require authentication when enabled
- **AND** the section SHALL list which endpoints (HTML pages) never require authentication

#### Scenario: Header authentication examples
- **WHEN** the user views the authentication documentation
- **THEN** the X-API-Key header method SHALL be explained
- **AND** a curl example SHALL be provided showing header usage:
  - Example: `curl -H "X-API-Key: your-uuid-key" http://192.168.1.100/status`
- **AND** the example SHALL use realistic placeholder values

#### Scenario: Query parameter authentication examples
- **WHEN** the user views the authentication documentation
- **THEN** the apiKey query parameter method SHALL be explained
- **AND** a URL example SHALL be provided showing query parameter usage:
  - Example: `http://192.168.1.100/status?apiKey=your-uuid-key`
- **AND** a curl example SHALL also be provided for the query parameter method

#### Scenario: Examples without authentication
- **WHEN** the user views the API documentation
- **THEN** each endpoint section SHALL show an example without authentication
- **AND** each example SHALL be clearly labeled as "Without Authentication" or similar
- **AND** these examples SHALL work when authentication is disabled

#### Scenario: Examples with authentication
- **WHEN** the user views the API documentation
- **THEN** each protected endpoint section SHALL show an example with authentication
- **AND** the example SHALL demonstrate the X-API-Key header method
- **AND** the example SHALL be clearly labeled as "With Authentication" or similar

#### Scenario: Error response documentation
- **WHEN** the user views authentication documentation
- **THEN** the 401 Unauthorized response format SHALL be documented with example JSON
- **AND** the 429 Too Many Requests response format SHALL be documented with example JSON
- **AND** common troubleshooting tips SHALL be included (e.g., check key is correct, check if locked out)

### Requirement: API Documentation Organization
The API documentation page SHALL be well-organized and easy to follow.

#### Scenario: Clear section structure
- **WHEN** the user visits the API documentation page
- **THEN** the page SHALL have a clear visual hierarchy
- **AND** sections SHALL be logically grouped (Authentication, Status, Configuration, Diagnostics, etc.)
- **AND** each endpoint SHALL have its own clearly labeled subsection

#### Scenario: Endpoint documentation format
- **WHEN** the user views an endpoint's documentation
- **THEN** the following SHALL be displayed for each endpoint:
  - HTTP method and path (e.g., "GET /status")
  - Brief description of what the endpoint does
  - Whether authentication is required (with visual indicator)
  - Example request (curl command)
  - Example response (formatted JSON)

#### Scenario: Copy-friendly examples
- **WHEN** the user views curl examples
- **THEN** the examples SHALL be displayed in a monospace font
- **AND** the examples SHALL be complete and ready to copy/paste
- **AND** IP addresses in examples SHALL use a placeholder like `<device-ip>` or the actual device IP

### Requirement: Toast Notification System
The Settings page SHALL display save feedback using toast notifications that are visible regardless of scroll position.

#### Scenario: Success toast on save
- **WHEN** settings are saved successfully
- **THEN** a green toast notification SHALL appear at the bottom of the viewport
- **AND** the toast SHALL display the success message
- **AND** the toast SHALL auto-dismiss after 4 seconds

#### Scenario: Error toast on save failure
- **WHEN** settings fail to save
- **THEN** a red toast notification SHALL appear at the bottom of the viewport
- **AND** the toast SHALL display the error message
- **AND** the toast SHALL remain visible until manually dismissed

#### Scenario: Toast visibility
- **WHEN** a toast notification is displayed
- **THEN** it SHALL be visible regardless of the current scroll position
- **AND** it SHALL not obstruct critical page content
- **AND** it SHALL include a close button for manual dismissal

#### Scenario: Toast on test notification
- **WHEN** a test notification is sent
- **THEN** the result SHALL be displayed as a toast notification
- **AND** success SHALL show as green, failure as red

