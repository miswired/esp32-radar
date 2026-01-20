## ADDED Requirements

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
