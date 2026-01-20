# diagnostics Specification

## Purpose
TBD - created by archiving change add-api-key-auth. Update Purpose after archive.
## Requirements
### Requirement: Authentication Event Logging
The system SHALL log authentication-related events to the event log.

#### Scenario: Failed authentication logged
- **WHEN** an authentication attempt fails
- **THEN** the system SHALL log an `AUTH_FAILED` event
- **AND** the event data SHALL include information to identify the attempt

#### Scenario: Lockout activation logged
- **WHEN** rate limiting triggers a lockout
- **THEN** the system SHALL log an `AUTH_LOCKOUT` event
- **AND** the event data SHALL include the lockout duration

### Requirement: Authentication Diagnostics
The diagnostics endpoint SHALL include authentication status information.

#### Scenario: Auth status in diagnostics
- **WHEN** a GET request is made to /diagnostics
- **THEN** the response SHALL include `authEnabled` (boolean)
- **AND** if in lockout, SHALL include `authLockoutRemaining` (seconds)
- **AND** SHALL include `authFailedAttempts` (current counter)

