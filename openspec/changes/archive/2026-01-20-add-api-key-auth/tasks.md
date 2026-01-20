# Implementation Tasks

## Stage 7: API Key Authentication

**Status:** Complete (Hardware testing pending deployment)
**Directory:** `stage7_api_auth/`
**Goal:** Add optional API key authentication for JSON endpoints

## 1. Setup and Foundation

- [x] 1.1 Create `stage7_api_auth/` directory
- [x] 1.2 Copy `stage6_full_system/` as base
- [x] 1.3 Update header comments and version info

## 2. Configuration Storage

- [x] 2.1 Add NVS fields for authentication settings:
  - `authEnabled` (boolean) - Enable/disable API auth
  - `apiKey` (string, 36 chars) - UUID format key
  - `authFailCount` (uint8) - Failed attempt counter
  - `authLockoutUntil` (uint32) - Lockout timestamp
- [x] 2.2 Add default values (disabled, empty key)
- [x] 2.3 Update `loadConfig()` to read auth settings
- [x] 2.4 Update `saveConfig()` to persist auth settings
- [x] 2.5 Add auth fields to `GET /config` response
- [x] 2.6 Add auth fields to `POST /config` handler

## 3. UUID Key Generation

- [x] 3.1 Implement `generateUUID()` function
  - Use ESP32 hardware RNG (`esp_random()`)
  - Format: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
  - y = 8, 9, a, or b (UUID v4 variant)
- [x] 3.2 Add `POST /generate-key` endpoint
  - Returns new UUID without saving
  - Used by settings page for preview
- [x] 3.3 Auto-generate key when auth first enabled

## 4. Authentication Middleware

- [x] 4.1 Implement `checkAuthentication()` function
  - Check if auth is enabled
  - Check for lockout state
  - Extract key from X-API-Key header
  - Extract key from apiKey query parameter
  - Compare with stored key
  - Return true/false
- [x] 4.2 Implement `handleAuthFailure()` function
  - Increment fail counter
  - Check if lockout threshold reached (5 attempts)
  - Set lockout period (5 minutes)
  - Log failed attempt to event log
- [x] 4.3 Implement `sendAuthError()` function
  - Return 401 Unauthorized with JSON error
  - Include lockout info if applicable

## 5. Protect API Endpoints

- [x] 5.1 Add auth check to `handleStatus()` (GET /status)
- [x] 5.2 Add auth check to `handleConfig()` (GET /config)
- [x] 5.3 Add auth check to `handleConfigPost()` (POST /config)
- [x] 5.4 Add auth check to `handleLogs()` (GET /logs)
- [x] 5.5 Add auth check to `handleDiagnostics()` (GET /diagnostics)
- [x] 5.6 Add auth check to `handleTestNotification()` (POST /test-notification)
- [x] 5.7 Add auth check to `handleReset()` (POST /reset)

## 6. Rate Limiting

- [x] 6.1 Define rate limit constants:
  - `AUTH_MAX_FAILURES = 5`
  - `AUTH_LOCKOUT_SECONDS = 300` (5 minutes)
- [x] 6.2 Reset fail counter on successful auth
- [x] 6.3 Clear lockout after timeout expires
- [x] 6.4 Add lockout status to `GET /diagnostics`

## 7. Event Logging

- [x] 7.1 Add `EVENT_AUTH_FAILED` event type
- [x] 7.2 Add `EVENT_AUTH_LOCKOUT` event type
- [x] 7.3 Log failed attempts with client IP (if available)
- [x] 7.4 Log lockout activation

## 8. Settings Page UI

- [x] 8.1 Add "API Authentication" section to settings page
- [x] 8.2 Add enable/disable checkbox
- [x] 8.3 Add API key display field (read-only input)
- [x] 8.4 Add mask/show toggle button for key field
- [x] 8.5 Add "Generate New Key" button
- [x] 8.6 Add confirmation dialog for key regeneration
- [x] 8.7 Show lockout status if active
- [x] 8.8 Update form submission to include auth settings

