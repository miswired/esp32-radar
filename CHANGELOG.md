# Changelog

All notable changes to this project are documented here.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased] - Project Reorganization

### Changed
- Moved all development stages to `archive/stages/` for cleaner project structure
- Created `firmware/` folder with latest production firmware
- Renamed main sketch to `firmware.ino` for Arduino compatibility

### Added
- `archive/stages/README.md` documenting all development stages

## [Stage 9] - Home Assistant MQTT Integration

### Added
- MQTT client using PubSubClient library
- Home Assistant MQTT Discovery for automatic device registration
- Motion binary sensor entity (device_class: motion)
- Diagnostic sensor entities (WiFi signal, uptime, heap, alarm/motion counts, filter level, IP)
- Number control entities for remote tuning (trip delay, clear timeout, filter threshold)
- MQTT availability tracking via Last Will and Testament (LWT)
- Support for authenticated MQTT connections (username/password)
- Optional TLS encryption for MQTT (port 8883)
- Configurable device name for Home Assistant
- MQTT settings section in web interface
- About page with license and project information
- GPL v3 license

### Fixed
- Motion binary sensor discovery now uses full topic paths for compatibility

## [Stage 8] - WLED Integration

### Added
- WLED device integration for visual alarm feedback
- Custom JSON payload support for WLED
- WLED URL and payload configuration in settings
- Test WLED button for verifying configuration
- Refactored notification methods to use checkboxes (GET/POST independent)

## [Stage 7] - API Authentication

### Added
- API key authentication for JSON endpoints
- Session-based web authentication with password
- Password hashing using SHA-256
- API key generation endpoint
- Login/logout functionality
- Rate limiting for failed authentication attempts

## [Stage 6] - Full System Integration

### Added
- Web-based Diagnostics page
- Real-time heap memory monitoring with history graph
- Event log display in web interface
- System health indicators
- Comprehensive status dashboard

## [Stage 5] - Adaptive Filter

### Added
- Adaptive motion filter to reduce false positives
- Moving average algorithm for motion detection
- Configurable filter threshold percentage
- Filter window timing configuration
- Motion event counting

## [Stage 4] - HTTP Notifications

### Added
- HTTP webhook notifications on alarm events
- Configurable notification URL
- GET and POST request methods
- Test notification button
- Notification success/failure tracking

## [Stage 3] - Configuration Persistence

### Added
- NVS (Non-Volatile Storage) for settings
- Configuration survives power cycles
- Trip delay and clear timeout settings
- Factory reset functionality
- Settings page in web interface

## [Stage 2] - WiFi and Web Interface

### Added
- WiFi Station mode (connects to existing network)
- WiFi Access Point mode (creates setup network)
- Web-based dashboard showing motion status
- REST API for status queries
- Real-time motion state display

## [Stage 1] - Basic Motion Detection

### Added
- RCWL-0516 radar sensor integration
- GPIO-based motion detection
- LED indicator for motion status
- Serial monitor output for debugging
- Basic state machine (IDLE, MOTION_DETECTED, ALARM_ACTIVE, ALARM_CLEARING)
