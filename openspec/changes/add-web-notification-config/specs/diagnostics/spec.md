# Diagnostics Capability

## ADDED Requirements

### Requirement: Real-Time Sensor Data Display
The web interface SHALL display real-time sensor data including presence status, motion status, and last update time.

#### Scenario: Displaying current sensor status
- **WHEN** a user accesses the diagnostics section of the web interface
- **THEN** the interface SHALL display current presence status (Yes/No)
- **AND** the interface SHALL display current motion status (Detected/None)
- **AND** the interface SHALL display timestamp of last sensor reading
- **AND** the interface SHALL display detection distance (if available from sensor)
- **AND** the interface SHALL display detection speed (if available from sensor)

#### Scenario: Auto-refresh of sensor data
- **WHEN** the diagnostics page is open
- **THEN** sensor data SHALL automatically update every 5 seconds
- **AND** updates SHALL occur without full page reload

### Requirement: System Status Monitoring
The web interface SHALL display system health metrics including uptime, memory usage, and WiFi status.

#### Scenario: Displaying system uptime
- **WHEN** a user accesses the diagnostics section
- **THEN** the interface SHALL display system uptime in days, hours, minutes, and seconds
- **AND** the uptime SHALL be calculated from boot time using `millis()`

#### Scenario: Displaying memory usage
- **WHEN** a user accesses the diagnostics section
- **THEN** the interface SHALL display free heap memory in kilobytes
- **AND** the interface SHALL display total heap size
- **AND** the interface SHALL calculate and display heap usage percentage

#### Scenario: Displaying WiFi status
- **WHEN** a user accesses the diagnostics section
- **THEN** the interface SHALL display WiFi connection status (Connected/Disconnected)
- **AND** if connected, the interface SHALL display SSID
- **AND** if connected, the interface SHALL display RSSI (signal strength in dBm)
- **AND** if connected, the interface SHALL display IP address

### Requirement: I2C Communication Health
The web interface SHALL display I2C communication health status with the C4001 sensor.

#### Scenario: I2C communication healthy
- **WHEN** the C4001 sensor is responding normally to I2C queries
- **THEN** the diagnostics SHALL display I2C status as "OK" or "Healthy"
- **AND** the interface SHALL show a green indicator

#### Scenario: I2C communication errors
- **WHEN** I2C communication errors occur (sensor disconnected or bus issues)
- **THEN** the diagnostics SHALL display I2C status as "Error" or "Degraded"
- **AND** the interface SHALL show a red indicator
- **AND** the interface SHALL display error count

### Requirement: Event Log
The system SHALL maintain a circular buffer of the last 50 events and display them in the web interface.

#### Scenario: Logging events to circular buffer
- **WHEN** a significant event occurs (state change, notification sent, error, configuration change)
- **THEN** the system SHALL add the event to the circular buffer with timestamp
- **AND** if the buffer is full, the oldest event SHALL be overwritten

#### Scenario: Displaying event log
- **WHEN** a user accesses the diagnostics section
- **THEN** the interface SHALL display all events in the circular buffer
- **AND** events SHALL be displayed in reverse chronological order (newest first)
- **AND** each event SHALL show timestamp, event type, and description

#### Scenario: Event log auto-refresh
- **WHEN** the diagnostics page is open
- **THEN** the event log SHALL automatically refresh every 5 seconds
- **AND** new events SHALL appear at the top of the log

### Requirement: Event Log Types
The system SHALL log the following event types to the circular buffer.

#### Scenario: Presence state change event
- **WHEN** presence state changes from FALSE to TRUE or TRUE to FALSE
- **THEN** the system SHALL log an event: "Presence detected" or "Presence cleared"

#### Scenario: Motion state change event
- **WHEN** motion state changes
- **THEN** the system SHALL log an event: "Motion detected" or "Motion stopped"

#### Scenario: Notification sent event
- **WHEN** a notification is sent successfully
- **THEN** the system SHALL log an event: "Notification sent (GET/POST/SOCKET) to <IP>:<port>"

#### Scenario: Notification failed event
- **WHEN** a notification fails to send
- **THEN** the system SHALL log an event: "Notification failed (GET/POST/SOCKET): <error reason>"