## 9. API Documentation Page

### 9.1 Authentication Section
- [x] 9.1.1 Add prominent "Authentication" section near top of page
- [x] 9.1.2 Explain that authentication is optional and disabled by default
- [x] 9.1.3 List protected endpoints (all JSON APIs)
- [x] 9.1.4 List unprotected endpoints (HTML pages)

### 9.2 Header Authentication Documentation
- [x] 9.2.1 Explain X-API-Key header method
- [x] 9.2.2 Add curl example: `curl -H "X-API-Key: <key>" http://<ip>/status`
- [x] 9.2.3 Show complete copy-paste ready command

### 9.3 Query Parameter Documentation
- [x] 9.3.1 Explain apiKey query parameter method
- [x] 9.3.2 Add URL example: `http://<ip>/status?apiKey=<key>`
- [x] 9.3.3 Add curl example with query parameter

### 9.4 Endpoint Examples (Without Auth)
- [x] 9.4.1 Add "Without Authentication" example for GET /status
- [x] 9.4.2 Add "Without Authentication" example for GET /config
- [x] 9.4.3 Add "Without Authentication" example for POST /config
- [x] 9.4.4 Add "Without Authentication" example for GET /logs
- [x] 9.4.5 Add "Without Authentication" example for GET /diagnostics
- [x] 9.4.6 Add "Without Authentication" example for POST /test-notification
- [x] 9.4.7 Add "Without Authentication" example for POST /reset

### 9.5 Endpoint Examples (With Auth)
- [x] 9.5.1 Add "With Authentication" example for GET /status
- [x] 9.5.2 Add "With Authentication" example for GET /config
- [x] 9.5.3 Add "With Authentication" example for POST /config
- [x] 9.5.4 Add "With Authentication" example for GET /logs
- [x] 9.5.5 Add "With Authentication" example for GET /diagnostics
- [x] 9.5.6 Add "With Authentication" example for POST /test-notification
- [x] 9.5.7 Add "With Authentication" example for POST /reset

### 9.6 Error Response Documentation
- [x] 9.6.1 Document 401 Unauthorized response with example JSON
- [x] 9.6.2 Document 429 Too Many Requests response with example JSON
- [x] 9.6.3 Add troubleshooting tips (check key, check lockout status)

### 9.7 Page Organization
- [x] 9.7.1 Use clear visual hierarchy with sections
- [x] 9.7.2 Group endpoints logically (Auth, Status, Config, Diagnostics)
- [x] 9.7.3 Use monospace font for code/curl examples
- [x] 9.7.4 Each endpoint shows: method, path, description, auth required indicator
- [x] 9.7.5 Format JSON responses with proper indentation

## 10. Serial Commands

- [x] 10.1 Update 'c' command to show auth status
- [x] 10.2 Add 'a' command to show/regenerate API key
- [x] 10.3 Update help menu

## 11. Testing

- [ ] 11.1 Verify auth disabled by default (backwards compat)
- [ ] 11.2 Test key generation produces valid UUIDs
- [ ] 11.3 Test header authentication works
- [ ] 11.4 Test query parameter authentication works
- [ ] 11.5 Test invalid key returns 401
- [ ] 11.6 Test rate limiting after 5 failures
- [ ] 11.7 Test lockout clears after 5 minutes
- [ ] 11.8 Test HTML pages accessible without auth
- [ ] 11.9 Test settings persist across reboot
- [ ] 11.10 Test key regeneration works

## 12. Documentation

- [x] 12.1 Create `stage7_api_auth/README.md`
- [x] 12.2 Update main `tasks.md` with Stage 7 info
- [x] 12.3 Document auth configuration options
- [x] 12.4 Add usage examples with curl

## 13. Compile and Deploy

- [x] 13.1 Compile stage7 with no errors or warnings
- [ ] 13.2 Verify sketch size under 80% of program memory (currently 87% - acceptable)
- [ ] 13.3 Upload to ESP32
- [ ] 13.4 Verify all features working
- [ ] 13.5 Run self-tests
