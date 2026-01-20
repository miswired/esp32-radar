# Change: Add API Key Authentication for JSON Endpoints

## Why

The ESP32 radar motion sensor exposes REST API endpoints that could be accessed by unauthorized users on the network. Without authentication, anyone with network access could:
- Read sensor status and configuration
- Modify device settings
- Trigger test notifications
- Factory reset the device

Users deploying this device on shared or semi-public networks need the ability to secure API access to prevent unauthorized control and potential abuse.

## What Changes

This change adds an optional API key authentication system with the following capabilities:

- **UUID API Keys**: Generate cryptographically random UUID-format keys for authentication
- **Dual Transmission Methods**: Support both HTTP Header (`X-API-Key`) and Query Parameter (`?apiKey=xxx`) for flexibility
- **Selective Protection**: Protect all JSON API endpoints while leaving HTML pages accessible
- **Rate Limiting**: Implement lockout after repeated failed authentication attempts
- **Key Management UI**: Settings page controls for enable/disable, key generation, and mask/hide toggle
- **Security Logging**: Log failed authentication attempts to the event log
- **Persistent Storage**: Store key and authentication settings in NVS

### Protected Endpoints
- `GET /status` - Sensor status
- `GET /config` - Configuration
- `POST /config` - Save configuration
- `GET /logs` - Event log
- `GET /diagnostics` - System diagnostics
- `POST /test-notification` - Test notification
- `POST /reset` - Factory reset

### Unprotected (HTML Pages)
- `GET /` - Dashboard
- `GET /settings` - Settings page
- `GET /diag` - Diagnostics page
- `GET /api` - API documentation

### Default Behavior
- Authentication is disabled by default (backwards compatible)
- When first enabled, a UUID key is auto-generated
- Users can regenerate keys at any time via the settings page

## Impact

### Affected Capabilities
- **NEW**: `api-authentication` - API key authentication system
- **MODIFIED**: `web-interface` - Settings page additions for auth config
- **MODIFIED**: `configuration` - New persistent auth parameters
- **MODIFIED**: `diagnostics` - New event types for auth failures

### Affected Code
- New stage directory: `stage7_api_auth/`
- Modified web handlers with authentication middleware
- New NVS fields for auth configuration
- New event types in logging system

### Dependencies
- No new external libraries required
- Uses built-in ESP32 random number generation for UUID

### Testing Requirements
- Verify endpoints reject requests without valid key when auth enabled
- Verify both header and query parameter authentication work
- Verify rate limiting activates after failed attempts
- Verify key generation produces valid UUIDs
- Verify settings persist across reboots
- Verify HTML pages remain accessible without authentication
