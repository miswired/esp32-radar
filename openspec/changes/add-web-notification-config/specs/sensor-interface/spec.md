# Sensor Interface Capability

## ADDED Requirements

### Requirement: I2C Communication with C4001 Sensor
The system SHALL establish I2C communication with the DFRobot C4001 mmWave sensor using address 0x2A on GPIO 21 (SDA) and GPIO 22 (SCL).

#### Scenario: Successful sensor initialization
- **WHEN** the ESP32 boots and initializes the I2C bus
- **THEN** the C4001 sensor SHALL respond to I2C queries at address 0x2A
- **AND** the system SHALL log successful initialization to serial output

#### Scenario: I2C communication failure
- **WHEN** the C4001 sensor does not respond on the I2C bus
- **THEN** the system SHALL log an I2C error message to serial
- **AND** the system SHALL attempt to re-initialize I2C communication on the next loop iteration

### Requirement: Presence Detection Reading
The system SHALL read presence detection status from the C4001 sensor at 1Hz (once per second).

#### Scenario: Presence detected
- **WHEN** a person is within the sensor's detection range (1.2m to 8m)
- **THEN** the sensor SHALL report presence as TRUE
- **AND** the system SHALL update the presence state variable

#### Scenario: No presence detected
- **WHEN** no person is within the sensor's detection range
- **THEN** the sensor SHALL report presence as FALSE
- **AND** the system SHALL update the presence state variable

### Requirement: Motion Detection Reading
The system SHALL read motion detection status from the C4001 sensor at 1Hz (once per second).

#### Scenario: Motion detected
- **WHEN** movement is detected within the sensor's range (1.2m to 12m)
- **THEN** the sensor SHALL report motion as TRUE
- **AND** the system SHALL update the motion state variable

#### Scenario: No motion detected
- **WHEN** no movement is detected within the sensor's range
- **THEN** the sensor SHALL report motion as FALSE
- **AND** the system SHALL update the motion state variable

### Requirement: Serial Output for Debugging
The system SHALL output sensor readings to the serial console at 115200 baud for debugging purposes.

#### Scenario: Serial output on state change
- **WHEN** presence or motion status changes
- **THEN** the system SHALL print a timestamped message to serial
- **AND** the message SHALL include both presence and motion status

#### Scenario: Periodic serial output
- **WHEN** one second has elapsed since the last sensor reading
- **THEN** the system SHALL print current sensor status to serial
- **AND** the output SHALL include timestamp, presence, and motion status

### Requirement: Error Handling for Sensor Disconnection
The system SHALL gracefully handle sensor disconnection and continue operating.

#### Scenario: Sensor physically disconnected
- **WHEN** the C4001 sensor is disconnected from the I2C bus
- **THEN** the system SHALL detect I2C communication failure
- **AND** the system SHALL log an error message
- **AND** the system SHALL continue attempting to read the sensor on subsequent iterations

#### Scenario: Sensor reconnected after disconnection
- **WHEN** the C4001 sensor is reconnected to the I2C bus after disconnection
- **THEN** the system SHALL resume normal I2C communication
- **AND** the system SHALL log successful reconnection
