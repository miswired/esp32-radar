# Change: Fix Settings Page Authentication and Improve Save Feedback UX

## Why

When API key authentication is enabled, the Settings page cannot save changes because `POST /config` requires an API key that the browser doesn't have access to. This creates a situation where users can enable authentication but then cannot disable it or change any settings via the web UI.

Additionally, the save status message appears at the top of the page, which is often out of view when the user clicks the Save button at the bottom. Users have to scroll up to see if their changes were saved successfully.

## What Changes

### Bug Fix: Web UI POST Endpoints
Remove API key authentication requirement from all POST endpoints. The rationale:
- API key authentication is intended for external API access (curl, scripts, integrations)
- The web UI is for local administration and will have separate password protection added later
- This allows the web interface to function fully regardless of API key settings

**POST endpoints to unprotect:**
- `POST /config` - Save settings
- `POST /test-notification` - Test notifications
- `POST /reset` - Factory reset

### UX Improvement: Save Feedback
Replace the top-of-page status message with a toast notification system:
- Toast appears near bottom of viewport, always visible
- Auto-dismisses after a few seconds for success messages
- Stays visible for errors until dismissed
- Uses color coding (green for success, red for error)

## Impact

### Affected Specs
- **MODIFIED**: `api-authentication` - Remove POST endpoints from protected list
- **MODIFIED**: `web-interface` - Add toast notification for save feedback

### Affected Code
- `stage7_api_auth/stage7_api_auth.ino`:
  - Remove auth checks from `handlePostConfig()`, `handleTestNotification()`, `handleReset()`
  - Update Settings page HTML/JS to use toast notifications
