# web-interface Specification Delta

## ADDED Requirements

### Requirement: About Page
The web interface SHALL include an About page displaying project and license information.

#### Scenario: About page accessible
- **GIVEN** the user is on any page of the web interface
- **WHEN** they click the "About" navigation link
- **THEN** the About page SHALL be displayed

#### Scenario: About page displays project information
- **GIVEN** the user is on the About page
- **THEN** the project name "ESP32 Microwave Motion Sensor" SHALL be displayed
- **AND** the current firmware version SHALL be displayed
- **AND** the author credit "Created by Miswired" SHALL be displayed

#### Scenario: About page displays license information
- **GIVEN** the user is on the About page
- **THEN** the license type "GNU General Public License v3.0" SHALL be displayed
- **AND** the copyright notice "Copyright 2024-2026 Miswired" SHALL be displayed
- **AND** a brief description of GPL v3 rights and obligations SHALL be provided
- **AND** a link to the full license text SHALL be provided

#### Scenario: About page displays documentation link
- **GIVEN** the user is on the About page
- **THEN** a link to the project documentation (GitHub Wiki) SHALL be provided
- **AND** a link to the project repository SHALL be provided

#### Scenario: About page displays device information
- **GIVEN** the user is on the About page
- **THEN** the device's chip ID SHALL be displayed
- **AND** the device's IP address SHALL be displayed
- **AND** the current uptime SHALL be displayed
