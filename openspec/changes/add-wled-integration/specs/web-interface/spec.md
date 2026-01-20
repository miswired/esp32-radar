# web-interface Specification Delta

## ADDED Requirements

### Requirement: WLED Settings UI Section
The Settings page SHALL include a WLED integration configuration section.

#### Scenario: WLED section display
- **GIVEN** the user is on the Settings page and logged in
- **THEN** a "WLED Integration" section SHALL be displayed
- **AND** the section SHALL appear after the "Notifications" section
- **AND** the section SHALL be visually consistent with other settings sections

#### Scenario: WLED enable checkbox
- **GIVEN** the WLED section is displayed
- **THEN** an "Enable WLED" checkbox SHALL be present
- **AND** the checkbox label SHALL indicate "Send payload to WLED device on alarm"
- **AND** the checkbox SHALL reflect the current `wledEnabled` setting

#### Scenario: WLED URL field
- **GIVEN** the WLED section is displayed
- **THEN** a "WLED URL" text input field SHALL be present
- **AND** the field SHALL have a placeholder showing example URL: "http://192.168.1.50/json/state"
- **AND** the field SHALL support URLs up to 128 characters
- **AND** the field SHALL display the current `wledUrl` setting

#### Scenario: WLED payload textarea
- **GIVEN** the WLED section is displayed
- **THEN** a "JSON Payload" textarea SHALL be present
- **AND** the textarea SHALL be multi-line (4-6 rows visible)
- **AND** the textarea SHALL use a monospace font
- **AND** the textarea SHALL support payloads up to 512 characters
- **AND** the textarea SHALL display the current `wledPayload` setting
- **AND** new users SHALL see the default payload pre-populated

#### Scenario: JSON validation feedback
- **GIVEN** the user enters text in the JSON payload textarea
- **WHEN** the payload is invalid JSON
- **THEN** an error message SHALL be displayed below the textarea
- **AND** the Save button SHOULD indicate the form has errors

### Requirement: Test WLED Button
The Settings page SHALL provide a button to test WLED configuration.

#### Scenario: Test button display
- **GIVEN** the WLED section is displayed
- **THEN** a "Test WLED" button SHALL be present
- **AND** the button SHALL be styled consistently with the existing "Test Notification" button

#### Scenario: Test with current form values
- **GIVEN** the user has entered values in the WLED URL and payload fields
- **WHEN** the user clicks "Test WLED"
- **THEN** the system SHALL send the current form values (not necessarily saved values)
- **AND** a toast notification SHALL indicate success or failure
- **AND** the toast SHALL include the HTTP response code on failure

#### Scenario: Test button authentication
- **GIVEN** API key authentication is enabled
- **WHEN** the user clicks "Test WLED" while logged in via web session
- **THEN** the test SHALL be allowed (session auth accepted)

### Requirement: Test WLED API Endpoint
The system SHALL provide an API endpoint to test WLED configuration.

#### Scenario: POST /test-wled endpoint
- **WHEN** a POST request is made to /test-wled
- **THEN** the system SHALL send the configured payload to the configured URL
- **AND** the response SHALL be JSON with `success` (boolean) and `message` (string)
- **AND** the response SHALL include `httpCode` on failure

#### Scenario: Test with request parameters
- **WHEN** a POST request to /test-wled includes `url` and `payload` parameters
- **THEN** the system SHALL use the provided values instead of saved configuration
- **AND** this allows testing unsaved values from the settings form

#### Scenario: Test endpoint authentication
- **GIVEN** API key authentication is enabled
- **WHEN** a POST request is made to /test-wled
- **THEN** the request SHALL require authentication (session cookie OR API key)
- **AND** unauthenticated requests SHALL receive 401 response

## MODIFIED Requirements

### Requirement: Notification Method Selection UI
The Settings page SHALL use checkboxes instead of a dropdown for notification method selection.

#### Scenario: Notification method checkboxes display
- **GIVEN** the user is on the Settings page in the Notifications section
- **THEN** a "Send GET request" checkbox SHALL be displayed
- **AND** a "Send POST request" checkbox SHALL be displayed
- **AND** the notification method dropdown SHALL NOT be displayed

#### Scenario: Independent checkbox selection
- **GIVEN** the notification method checkboxes are displayed
- **THEN** the user SHALL be able to enable GET only
- **OR** the user SHALL be able to enable POST only
- **OR** the user SHALL be able to enable both GET and POST
- **OR** the user SHALL be able to disable both

#### Scenario: Checkbox state reflects configuration
- **GIVEN** the Settings page loads
- **THEN** the "Send GET request" checkbox SHALL reflect the `notifyGet` setting
- **AND** the "Send POST request" checkbox SHALL reflect the `notifyPost` setting

#### Scenario: Checkbox changes saved to configuration
- **GIVEN** the user changes the notification method checkboxes
- **WHEN** the user saves settings
- **THEN** the `notifyGet` and `notifyPost` flags SHALL be updated accordingly
