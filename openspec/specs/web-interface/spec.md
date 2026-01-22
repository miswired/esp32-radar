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

### Requirement: Web Password Authentication
The system SHALL provide password-based authentication for the Settings page to prevent unauthorized configuration changes.

#### Scenario: First-time password setup
- **WHEN** a user visits the Settings page for the first time
- **AND** no password has been configured
- **THEN** the system SHALL display a modal prompting the user to create a password
- **AND** the modal SHALL require a minimum of 6 characters
- **AND** the modal SHALL suggest using special characters and numbers
- **AND** the modal SHALL require password confirmation
- **AND** the modal SHALL display a warning about HTTP being unencrypted

#### Scenario: Login required for Settings
- **WHEN** a password has been configured
- **AND** the user visits the Settings page without a valid session
- **THEN** the system SHALL display a login modal
- **AND** the modal SHALL display a warning about HTTP being unencrypted
- **AND** the user SHALL NOT be able to view or modify settings until authenticated

#### Scenario: Session management
- **WHEN** a user successfully logs in
- **THEN** the system SHALL create a session with a random 32-character hex token
- **AND** the session SHALL be stored in a cookie
- **AND** the session SHALL expire after 1 hour of inactivity
- **AND** the session SHALL expire when the browser is closed
- **AND** only one session SHALL be active at a time (new login invalidates previous)

#### Scenario: Public pages remain accessible
- **WHEN** a password has been configured
- **THEN** the Dashboard page SHALL remain accessible without authentication
- **AND** the Diagnostics page SHALL remain accessible without authentication
- **AND** the API documentation page SHALL remain accessible without authentication

#### Scenario: AP mode authentication
- **WHEN** the device is in AP fallback mode
- **AND** a password has been configured
- **THEN** the Settings page SHALL still require authentication

### Requirement: Navigation Bar Login Status
The system SHALL display login/logout controls in the navigation bar on all pages.

#### Scenario: Login button when logged out
- **WHEN** a password is configured
- **AND** the user is not logged in
- **THEN** the navigation bar SHALL display a "Login" button on all pages
- **AND** clicking the button SHALL navigate to the Settings page to trigger login modal

#### Scenario: Logout button when logged in
- **WHEN** the user is logged in
- **THEN** the navigation bar SHALL display a "Logout" button on all pages
- **AND** clicking the button SHALL clear the session and log out the user

### Requirement: Serial Password Reset
The system SHALL provide a serial command to reset the web password.

#### Scenario: Password reset via serial
- **WHEN** the user executes the password reset serial command
- **THEN** the system SHALL clear the stored password hash
- **AND** the system SHALL clear any active sessions
- **AND** the system SHALL return to the first-time setup state
- **AND** the next Settings page visit SHALL prompt for new password creation

### Requirement: About Page
The web interface SHALL include an About page displaying project and license information.

#### Scenario: About page accessible
- **GIVEN** the user is on any page of the web interface
- **WHEN** they click the "About" navigation link
- **THEN** the About page SHALL be displayed

#### Scenario: About page displays project information
- **GIVEN** the user is on the About page
- **THEN** the project name "ESP32 Microwave Motion Sensor" SHALL be displayed
- **AND** the current firmware version SHALL be displayed
- **AND** the author credit "Created by Miswired" SHALL be displayed

#### Scenario: About page displays license information
- **GIVEN** the user is on the About page
- **THEN** the license type "GNU General Public License v3.0" SHALL be displayed
- **AND** the copyright notice "Copyright 2024-2026 Miswired" SHALL be displayed
- **AND** a brief description of GPL v3 rights and obligations SHALL be provided
- **AND** a link to the full license text SHALL be provided

#### Scenario: About page displays documentation link
- **GIVEN** the user is on the About page
- **THEN** a link to the project documentation (GitHub Wiki) SHALL be provided
- **AND** a link to the project repository SHALL be provided

#### Scenario: About page displays device information
- **GIVEN** the user is on the About page
- **THEN** the device's chip ID SHALL be displayed
- **AND** the device's IP address SHALL be displayed
- **AND** the current uptime SHALL be displayed

