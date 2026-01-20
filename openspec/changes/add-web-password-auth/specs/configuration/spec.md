## ADDED Requirements

### Requirement: Web Password Storage
The system SHALL securely store the web interface password.

#### Scenario: Password hashing
- **WHEN** a user creates or changes their password
- **THEN** the system SHALL hash the password using SHA-256
- **AND** the hash SHALL be stored in NVS (non-volatile storage)
- **AND** the plaintext password SHALL NOT be stored

#### Scenario: Password persistence
- **WHEN** the device reboots
- **THEN** the password hash SHALL be preserved
- **AND** the user SHALL NOT need to recreate the password

#### Scenario: Factory reset clears password
- **WHEN** a factory reset is performed
- **THEN** the password hash SHALL be cleared
- **AND** the device SHALL return to the first-time setup state
