# Change: Add WLED Integration

## Why

Users want visual feedback when motion alarms trigger. WLED is a popular open-source firmware for ESP8266/ESP32-based LED controllers that provides a JSON API for controlling LED strips. Adding WLED integration allows the motion sensor to trigger visual alerts (e.g., flash red) on WLED devices when an alarm activates.

This feature is separate from the existing notification system, allowing users to have both HTTP notifications (to Home Assistant, webhooks, etc.) AND visual WLED alerts simultaneously.

Additionally, the current notification method selection (GET/POST dropdown) should be changed to checkboxes for consistency with the WLED checkbox pattern and to allow sending both GET and POST requests simultaneously.

## What Changes

### WLED Integration Feature

- **Enable/Disable toggle**: Checkbox in Settings to enable WLED integration
- **URL configuration**: Full URL field for WLED device endpoint (e.g., `http://192.168.1.50/json/state`)
- **Custom JSON payload**: Textarea for user-defined JSON payload sent to WLED
- **Default payload**: Pre-populated example that sets WLED to solid red at full brightness
- **Test button**: "Test WLED" button to verify configuration without triggering motion
- **JSON validation**: Validate payload syntax before saving to prevent silent failures

### Notification Method UI Refactor

- **Replace dropdown with checkboxes**: Change from single-select dropdown (GET or POST) to multi-select checkboxes (GET and/or POST)
- **Independent selection**: Users can enable GET only, POST only, both, or neither
- **Consistent UI pattern**: Matches the checkbox pattern used for WLED and future notification options
- **Backwards compatible**: Migrate existing `notifyMethod` (0=GET, 1=POST) to new checkbox fields

### Trigger Behavior

- Payload sent only when alarm triggers (after trip delay, when entering ALARM_ACTIVE state)
- Fire-and-forget: no state restoration when alarm clears
- Silent failure: log errors to event system but don't block alarm processing
- Single WLED device support (no multi-device configuration)

### Technical Implementation

- HTTP POST request to configured URL with JSON payload
- Content-Type: application/json header
- 3-second timeout to avoid blocking main loop
- Maximum payload size: 512 characters
- Reuse existing HTTPClient pattern from notification system

## Impact

### Affected Specs
- **MODIFIED**: `configuration` - Add WLED settings storage, change notification method storage
- **MODIFIED**: `web-interface` - Add WLED settings UI section, refactor notification method UI
- **MODIFIED**: `diagnostics` - Add WLED event logging

### Affected Code
- `stage7_api_auth/stage7_api_auth.ino`:
  - Add WLED fields to Config struct
  - Change `notifyMethod` from uint8_t to two booleans (`notifyGet`, `notifyPost`)
  - Update `sendNotification()` to check both flags and send multiple requests if both enabled
  - Add `sendWledPayload()` function
  - Add WLED UI section in SETTINGS_HTML
  - Replace notification method dropdown with checkboxes in SETTINGS_HTML
  - Add `/test-wled` endpoint
  - Add WLED_SENT and WLED_FAILED event types
  - Call `sendWledPayload()` when alarm triggers
  - Add JSON validation helper function
