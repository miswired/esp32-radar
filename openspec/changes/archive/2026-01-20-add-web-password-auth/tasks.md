# Implementation Tasks

## Add Web Password Authentication

**Status:** Completed
**Goal:** Secure web interface with password authentication while keeping API key for external access

## 1. Configuration Storage

- [x] 1.1 Add password hash field to Config struct (64 chars for SHA-256 hex)
- [x] 1.2 Add `passwordConfigured` flag to track if password has been set
- [x] 1.3 Update `loadConfig()` to read password hash from NVS
- [x] 1.4 Update `saveConfig()` to persist password hash to NVS
- [x] 1.5 Update `setDefaultConfig()` to clear password hash
- [x] 1.6 Update factory reset to clear password

## 2. Password Hashing

- [x] 2.1 Include mbedtls SHA-256 library
- [x] 2.2 Implement `hashPassword(plaintext)` function returning hex string
- [x] 2.3 Implement `verifyPassword(plaintext, hash)` function

## 3. Session Management

- [x] 3.1 Define session struct (token, expiration timestamp)
- [x] 3.2 Implement `generateSessionToken()` using ESP32 hardware RNG
- [x] 3.3 Implement `createSession()` - clears any existing session, creates new one
- [x] 3.4 Implement `validateSession(token)` - check token and expiration
- [x] 3.5 Implement `clearSession()` - logout functionality
- [x] 3.6 Add session expiration check (1 hour timeout)
- [x] 3.7 Set session cookie with `SameSite=Strict` and no `Max-Age` (browser session)

## 4. Re-add API Key Authentication to POST Endpoints

- [x] 4.1 Add auth check back to `handlePostConfig()`
- [x] 4.2 Add auth check back to `handleTestNotification()`
- [x] 4.3 Add auth check back to `handleReset()`
- [x] 4.4 Update comments to explain dual auth model

## 5. Login/Password Setup Modals

- [x] 5.1 Create CSS for modal overlay (dark backdrop, centered card)
- [x] 5.2 Create "Create Password" modal HTML:
  - Password field with show/hide toggle
  - Confirm password field
  - Requirements text (min 6 chars, suggest special chars)
  - HTTP warning message
  - Create button
- [x] 5.3 Create "Login" modal HTML:
  - Password field with show/hide toggle
  - HTTP warning message
  - Login button
- [x] 5.4 Add JavaScript for password validation (min 6 chars, match confirm)
- [x] 5.5 Add JavaScript for modal show/hide logic
- [x] 5.6 Add JavaScript for password setup form submission
- [x] 5.7 Add JavaScript for login form submission

## 6. Web Endpoints for Auth

- [x] 6.1 Add `POST /auth/setup` - Create initial password
  - Accepts: password, confirmPassword
  - Returns: success/error, sets session cookie on success
- [x] 6.2 Add `POST /auth/login` - Authenticate with password
  - Accepts: password
  - Returns: success/error, sets session cookie on success
- [x] 6.3 Add `POST /auth/logout` - Clear session
  - Clears session cookie
- [x] 6.4 Add `GET /auth/status` - Check auth state
  - Returns: passwordConfigured, loggedIn

## 7. Settings Page Protection

- [x] 7.1 Modify Settings page to check session on load
- [x] 7.2 If no password configured, show setup modal
- [x] 7.3 If password configured but not logged in, show login modal
- [x] 7.4 If logged in, show settings form normally
- [x] 7.5 Block form submission if not authenticated

## 8. Navigation Bar Updates

- [x] 8.1 Add Login/Logout button to nav bar in Dashboard page
- [x] 8.2 Add Login/Logout button to nav bar in Settings page
- [x] 8.3 Add Login/Logout button to nav bar in Diagnostics page
- [x] 8.4 Add Login/Logout button to nav bar in API docs page
- [x] 8.5 Add JavaScript to update button text based on auth state
- [x] 8.6 Add JavaScript for logout button click handler

## 9. Serial Password Reset Command

- [x] 9.1 Add 'p' command for password management
- [x] 9.2 Show current password status (configured/not configured)
- [x] 9.3 Sub-command to clear password and return to setup state
- [x] 9.4 Update help menu with new command

## 10. Documentation

- [x] 10.1 Update README.md with web password documentation
- [x] 10.2 Document password requirements
- [x] 10.3 Document serial reset procedure
- [x] 10.4 Update API docs page with auth information

## 11. Testing

- [x] 11.1 Test first-time password setup flow
- [x] 11.2 Test login flow
- [x] 11.3 Test logout flow
- [x] 11.4 Test session timeout (1 hour)
- [x] 11.5 Test session ends on browser close
- [x] 11.6 Test single session enforcement (new login kicks out old)
- [x] 11.7 Test password requirements validation
- [x] 11.8 Test serial password reset
- [x] 11.9 Test API key still required for POST endpoints
- [x] 11.10 Test Settings page blocked without login
- [x] 11.11 Test Dashboard/Diagnostics accessible without login
- [x] 11.12 Test AP mode still requires password

## 12. Compile and Deploy

- [x] 12.1 Compile with no errors
- [x] 12.2 Verify memory usage acceptable
- [x] 12.3 Upload to ESP32
- [x] 12.4 Full integration test
