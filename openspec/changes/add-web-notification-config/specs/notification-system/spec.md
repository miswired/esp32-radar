# Notification System Capability

## ADDED Requirements

### Requirement: HTTP GET Notifications
The system SHALL send HTTP GET requests to a configurable endpoint when presence state changes.

#### Scenario: Presence detected notification (GET)
- **WHEN** presence state changes from FALSE to TRUE
- **AND** notifications are enabled
- **AND** notification type is HTTP GET
- **THEN** the system SHALL send an HTTP GET request to the configured URL
- **AND** the URL SHALL include query parameters: `?presence=1&motion=0` (or 1 depending on motion state)
- **AND** the request SHALL have a 1-second timeout
- **AND** the system SHALL not block waiting for response (fire-and-forget)

#### Scenario: Presence cleared notification (GET)
- **WHEN** presence state changes from TRUE to FALSE after presence timeout expires
- **AND** notifications are enabled
- **AND** notification type is HTTP GET
- **THEN** the system SHALL send an HTTP GET request with `?presence=0&motion=0`

### Requirement: HTTP POST Notifications
The system SHALL send HTTP POST requests with JSON payload to a configurable endpoint when presence state changes.

#### Scenario: Presence detected notification (POST)
- **WHEN** presence state changes from FALSE to TRUE
- **AND** notifications are enabled
- **AND** notification type is HTTP POST
- **THEN** the system SHALL send an HTTP POST request to the configured URL
- **AND** the Content-Type header SHALL be "application/json"
- **AND** the request body SHALL be valid JSON: `{"presence": true, "motion": false, "timestamp": 1673891234}`
- **AND** the request SHALL have a 1-second timeout

#### Scenario: Presence cleared notification (POST)
- **WHEN** presence state changes from TRUE to FALSE after presence timeout expires
- **AND** notifications are enabled
- **AND** notification type is HTTP POST
- **THEN** the system SHALL send an HTTP POST request with `{"presence": false, "motion": false, "timestamp": <current>}`

### Requirement: Raw Socket Notifications
The system SHALL send raw TCP socket messages to a configurable IP and port when presence state changes.

#### Scenario: Presence detected notification (Socket)
- **WHEN** presence state changes from FALSE to TRUE
- **AND** notifications are enabled
- **AND** notification type is RAW SOCKET
- **THEN** the system SHALL open a TCP connection to configured IP and port
- **AND** the system SHALL send the message: `presence=1,motion=0\n`
- **AND** the system SHALL close the connection immediately (fire-and-forget)
- **AND** the connection SHALL have a 1-second timeout

#### Scenario: Socket connection failure
- **WHEN** the system attempts to send a socket notification
- **AND** the target IP:port is unreachable
- **THEN** the system SHALL timeout after 1 second
- **AND** the system SHALL log the failure to the event log
- **AND** the system SHALL continue normal operation

### Requirement: Fire-and-Forget Delivery
The system SHALL send notifications without waiting for acknowledgment or retrying failures.

#### Scenario: Notification sent without blocking
- **WHEN** a notification is triggered
- **THEN** the system SHALL send the notification asynchronously
- **AND** the main loop SHALL not block waiting for response
- **AND** sensor polling SHALL continue without interruption

#### Scenario: Notification failure does not affect operation
- **WHEN** a notification fails (timeout, connection refused, etc.)
- **THEN** the system SHALL log the failure
- **AND** the system SHALL continue normal sensor operation
- **AND** the system SHALL NOT retry the failed notification

### Requirement: State Change Triggering
The system SHALL send notifications only when presence state changes (not on every sensor reading).

#### Scenario: Notification on state change only
- **WHEN** presence state changes from FALSE to TRUE (or TRUE to FALSE)
- **AND** the change has persisted beyond debounce time
- **THEN** the system SHALL send exactly one notification
- **AND** subsequent sensor readings with the same state SHALL not trigger notifications

#### Scenario: No notification when state unchanged
- **WHEN** sensor readings show presence=TRUE on consecutive polls
- **AND** the presence state was already TRUE before the reading
- **THEN** the system SHALL NOT send a notification

### Requirement: Debounce Logic
The system SHALL apply debounce logic to prevent notification spam on rapid state changes.

#### Scenario: Debounce filters rapid state flicker
- **WHEN** presence state changes from FALSE to TRUE and back to FALSE within debounce time
- **THEN** the system SHALL NOT send any notifications
- **AND** the state change SHALL be ignored as transient noise

#### Scenario: State change persists beyond debounce
- **WHEN** presence state changes from FALSE to TRUE
- **AND** the state remains TRUE for longer than debounce time (default 500ms)
- **THEN** the system SHALL recognize the state change as stable
- **AND** the system SHALL send a notification

### Requirement: Presence Timeout
The system SHALL apply a presence timeout before declaring "no presence" to prevent false negatives.

#### Scenario: Presence timeout prevents premature absence notification
- **WHEN** presence sensor reports FALSE (no presence)
- **AND** previous state was TRUE (presence detected)
- **AND** less than presence timeout (default 10 seconds) has elapsed
- **THEN** the system SHALL maintain presence state as TRUE
- **AND** the system SHALL NOT send "no presence" notification

#### Scenario: Presence timeout expires
- **WHEN** presence sensor reports FALSE continuously for presence timeout duration
- **THEN** the system SHALL change presence state to FALSE
- **AND** the system SHALL send "no presence" notification

### Requirement: Test Notification Function
The system SHALL provide a manual test notification feature in the web interface.

#### Scenario: Sending test notification
- **WHEN** a user clicks "Test Notification" button in web interface
- **THEN** the system SHALL send a notification using current configuration
- **AND** the notification payload SHALL indicate it is a test: `{"presence": false, "motion": false, "test": true}`
- **AND** the system SHALL display success or failure feedback in the web UI

#### Scenario: Test notification with invalid configuration
- **WHEN** a user clicks "Test Notification" with invalid endpoint configuration
- **THEN** the system SHALL attempt to send the notification
- **AND** the system SHALL log the failure
- **AND** the system SHALL display error feedback in the web UI

### Requirement: Notification Status Logging
The system SHALL log all notification attempts and their outcomes to the event log.

#### Scenario: Successful notification logged
- **WHEN** a notification is sent successfully (HTTP 200 response or socket connection succeeded)
- **THEN** the system SHALL log a success event with timestamp and notification type

#### Scenario: Failed notification logged
- **WHEN** a notification fails (timeout, connection refused, HTTP error)
- **THEN** the system SHALL log a failure event with timestamp, notification type, and error reason
