## ADDED Requirements

### Requirement: Toast Notification System
The Settings page SHALL display save feedback using toast notifications that are visible regardless of scroll position.

#### Scenario: Success toast on save
- **WHEN** settings are saved successfully
- **THEN** a green toast notification SHALL appear at the bottom of the viewport
- **AND** the toast SHALL display the success message
- **AND** the toast SHALL auto-dismiss after 4 seconds

#### Scenario: Error toast on save failure
- **WHEN** settings fail to save
- **THEN** a red toast notification SHALL appear at the bottom of the viewport
- **AND** the toast SHALL display the error message
- **AND** the toast SHALL remain visible until manually dismissed

#### Scenario: Toast visibility
- **WHEN** a toast notification is displayed
- **THEN** it SHALL be visible regardless of the current scroll position
- **AND** it SHALL not obstruct critical page content
- **AND** it SHALL include a close button for manual dismissal

#### Scenario: Toast on test notification
- **WHEN** a test notification is sent
- **THEN** the result SHALL be displayed as a toast notification
- **AND** success SHALL show as green, failure as red
