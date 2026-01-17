# Configuration Capability

## ADDED Requirements

### Requirement: Persistent Storage via NVS
The system SHALL store all configuration parameters in Non-Volatile Storage (NVS) using the ESP32 Preferences library.

#### Scenario: Saving configuration to NVS
- **WHEN** configuration parameters are updated via the web interface
- **THEN** the system SHALL write all parameters to NVS
- **AND** the write operation SHALL complete successfully before responding to the user
- **AND** the system SHALL log successful save to event log

#### Scenario: Loading configuration from NVS on boot
- **WHEN** the ESP32 boots
- **THEN** the system SHALL read configuration from NVS
- **AND** if no configuration exists, the system SHALL use default values
- **AND** the system SHALL log loaded configuration to serial

#### Scenario: Configuration persists across reboots
- **WHEN** configuration is saved and the ESP32 is rebooted
- **THEN** the system SHALL load the previously saved configuration
- **AND** all parameters SHALL match the values before reboot

### Requirement: WiFi Configuration Parameters
The system SHALL store WiFi SSID and password as configurable parameters.

#### Scenario: Configuring WiFi credentials
- **WHEN** a user submits WiFi SSID and password via the web interface
- **THEN** the system SHALL store both values in NVS
- **AND** the system SHALL attempt to reconnect to WiFi with new credentials
- **AND** the system SHALL provide feedback on connection success or failure

#### Scenario: Default WiFi credentials
- **WHEN** no WiFi credentials are stored in NVS
- **THEN** the system SHALL use default hardcoded credentials for initial setup
- **AND** the system SHALL prompt user to update credentials via web interface

### Requirement: Detection Configuration Parameters
The system SHALL store detection-related parameters including sensitivity, distance range, debounce time, and presence timeout.

#### Scenario: Configuring detection parameters
- **WHEN** a user updates detection parameters via the web interface
- **THEN** the system SHALL validate parameter ranges (e.g., min distance < max distance)
- **AND** the system SHALL store valid parameters in NVS
- **AND** the system SHALL apply parameters immediately without requiring reboot
- **AND** the system SHALL respond with success or validation error message

#### Scenario: Invalid detection parameters
- **WHEN** a user submits invalid parameters (e.g., min distance > max distance)
- **THEN** the system SHALL reject the update
- **AND** the system SHALL return an error message describing the validation failure
- **AND** the system SHALL NOT save invalid parameters to NVS

#### Scenario: Default detection parameters
- **WHEN** no detection parameters are stored in NVS
- **THEN** the system SHALL use default values:
  - Minimum distance: 1.2 meters
  - Maximum distance: 8.0 meters
  - Debounce time: 500 milliseconds
  - Presence timeout: 10 seconds

### Requirement: Notification Configuration Parameters
The system SHALL store notification-related parameters including enable flag, type, IP address, port, and URL path.

#### Scenario: Configuring notification endpoint
- **WHEN** a user configures a notification endpoint via the web interface
- **THEN** the system SHALL validate IP address format
- **AND** the system SHALL validate port range (1-65535)
- **AND** the system SHALL store valid parameters in NVS
- **AND** the system SHALL respond with success or validation error message

#### Scenario: Disabling notifications
- **WHEN** a user disables notifications via the web interface
- **THEN** the system SHALL set the notification enabled flag to FALSE
- **AND** the system SHALL store the updated flag in NVS
- **AND** the system SHALL stop sending notifications immediately

#### Scenario: Default notification parameters
- **WHEN** no notification parameters are stored in NVS
- **THEN** the system SHALL use default values:
  - Enabled: false
  - Type: HTTP POST
  - IP: 192.168.1.100
  - Port: 8080
  - URL Path: /notify

### Requirement: Factory Reset
The system SHALL provide a factory reset function to clear all stored configuration and revert to defaults.

#### Scenario: Executing factory reset
- **WHEN** a user triggers factory reset via the web interface
- **THEN** the system SHALL clear all NVS configuration data
- **AND** the system SHALL load default configuration values
- **AND** the system SHALL restart the ESP32
- **AND** the system SHALL log factory reset to serial before restarting

#### Scenario: Factory reset confirmation
- **WHEN** a user clicks the factory reset button
- **THEN** the web interface SHALL prompt for confirmation
- **AND** the reset SHALL only execute if confirmed

### Requirement: Configuration Validation
The system SHALL validate all configuration parameters before storing them in NVS.

#### Scenario: Valid configuration accepted
- **WHEN** a user submits valid configuration parameters
- **THEN** the system SHALL validate all fields
- **AND** the system SHALL store parameters in NVS
- **AND** the system SHALL apply parameters immediately

#### Scenario: Invalid configuration rejected
- **WHEN** a user submits invalid configuration (e.g., malformed IP address)
- **THEN** the system SHALL reject the submission
- **AND** the system SHALL return a descriptive error message
- **AND** the system SHALL retain previous valid configuration
