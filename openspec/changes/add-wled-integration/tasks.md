# Implementation Tasks

## Add WLED Integration

**Status:** Pending
**Goal:** Send custom JSON payloads to WLED devices when motion alarms trigger, and refactor notification methods to use checkboxes

## 1. Notification Method Refactor - Configuration

- [ ] 1.1 Replace `notifyMethod` (uint8_t) with two booleans in Config struct:
  - `notifyGet` (bool) - Send GET request on alarm
  - `notifyPost` (bool) - Send POST request on alarm
- [ ] 1.2 Update `setDefaultConfig()` to set both to false
- [ ] 1.3 Update `loadConfig()` to read new fields from NVS
- [ ] 1.4 Update `saveConfig()` to persist new fields to NVS
- [ ] 1.5 Handle migration: if old `notifyMethod` exists, convert to new format

## 2. Notification Method Refactor - Send Function

- [ ] 2.1 Update `sendNotification()` to check `notifyGet` flag
- [ ] 2.2 Update `sendNotification()` to check `notifyPost` flag
- [ ] 2.3 If both flags are true, send both GET and POST requests
- [ ] 2.4 Update logging to indicate which method(s) were sent

## 3. Notification Method Refactor - UI

- [ ] 3.1 Remove the notification method dropdown from SETTINGS_HTML
- [ ] 3.2 Add "Send GET request" checkbox
- [ ] 3.3 Add "Send POST request" checkbox
- [ ] 3.4 Update JavaScript to load/save new checkbox fields
- [ ] 3.5 Update `handleGetConfig()` to return `notifyGet` and `notifyPost`
- [ ] 3.6 Update `handlePostConfig()` to accept `notifyGet` and `notifyPost`

## 4. WLED Configuration Storage

- [ ] 4.1 Add WLED fields to Config struct:
  - `wledEnabled` (bool)
  - `wledUrl` (char[128])
  - `wledPayload` (char[512])
- [ ] 4.2 Update `setDefaultConfig()` with WLED defaults:
  - `wledEnabled = false`
  - `wledUrl = ""`
  - `wledPayload = {"on":true,"bri":255,"seg":[{"col":[[255,0,0]]}]}`
- [ ] 4.3 Update `loadConfig()` to read WLED settings from NVS
- [ ] 4.4 Update `saveConfig()` to persist WLED settings to NVS
- [ ] 4.5 Update factory reset to clear WLED settings

## 5. WLED Send Function

- [ ] 5.1 Add `WLED_SENT` and `WLED_FAILED` event types
- [ ] 5.2 Implement `sendWledPayload()` function:
  - Check if WLED is enabled and URL is configured
  - Check WiFi is connected (station mode)
  - POST to configured URL with configured payload
  - Set Content-Type: application/json header
  - Use 3-second timeout
  - Log success/failure to event system
- [ ] 5.3 Call `sendWledPayload()` when alarm triggers (in state machine ALARM_ACTIVE transition)

## 6. JSON Validation

- [ ] 6.1 Implement `isValidJson(const char* json)` helper function
- [ ] 6.2 Use ArduinoJson to parse and validate payload
- [ ] 6.3 Return validation error message for UI feedback

## 7. WLED Settings Page UI

- [ ] 7.1 Add "WLED Integration" section after "Notifications" section
- [ ] 7.2 Add enable checkbox with label "Send payload to WLED device on alarm"
- [ ] 7.3 Add URL input field:
  - Label: "WLED URL"
  - Placeholder: "http://192.168.1.50/json/state"
  - Full width text input
- [ ] 7.4 Add JSON payload textarea:
  - Label: "JSON Payload"
  - Multi-line textarea (4-6 rows)
  - Monospace font for readability
  - Pre-populated with default payload
- [ ] 7.5 Add "Test WLED" button next to the section
- [ ] 7.6 Add JSON validation error display below textarea
- [ ] 7.7 Style consistently with existing settings sections

## 8. WLED Config API Updates

- [ ] 8.1 Update `handleGetConfig()` to include WLED fields in JSON response
- [ ] 8.2 Update `handlePostConfig()` to accept WLED fields:
  - `wledEnabled` (boolean)
  - `wledUrl` (string)
  - `wledPayload` (string)
- [ ] 8.3 Add JSON validation before saving payload
- [ ] 8.4 Return validation error if payload is invalid JSON

## 9. Test WLED Endpoint

- [ ] 9.1 Add `POST /test-wled` endpoint
- [ ] 9.2 Apply same auth check as other POST endpoints (session or API key)
- [ ] 9.3 Accept optional `url` and `payload` parameters to test unsaved values
- [ ] 9.4 Fall back to saved config if parameters not provided
- [ ] 9.5 Return JSON response with success/failure and HTTP status code

## 10. WLED Settings Page JavaScript

- [ ] 10.1 Add `loadConfig()` handling for WLED fields
- [ ] 10.2 Add form submission handling for WLED fields
- [ ] 10.3 Add JSON validation on input change (client-side feedback)
- [ ] 10.4 Add "Test WLED" button click handler
- [ ] 10.5 Show toast notification for test result

## 11. Event Logging

- [ ] 11.1 Add event type strings for WLED_SENT and WLED_FAILED
- [ ] 11.2 Log HTTP response code on success
- [ ] 11.3 Log error code on failure
- [ ] 11.4 Update diagnostics page event display styling for WLED events

## 12. Documentation

- [ ] 12.1 Update README.md with WLED integration section
- [ ] 12.2 Document URL format and endpoint requirements
- [ ] 12.3 Document default payload and customization examples
- [ ] 12.4 Document notification method checkbox changes
- [ ] 12.5 Add troubleshooting section for common WLED issues

## 13. Testing - Notification Refactor

- [ ] 13.1 Test GET-only notification
- [ ] 13.2 Test POST-only notification
- [ ] 13.3 Test both GET and POST enabled (sends two requests)
- [ ] 13.4 Test neither enabled (no notification sent)
- [ ] 13.5 Test migration from old notifyMethod format

## 14. Testing - WLED

- [ ] 14.1 Test WLED enable/disable toggle
- [ ] 14.2 Test URL and payload persistence across reboots
- [ ] 14.3 Test "Test WLED" button with valid configuration
- [ ] 14.4 Test "Test WLED" button with invalid URL (timeout)
- [ ] 14.5 Test JSON validation rejects invalid payloads
- [ ] 14.6 Test JSON validation accepts valid payloads
- [ ] 14.7 Test WLED triggers on alarm activation
- [ ] 14.8 Test WLED does NOT trigger on alarm clear
- [ ] 14.9 Test factory reset clears WLED settings
- [ ] 14.10 Test auth protection on /test-wled endpoint

## 15. Compile and Deploy

- [ ] 15.1 Compile with no errors
- [ ] 15.2 Verify memory usage acceptable
- [ ] 15.3 Upload to ESP32
- [ ] 15.4 Full integration test with actual WLED device