#### Scenario: Configuration change event
- **WHEN** configuration is updated via web interface
- **THEN** the system SHALL log an event: "Configuration updated"

#### Scenario: I2C error event
- **WHEN** an I2C communication error occurs
- **THEN** the system SHALL log an event: "I2C error: <error description>"

#### Scenario: WiFi connection event
- **WHEN** WiFi connects or disconnects
- **THEN** the system SHALL log an event: "WiFi connected to <SSID>" or "WiFi disconnected"

#### Scenario: Factory reset event
- **WHEN** a factory reset is performed
- **THEN** the system SHALL log an event: "Factory reset initiated" before clearing NVS

### Requirement: Notification History
The web interface SHALL display notification history including recent sends and their status.

#### Scenario: Displaying notification history
- **WHEN** a user accesses the diagnostics section
- **THEN** the interface SHALL display the last 10 notification attempts
- **AND** each entry SHALL show timestamp, type (GET/POST/SOCKET), target, and status (success/failure)

#### Scenario: Notification success indicator
- **WHEN** a notification was sent successfully
- **THEN** the history entry SHALL display a green "Success" indicator

#### Scenario: Notification failure indicator
- **WHEN** a notification failed
- **THEN** the history entry SHALL display a red "Failed" indicator
- **AND** the entry SHALL show the error reason (e.g., "Timeout", "Connection refused")

### Requirement: Diagnostics API Endpoint
The system SHALL provide a JSON API endpoint at /diagnostics returning comprehensive system diagnostics.

#### Scenario: Querying diagnostics endpoint
- **WHEN** an HTTP GET request is made to /diagnostics
- **THEN** the system SHALL respond with HTTP 200 OK
- **AND** the response SHALL be valid JSON containing:
  - Current sensor status (presence, motion)
  - System uptime (milliseconds)
  - Free heap memory (bytes)
  - WiFi status (connected, SSID, RSSI, IP)
  - I2C status (ok, error count)
  - Last 50 events (array)

#### Scenario: Diagnostics endpoint format
- **WHEN** /diagnostics returns data
- **THEN** the JSON SHALL follow this structure:
```json
{
  "sensor": {"presence": true, "motion": false},
  "system": {"uptime_ms": 3600000, "free_heap": 245000, "heap_size": 327680},
  "wifi": {"connected": true, "ssid": "MyNetwork", "rssi": -45, "ip": "192.168.1.50"},
  "i2c": {"status": "ok", "errors": 0},
  "events": [{"timestamp": 1673891234, "type": "presence", "message": "Presence detected"}]
}
```

### Requirement: Events API Endpoint
The system SHALL provide a JSON API endpoint at /events returning the event log.

#### Scenario: Querying events endpoint
- **WHEN** an HTTP GET request is made to /events
- **THEN** the system SHALL respond with HTTP 200 OK
- **AND** the response SHALL be a JSON array of events
- **AND** events SHALL be ordered from newest to oldest

#### Scenario: Events endpoint format
- **WHEN** /events returns data
- **THEN** the JSON SHALL follow this structure:
```json
[
  {"timestamp": 1673891240, "type": "notification", "message": "Notification sent (POST) to 192.168.1.100:8080"},
  {"timestamp": 1673891235, "type": "presence", "message": "Presence detected"}
]
```

### Requirement: Visual Status Indicators
The web interface SHALL use color-coded visual indicators for system status.

#### Scenario: Green indicator for healthy status
- **WHEN** a system component is functioning normally (I2C OK, WiFi connected, notifications succeeding)
- **THEN** the web interface SHALL display a green indicator (color: #28a745 or equivalent)

#### Scenario: Red indicator for error status
- **WHEN** a system component has errors (I2C failure, WiFi disconnected, notifications failing)
- **THEN** the web interface SHALL display a red indicator (color: #dc3545 or equivalent)

#### Scenario: Yellow indicator for warning status
- **WHEN** a system component is degraded but functional (WiFi weak signal, memory low)
- **THEN** the web interface SHALL display a yellow indicator (color: #ffc107 or equivalent)
