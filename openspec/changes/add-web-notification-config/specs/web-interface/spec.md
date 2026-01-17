# Web Interface Capability

## ADDED Requirements

### Requirement: WiFi Connectivity
The system SHALL connect to a WiFi network using stored credentials and provide HTTP access to the web interface.

#### Scenario: Successful WiFi connection
- **WHEN** the ESP32 boots with valid WiFi credentials
- **THEN** the system SHALL connect to the specified WiFi network
- **AND** the system SHALL obtain an IP address via DHCP
- **AND** the system SHALL log the assigned IP address to serial

#### Scenario: WiFi connection failure
- **WHEN** the ESP32 cannot connect to WiFi (invalid credentials or network unavailable)
- **THEN** the system SHALL retry connection every 5 seconds for up to 30 seconds
- **AND** the system SHALL log connection failure to serial
- **AND** the system SHALL continue sensor operation even without WiFi

#### Scenario: WiFi reconnection after network outage
- **WHEN** WiFi connection is lost during operation
- **THEN** the system SHALL detect disconnection
- **AND** the system SHALL automatically attempt to reconnect
- **AND** the system SHALL continue sensor operation during reconnection attempts

### Requirement: Access Point (AP) Fallback Mode
The system SHALL activate Access Point mode when WiFi connection fails, allowing initial configuration without pre-configured credentials.

#### Scenario: AP mode activation on connection failure
- **WHEN** the ESP32 cannot connect to WiFi after retry attempts
- **THEN** the system SHALL start Access Point mode
- **AND** the system SHALL broadcast SSID "ESP32-Radar-Setup"
- **AND** the system SHALL configure AP IP address as 192.168.4.1
- **AND** the system SHALL log AP mode activation to serial

#### Scenario: Web interface access in AP mode
- **WHEN** the ESP32 is operating in AP mode
- **AND** a user connects to the "ESP32-Radar-Setup" network
- **THEN** the user SHALL be able to access the web interface at 192.168.4.1
- **AND** the web interface SHALL display a WiFi configuration form
- **AND** the interface SHALL indicate that the device is in AP mode

#### Scenario: WiFi configuration in AP mode
- **WHEN** a user submits WiFi credentials via the AP mode web interface
- **THEN** the system SHALL save the credentials to NVS
- **AND** the system SHALL respond with a success message
- **AND** the system SHALL restart to attempt connection with new credentials

#### Scenario: Switching from AP to STA mode
- **WHEN** the ESP32 successfully connects to a WiFi network after being in AP mode
- **THEN** the system SHALL switch to Station (STA) mode
- **AND** the system SHALL disable AP mode
- **AND** the system SHALL log the successful mode switch to serial

### Requirement: Async Web Server
The system SHALL run an asynchronous HTTP web server on port 80 that does not block sensor operations.

#### Scenario: Web server initialization
- **WHEN** WiFi connection is established
- **THEN** the system SHALL start the async web server on port 80
- **AND** the system SHALL log successful server start to serial

#### Scenario: Handling concurrent requests
- **WHEN** multiple HTTP requests arrive simultaneously
- **THEN** the async web server SHALL handle all requests concurrently
- **AND** sensor polling SHALL continue without interruption

### Requirement: HTML Dashboard
The system SHALL serve an HTML dashboard at the root URL (/) displaying real-time sensor status.

#### Scenario: Accessing the dashboard
- **WHEN** a user navigates to the ESP32's IP address in a web browser
- **THEN** the system SHALL serve the HTML dashboard
- **AND** the dashboard SHALL display current presence status (Yes/No)
- **AND** the dashboard SHALL display current motion status (Detected/None)
- **AND** the dashboard SHALL display the ESP32's IP address
- **AND** the dashboard SHALL display a last update timestamp

#### Scenario: Dashboard auto-refresh
- **WHEN** the dashboard is open in a browser
- **THEN** the dashboard SHALL automatically refresh sensor data every 5 seconds
- **AND** the refresh SHALL occur without reloading the entire page

### Requirement: Status API Endpoint
The system SHALL provide a JSON API endpoint at /status returning current sensor status.

#### Scenario: Querying status endpoint
- **WHEN** an HTTP GET request is made to /status
- **THEN** the system SHALL respond with HTTP 200 OK
- **AND** the response body SHALL be valid JSON
- **AND** the JSON SHALL include "presence" field (boolean)
- **AND** the JSON SHALL include "motion" field (boolean)
- **AND** the JSON SHALL include "timestamp" field (Unix timestamp)

#### Scenario: Status endpoint format
- **WHEN** /status returns sensor data
- **THEN** the JSON format SHALL be: `{"presence": true, "motion": false, "timestamp": 1673891234}`

### Requirement: Configuration API Endpoints
The system SHALL provide JSON API endpoints at /config for reading and writing configuration.

#### Scenario: Reading configuration (GET /config)
- **WHEN** an HTTP GET request is made to /config
- **THEN** the system SHALL respond with HTTP 200 OK
- **AND** the response SHALL contain all configuration parameters in JSON format
- **AND** sensitive fields (WiFi password) SHALL be redacted or omitted

#### Scenario: Writing configuration (POST /config)
- **WHEN** an HTTP POST request with JSON body is made to /config
- **THEN** the system SHALL parse the JSON payload
- **AND** the system SHALL update configuration parameters
- **AND** the system SHALL save configuration to NVS
- **AND** the system SHALL respond with HTTP 200 OK and confirmation message

#### Scenario: Invalid JSON in configuration request
- **WHEN** an HTTP POST request with invalid JSON is made to /config
- **THEN** the system SHALL respond with HTTP 400 Bad Request
- **AND** the response SHALL include an error message describing the issue

### Requirement: Responsive Design
The web interface SHALL be usable on desktop and mobile browsers.

#### Scenario: Desktop browser access
- **WHEN** the dashboard is accessed from a desktop browser (Chrome, Firefox, Safari)
- **THEN** the interface SHALL render correctly with appropriate layout
- **AND** all controls SHALL be functional

#### Scenario: Mobile browser access
- **WHEN** the dashboard is accessed from a mobile browser (iOS Safari, Android Chrome)
- **THEN** the interface SHALL render with mobile-optimized layout
- **AND** all controls SHALL be touch-friendly and functional
