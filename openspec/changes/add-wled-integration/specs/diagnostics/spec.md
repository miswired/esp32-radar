# diagnostics Specification Delta

## ADDED Requirements

### Requirement: WLED Event Logging
The system SHALL log WLED-related events to the event system.

#### Scenario: Successful WLED request logged
- **WHEN** a WLED payload is successfully sent (HTTP 2xx response)
- **THEN** a `WLED_SENT` event SHALL be logged
- **AND** the event data SHALL include the HTTP response code

#### Scenario: Failed WLED request logged
- **WHEN** a WLED payload fails to send (timeout, connection error, HTTP error)
- **THEN** a `WLED_FAILED` event SHALL be logged
- **AND** the event data SHALL include the error code or HTTP response code

#### Scenario: WLED events in diagnostics display
- **GIVEN** the user views the Diagnostics page event log
- **WHEN** WLED events have been logged
- **THEN** `WLED_SENT` events SHALL be displayed with appropriate styling
- **AND** `WLED_FAILED` events SHALL be displayed with error styling
