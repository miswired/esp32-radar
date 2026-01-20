# Change: Add Web Password Authentication

**Status:** Completed (2026-01-20)

## Why

The current system has API key authentication for securing API endpoints, but the web interface has no access control. Anyone on the local network can access the Settings page and modify device configuration. Users need the ability to protect the web interface with a password while keeping the API key system separate for external integrations.

## What Changes

### Web Password Authentication System

- **First-time setup**: On first visit to Settings page, prompt user to create a password via modal overlay
- **Login required**: Settings page requires password authentication; Dashboard and Diagnostics remain public
- **Session management**: Cookie-based sessions with 1 hour timeout, single session only (new login kicks out previous)
- **Password storage**: SHA-256 hash stored in NVS (cannot be recovered, only reset)
- **Password requirements**: Minimum 6 characters, UI suggests special characters and numbers
- **Logout mechanism**: Login/Logout button visible in navigation bar on all pages
- **HTTPS warning**: Display warning on login/setup pages about unencrypted HTTP connection
- **AP mode**: Password still required even when device is in AP fallback mode
- **Serial reset**: New serial command to clear password and return to first-time setup state

### API Key Authentication (Restored)

- Re-add API key requirement to POST endpoints when API auth is enabled
- API key and web password are completely separate credentials
- API key for external integrations (curl, scripts, Home Assistant)
- Web password for browser-based administration

### Protected Endpoints (when API auth enabled)

- `POST /config`
- `POST /test-notification`
- `POST /reset`

### Login Flow

1. User visits Settings page
2. If no password configured: Show "Create Password" modal
3. If password configured but not logged in: Show "Login" modal
4. If logged in: Show Settings page normally
5. Session expires after 1 hour or browser close

## Impact

### Affected Specs
- **MODIFIED**: `web-interface` - Add password authentication, login modal, nav bar login button
- **MODIFIED**: `api-authentication` - Re-add POST endpoint protection
- **MODIFIED**: `configuration` - Add password hash storage

### Affected Code
- `stage7_api_auth/stage7_api_auth.ino`:
  - Add password hash to Config struct and NVS
  - Add session management (token generation, validation, expiration)
  - Add login modal HTML/CSS/JS
  - Add password setup modal HTML/CSS/JS
  - Add login/logout button to all page nav bars
  - Re-add auth checks to POST handlers
  - Add serial command for password reset
  - Add SHA-256 hashing using mbedtls (built into ESP32)
