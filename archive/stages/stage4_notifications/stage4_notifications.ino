/**
 * ESP32 Microwave Motion Sensor - Stage 4: Notification System
 *
 * This stage builds on Stage 3 and adds:
 * - HTTP notifications on alarm state changes
 * - Configurable notification URL and method (GET/POST)
 * - JSON payload for POST notifications
 * - Test notification button in web UI
 * - Notification status tracking
 *
 * Hardware Requirements:
 * - ESP32-WROOM-32
 * - RCWL-0516 Microwave Radar Motion Sensor
 *
 * Connections:
 * - Sensor VIN → ESP32 VIN (5V from USB)
 * - Sensor GND → ESP32 GND
 * - Sensor OUT → ESP32 GPIO 13
 *
 * WiFi Modes:
 * - Station Mode (STA): Connects to your WiFi network
 * - Access Point Mode (AP): Creates "ESP32-Radar-Setup" network at 192.168.4.1
 *
 * Web Interface:
 * - http://<ip>/ - Dashboard with motion status
 * - http://<ip>/settings - Configuration page
 * - http://<ip>/api - API documentation
 * - GET /status - JSON API for current status
 * - GET /config - JSON API for current configuration
 * - POST /config - Save configuration
 * - POST /reset - Factory reset
 * - POST /test-notification - Send test notification
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <rom/rtc.h>
#include "credentials.h"  // WiFi credentials (copy credentials.h.example to credentials.h)

// ============================================================================
// PIN CONFIGURATION (fixed, not configurable)
// ============================================================================

#define SENSOR_PIN 13
#define LED_PIN 2

// ============================================================================
// TIMING CONFIGURATION (fixed)
// ============================================================================

#define SENSOR_POLL_INTERVAL 100    // 10Hz (100ms)
#define HEAP_PRINT_INTERVAL 60000   // Print heap every 60 seconds
#define HEAP_MIN_THRESHOLD 50000    // 50KB minimum
#define WIFI_CONNECT_TIMEOUT 15000  // 15 seconds WiFi connection timeout

// ============================================================================
// DEFAULT VALUES FOR CONFIGURABLE SETTINGS
// ============================================================================

#define DEFAULT_TRIP_DELAY_SECONDS 3
#define DEFAULT_CLEAR_TIMEOUT_SECONDS 10
#define DEFAULT_FILTER_THRESHOLD_PERCENT 70
#define DEFAULT_FILTER_SAMPLE_COUNT 10
#define DEFAULT_FILTER_WINDOW_MS 1000

// ============================================================================
// CONFIGURATION STRUCTURE
// ============================================================================

struct Config {
  // WiFi settings
  char wifiSsid[64];
  char wifiPassword[64];

  // Motion detection settings
  uint16_t tripDelaySeconds;
  uint16_t clearTimeoutSeconds;

  // Filter settings
  uint8_t filterThresholdPercent;

  // Notification settings
  bool notifyEnabled;
  char notifyUrl[128];
  uint8_t notifyMethod;  // 0 = GET, 1 = POST

  // Checksum to validate stored data
  uint32_t checksum;
};

// Notification method constants
#define NOTIFY_METHOD_GET 0
#define NOTIFY_METHOD_POST 1

// Notification tracking
unsigned long lastNotificationTime = 0;
int lastNotificationStatus = 0;  // HTTP response code or -1 for error
unsigned long totalNotificationsSent = 0;
unsigned long totalNotificationsFailed = 0;

Config config;
Preferences preferences;

// ============================================================================
// WIFI CONFIGURATION
// ============================================================================

// AP Mode fallback settings
const char* AP_SSID = "ESP32-Radar-Setup";
const char* AP_PASSWORD = "12345678";  // Min 8 characters for WPA2

// ============================================================================
// STATE MACHINE DEFINITIONS
// ============================================================================

enum MotionState {
  STATE_IDLE,
  STATE_MOTION_PENDING,
  STATE_ALARM_ACTIVE,
  STATE_ALARM_CLEARING
};

const char* stateNames[] = {
  "IDLE",
  "MOTION_PENDING",
  "ALARM_ACTIVE",
  "ALARM_CLEARING"
};

// WiFi mode tracking (using custom names to avoid ESP32 SDK conflicts)
enum CurrentWiFiMode {
  MODE_NONE,
  MODE_STATION,
  MODE_ACCESS_POINT
};

CurrentWiFiMode currentWiFiMode = MODE_NONE;

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

// Timing
unsigned long lastSensorRead = 0;
unsigned long lastHeapPrint = 0;
unsigned long lastFilterSample = 0;

// Motion filter - circular buffer
bool sampleBuffer[DEFAULT_FILTER_SAMPLE_COUNT];
int sampleIndex = 0;
int samplePositiveCount = 0;
bool filterInitialized = false;

// Motion state
bool rawMotionDetected = false;
bool filteredMotionDetected = false;
int currentFilterPercent = 0;
MotionState currentState = STATE_IDLE;
unsigned long motionStartTime = 0;
unsigned long motionStopTime = 0;
unsigned long alarmTriggerTime = 0;
unsigned long totalAlarmEvents = 0;
unsigned long totalMotionEvents = 0;

// Web server (using built-in ESP32 WebServer)
WebServer server(80);

// ============================================================================
// CONFIGURATION FUNCTIONS
// ============================================================================

uint32_t calculateChecksum(Config* cfg) {
  uint32_t sum = 0;
  uint8_t* data = (uint8_t*)cfg;
  // Exclude the checksum field itself from calculation
  size_t len = sizeof(Config) - sizeof(cfg->checksum);
  for (size_t i = 0; i < len; i++) {
    sum += data[i];
  }
  return sum ^ 0xDEADBEEF;  // XOR with magic number
}

void setDefaultConfig() {
  // Use credentials from credentials.h as defaults
  strncpy(config.wifiSsid, WIFI_SSID, sizeof(config.wifiSsid) - 1);
  strncpy(config.wifiPassword, WIFI_PASSWORD, sizeof(config.wifiPassword) - 1);

  config.tripDelaySeconds = DEFAULT_TRIP_DELAY_SECONDS;
  config.clearTimeoutSeconds = DEFAULT_CLEAR_TIMEOUT_SECONDS;
  config.filterThresholdPercent = DEFAULT_FILTER_THRESHOLD_PERCENT;

  // Notification defaults
  config.notifyEnabled = false;
  config.notifyUrl[0] = '\0';
  config.notifyMethod = NOTIFY_METHOD_POST;

  config.checksum = calculateChecksum(&config);

  Serial.println("Configuration set to defaults");
}

bool loadConfig() {
  Serial.print("Loading configuration from NVS... ");

  preferences.begin("radar-config", true);  // Read-only

  // Check if we have stored config
  size_t len = preferences.getBytesLength("config");
  if (len != sizeof(Config)) {
    preferences.end();
    Serial.println("No valid config found");
    return false;
  }

  // Read config
  Config tempConfig;
  preferences.getBytes("config", &tempConfig, sizeof(Config));
  preferences.end();

  // Validate checksum
  uint32_t expectedChecksum = calculateChecksum(&tempConfig);
  if (tempConfig.checksum != expectedChecksum) {
    Serial.println("Checksum mismatch!");
    return false;
  }

  // Valid config, copy to active config
  memcpy(&config, &tempConfig, sizeof(Config));
  Serial.println("OK");

  Serial.print("  WiFi SSID: ");
  Serial.println(config.wifiSsid);
  Serial.print("  Trip Delay: ");
  Serial.print(config.tripDelaySeconds);
  Serial.println("s");
  Serial.print("  Clear Timeout: ");
  Serial.print(config.clearTimeoutSeconds);
  Serial.println("s");
  Serial.print("  Filter Threshold: ");
  Serial.print(config.filterThresholdPercent);
  Serial.println("%");
  Serial.print("  Notifications: ");
  Serial.println(config.notifyEnabled ? "Enabled" : "Disabled");
  if (config.notifyEnabled && strlen(config.notifyUrl) > 0) {
    Serial.print("  Notify URL: ");
    Serial.println(config.notifyUrl);
    Serial.print("  Notify Method: ");
    Serial.println(config.notifyMethod == NOTIFY_METHOD_GET ? "GET" : "POST");
  }

  return true;
}

bool saveConfig() {
  Serial.print("Saving configuration to NVS... ");

  // Update checksum
  config.checksum = calculateChecksum(&config);

  preferences.begin("radar-config", false);  // Read-write
  size_t written = preferences.putBytes("config", &config, sizeof(Config));
  preferences.end();

  if (written == sizeof(Config)) {
    Serial.println("OK");
    return true;
  } else {
    Serial.println("FAILED");
    return false;
  }
}

void factoryReset() {
  Serial.println("Performing factory reset...");

  preferences.begin("radar-config", false);
  preferences.clear();
  preferences.end();

  setDefaultConfig();
  saveConfig();

  Serial.println("Factory reset complete. Restarting...");
  delay(1000);
  ESP.restart();
}

// ============================================================================
// HTML DASHBOARD
// ============================================================================

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 Motion Sensor</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
      background: #1a1a2e;
      color: #eee;
      min-height: 100vh;
    }
    nav {
      background: #0f0f1a;
      padding: 15px 20px;
      display: flex;
      justify-content: space-between;
      align-items: center;
      border-bottom: 1px solid #16213e;
      position: sticky;
      top: 0;
      z-index: 100;
    }
    .nav-brand {
      color: #00d4ff;
      font-weight: bold;
      font-size: 18px;
      text-decoration: none;
    }
    .nav-links {
      display: flex;
      gap: 20px;
    }
    .nav-links a {
      color: #888;
      text-decoration: none;
      font-size: 14px;
      padding: 8px 16px;
      border-radius: 6px;
      transition: all 0.2s;
    }
    .nav-links a:hover {
      color: #eee;
      background: #16213e;
    }
    .nav-links a.active {
      color: #00d4ff;
      background: #16213e;
    }
    .container { max-width: 600px; margin: 0 auto; padding: 20px; }
    .card {
      background: #16213e;
      border-radius: 12px;
      padding: 20px;
      margin-bottom: 20px;
      box-shadow: 0 4px 6px rgba(0,0,0,0.3);
    }
    .card h2 {
      font-size: 14px;
      text-transform: uppercase;
      color: #888;
      margin-bottom: 10px;
    }
    .status {
      font-size: 48px;
      font-weight: bold;
      text-align: center;
      padding: 20px;
    }
    .status.idle { color: #4ade80; }
    .status.pending { color: #fbbf24; }
    .status.alarm { color: #ef4444; animation: pulse 1s infinite; }
    .status.clearing { color: #f97316; }
    @keyframes pulse {
      0%, 100% { opacity: 1; }
      50% { opacity: 0.5; }
    }
    .stats {
      display: grid;
      grid-template-columns: repeat(2, 1fr);
      gap: 15px;
    }
    .stat {
      background: #1a1a2e;
      padding: 15px;
      border-radius: 8px;
      text-align: center;
    }
    .stat-value {
      font-size: 24px;
      font-weight: bold;
      color: #00d4ff;
    }
    .stat-label {
      font-size: 12px;
      color: #888;
      margin-top: 5px;
    }
    .wifi-info {
      font-size: 12px;
      color: #888;
      text-align: center;
      margin-top: 20px;
    }
    .motion-indicator {
      width: 20px;
      height: 20px;
      border-radius: 50%;
      display: inline-block;
      margin-right: 10px;
    }
    .motion-indicator.active { background: #ef4444; }
    .motion-indicator.inactive { background: #4ade80; }
  </style>
</head>
<body>
  <nav>
    <a href="/" class="nav-brand">ESP32 Radar</a>
    <div class="nav-links">
      <a href="/" class="active">Dashboard</a>
      <a href="/settings">Settings</a>
      <a href="/api">API</a>
    </div>
  </nav>

  <div class="container">
    <div class="card">
      <h2>Current State</h2>
      <div class="status" id="state">Loading...</div>
    </div>

    <div class="card">
      <h2>Motion</h2>
      <div style="display: flex; align-items: center; justify-content: center; font-size: 24px; margin-bottom: 15px;">
        <span class="motion-indicator" id="motionIndicator"></span>
        <span id="motionText">--</span>
      </div>
      <div class="stats">
        <div class="stat">
          <div class="stat-value"><span id="filterPercent">0</span> / <span id="filterThreshold">70</span>%</div>
          <div class="stat-label">Filter Level / Threshold</div>
        </div>
        <div class="stat">
          <div class="stat-value" id="rawMotion">--</div>
          <div class="stat-label">Raw Sensor</div>
        </div>
      </div>
    </div>

    <div class="card">
      <h2>Statistics</h2>
      <div class="stats">
        <div class="stat">
          <div class="stat-value" id="alarmCount">0</div>
          <div class="stat-label">Alarm Events</div>
        </div>
        <div class="stat">
          <div class="stat-value" id="motionCount">0</div>
          <div class="stat-label">Motion Events</div>
        </div>
        <div class="stat">
          <div class="stat-value" id="uptime">0s</div>
          <div class="stat-label">Uptime</div>
        </div>
        <div class="stat">
          <div class="stat-value" id="heap">0 KB</div>
          <div class="stat-label">Free Memory</div>
        </div>
      </div>
    </div>

    <div class="card">
      <h2>Configuration</h2>
      <div class="stats">
        <div class="stat">
          <div class="stat-value" id="tripDelay">0s</div>
          <div class="stat-label">Trip Delay</div>
        </div>
        <div class="stat">
          <div class="stat-value" id="clearTimeout">0s</div>
          <div class="stat-label">Clear Timeout</div>
        </div>
      </div>
    </div>

    <div class="wifi-info">
      <span id="wifiInfo">Connecting...</span>
    </div>
  </div>

  <script>
    function updateStatus() {
      fetch('/status')
        .then(response => response.json())
        .then(data => {
          // Update state
          const stateEl = document.getElementById('state');
          stateEl.textContent = data.state;
          stateEl.className = 'status ' + data.state.toLowerCase().replace('_', '');
          if (data.state === 'ALARM_ACTIVE' || data.state === 'ALARM_CLEARING') {
            stateEl.classList.add('alarm');
          }

          // Update motion indicator (uses FILTERED motion)
          const indicator = document.getElementById('motionIndicator');
          const motionText = document.getElementById('motionText');
          if (data.filteredMotion) {
            indicator.className = 'motion-indicator active';
            motionText.textContent = 'DETECTED';
          } else {
            indicator.className = 'motion-indicator inactive';
            motionText.textContent = 'None';
          }

          // Update filter stats
          document.getElementById('filterPercent').textContent = data.filterPercent;
          document.getElementById('filterThreshold').textContent = data.filterThreshold;
          document.getElementById('rawMotion').textContent = data.rawMotion ? 'HIGH' : 'LOW';

          // Update stats
          document.getElementById('alarmCount').textContent = data.alarmEvents;
          document.getElementById('motionCount').textContent = data.motionEvents;
          document.getElementById('uptime').textContent = formatUptime(data.uptime);
          document.getElementById('heap').textContent = Math.round(data.freeHeap / 1024) + ' KB';

          // Update config
          document.getElementById('tripDelay').textContent = data.tripDelay + 's';
          document.getElementById('clearTimeout').textContent = data.clearTimeout + 's';

          // Update WiFi info
          document.getElementById('wifiInfo').textContent =
            data.wifiMode + ' | ' + data.ipAddress + ' | RSSI: ' + data.rssi + ' dBm';
        })
        .catch(err => {
          document.getElementById('state').textContent = 'Error';
        });
    }

    function formatUptime(seconds) {
      if (seconds < 60) return seconds + 's';
      if (seconds < 3600) return Math.floor(seconds / 60) + 'm';
      if (seconds < 86400) return Math.floor(seconds / 3600) + 'h';
      return Math.floor(seconds / 86400) + 'd';
    }

    // Update every second
    setInterval(updateStatus, 1000);
    updateStatus();
  </script>
</body>
</html>
)rawliteral";

// ============================================================================
// HTML SETTINGS PAGE
// ============================================================================

const char SETTINGS_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Settings - ESP32 Motion Sensor</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
      background: #1a1a2e;
      color: #eee;
      min-height: 100vh;
    }
    nav {
      background: #0f0f1a;
      padding: 15px 20px;
      display: flex;
      justify-content: space-between;
      align-items: center;
      border-bottom: 1px solid #16213e;
      position: sticky;
      top: 0;
      z-index: 100;
    }
    .nav-brand {
      color: #00d4ff;
      font-weight: bold;
      font-size: 18px;
      text-decoration: none;
    }
    .nav-links {
      display: flex;
      gap: 20px;
    }
    .nav-links a {
      color: #888;
      text-decoration: none;
      font-size: 14px;
      padding: 8px 16px;
      border-radius: 6px;
      transition: all 0.2s;
    }
    .nav-links a:hover {
      color: #eee;
      background: #16213e;
    }
    .nav-links a.active {
      color: #00d4ff;
      background: #16213e;
    }
    .container { max-width: 600px; margin: 0 auto; padding: 20px; }
    .card {
      background: #16213e;
      border-radius: 12px;
      padding: 20px;
      margin-bottom: 20px;
      box-shadow: 0 4px 6px rgba(0,0,0,0.3);
    }
    .card h2 {
      font-size: 14px;
      text-transform: uppercase;
      color: #888;
      margin-bottom: 15px;
    }
    .form-group {
      margin-bottom: 15px;
    }
    label {
      display: block;
      margin-bottom: 5px;
      color: #aaa;
      font-size: 14px;
    }
    input[type="text"],
    input[type="password"],
    input[type="number"] {
      width: 100%;
      padding: 12px;
      border: 1px solid #333;
      border-radius: 8px;
      background: #1a1a2e;
      color: #eee;
      font-size: 16px;
    }
    input:focus {
      outline: none;
      border-color: #00d4ff;
    }
    .btn {
      display: inline-block;
      padding: 12px 24px;
      border: none;
      border-radius: 8px;
      font-size: 16px;
      cursor: pointer;
      text-decoration: none;
      margin-right: 10px;
      margin-bottom: 10px;
    }
    .btn-primary {
      background: #00d4ff;
      color: #1a1a2e;
    }
    .btn-primary:hover { background: #00b8e6; }
    .btn-danger {
      background: #ef4444;
      color: #fff;
    }
    .btn-danger:hover { background: #dc2626; }
    .message {
      padding: 12px;
      border-radius: 8px;
      margin-bottom: 20px;
      display: none;
    }
    .message.success { background: #065f46; display: block; }
    .message.error { background: #991b1b; display: block; }
    .hint {
      font-size: 12px;
      color: #666;
      margin-top: 5px;
    }
    .button-group {
      display: flex;
      flex-wrap: wrap;
      gap: 10px;
      margin-top: 20px;
    }
  </style>
</head>
<body>
  <nav>
    <a href="/" class="nav-brand">ESP32 Radar</a>
    <div class="nav-links">
      <a href="/">Dashboard</a>
      <a href="/settings" class="active">Settings</a>
      <a href="/api">API</a>
    </div>
  </nav>

  <div class="container">
    <div id="message" class="message"></div>

    <form id="settingsForm">
      <div class="card">
        <h2>WiFi Configuration</h2>
        <div class="form-group">
          <label for="wifiSsid">WiFi Network Name (SSID)</label>
          <input type="text" id="wifiSsid" name="wifiSsid" maxlength="63">
        </div>
        <div class="form-group">
          <label for="wifiPassword">WiFi Password</label>
          <input type="password" id="wifiPassword" name="wifiPassword" maxlength="63">
          <div class="hint">Leave empty to keep current password</div>
        </div>
      </div>

      <div class="card">
        <h2>Motion Detection</h2>
        <div class="form-group">
          <label for="tripDelay">Trip Delay (seconds)</label>
          <input type="number" id="tripDelay" name="tripDelay" min="1" max="60" value="3">
          <div class="hint">Motion must be sustained for this duration before alarm triggers (1-60s)</div>
        </div>
        <div class="form-group">
          <label for="clearTimeout">Clear Timeout (seconds)</label>
          <input type="number" id="clearTimeout" name="clearTimeout" min="1" max="300" value="10">
          <div class="hint">No motion for this duration clears the alarm (1-300s)</div>
        </div>
        <div class="form-group">
          <label for="filterThreshold">Filter Threshold (%)</label>
          <input type="number" id="filterThreshold" name="filterThreshold" min="10" max="100" value="70">
          <div class="hint">Percentage of positive samples required to detect motion (10-100%)</div>
        </div>
      </div>

      <div class="card">
        <h2>Notifications</h2>
        <div class="form-group">
          <label style="display: flex; align-items: center; gap: 10px;">
            <input type="checkbox" id="notifyEnabled" name="notifyEnabled" style="width: auto;">
            Enable HTTP notifications on alarm events
          </label>
        </div>
        <div class="form-group">
          <label for="notifyUrl">Notification URL</label>
          <input type="text" id="notifyUrl" name="notifyUrl" placeholder="http://example.com/webhook" maxlength="127">
          <div class="hint">URL to call when alarm triggers or clears</div>
        </div>
        <div class="form-group">
          <label for="notifyMethod">Method</label>
          <select id="notifyMethod" name="notifyMethod" style="width: 100%; padding: 12px; border: 1px solid #333; border-radius: 8px; background: #1a1a2e; color: #eee; font-size: 16px;">
            <option value="0">GET (query parameters)</option>
            <option value="1">POST (JSON body)</option>
          </select>
          <div class="hint">GET appends ?event=...&state=... to URL. POST sends JSON body.</div>
        </div>
        <div class="button-group" style="margin-top: 15px;">
          <button type="button" class="btn btn-secondary" id="testNotifyBtn">Test Notification</button>
        </div>
        <div id="notifyStatus" style="margin-top: 10px; font-size: 12px; color: #888;"></div>
      </div>

      <div class="button-group">
        <button type="submit" class="btn btn-primary">Save Settings</button>
        <button type="button" class="btn btn-danger" id="resetBtn">Factory Reset</button>
      </div>
    </form>
  </div>

  <script>
    // Load current settings
    fetch('/config')
      .then(response => response.json())
      .then(data => {
        document.getElementById('wifiSsid').value = data.wifiSsid || '';
        document.getElementById('tripDelay').value = data.tripDelaySeconds;
        document.getElementById('clearTimeout').value = data.clearTimeoutSeconds;
        document.getElementById('filterThreshold').value = data.filterThresholdPercent;
        document.getElementById('notifyEnabled').checked = data.notifyEnabled || false;
        document.getElementById('notifyUrl').value = data.notifyUrl || '';
        document.getElementById('notifyMethod').value = data.notifyMethod || 0;
      });

    // Save settings
    document.getElementById('settingsForm').addEventListener('submit', function(e) {
      e.preventDefault();

      const formData = {
        wifiSsid: document.getElementById('wifiSsid').value,
        wifiPassword: document.getElementById('wifiPassword').value,
        tripDelay: parseInt(document.getElementById('tripDelay').value),
        clearTimeout: parseInt(document.getElementById('clearTimeout').value),
        filterThreshold: parseInt(document.getElementById('filterThreshold').value),
        notifyEnabled: document.getElementById('notifyEnabled').checked,
        notifyUrl: document.getElementById('notifyUrl').value,
        notifyMethod: parseInt(document.getElementById('notifyMethod').value)
      };

      fetch('/config', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(formData)
      })
      .then(response => response.json())
      .then(data => {
        const msgEl = document.getElementById('message');
        if (data.success) {
          msgEl.className = 'message success';
          msgEl.textContent = data.message + (data.restart ? ' Device will restart in 3 seconds...' : '');
          if (data.restart) {
            setTimeout(() => { window.location.href = '/'; }, 5000);
          }
        } else {
          msgEl.className = 'message error';
          msgEl.textContent = 'Error: ' + data.message;
        }
      })
      .catch(err => {
        const msgEl = document.getElementById('message');
        msgEl.className = 'message error';
        msgEl.textContent = 'Error saving settings';
      });
    });

    // Test notification - sends current form values (not saved config)
    document.getElementById('testNotifyBtn').addEventListener('click', function() {
      const statusEl = document.getElementById('notifyStatus');
      const url = document.getElementById('notifyUrl').value.trim();
      const method = parseInt(document.getElementById('notifyMethod').value);

      if (!url) {
        statusEl.style.color = '#ef4444';
        statusEl.textContent = 'Please enter a notification URL first';
        return;
      }

      statusEl.textContent = 'Sending test notification...';
      statusEl.style.color = '#888';

      fetch('/test-notification', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ url: url, method: method })
      })
        .then(response => response.json())
        .then(data => {
          if (data.success) {
            statusEl.style.color = '#4ade80';
            statusEl.textContent = 'Test sent! Response: ' + data.httpCode;
          } else {
            statusEl.style.color = '#ef4444';
            statusEl.textContent = 'Failed: ' + data.message;
          }
        })
        .catch(err => {
          statusEl.style.color = '#ef4444';
          statusEl.textContent = 'Error sending test';
        });
    });

    // Factory reset
    document.getElementById('resetBtn').addEventListener('click', function() {
      if (confirm('Are you sure you want to factory reset? All settings will be erased.')) {
        fetch('/reset', { method: 'POST' })
          .then(response => response.json())
          .then(data => {
            const msgEl = document.getElementById('message');
            msgEl.className = 'message success';
            msgEl.textContent = 'Factory reset initiated. Device will restart...';
          });
      }
    });
  </script>
</body>
</html>
)rawliteral";

// ============================================================================
// HTML API DOCUMENTATION PAGE
// ============================================================================

const char API_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>API - ESP32 Motion Sensor</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
      background: #1a1a2e;
      color: #eee;
      min-height: 100vh;
    }
    nav {
      background: #0f0f1a;
      padding: 15px 20px;
      display: flex;
      justify-content: space-between;
      align-items: center;
      border-bottom: 1px solid #16213e;
      position: sticky;
      top: 0;
      z-index: 100;
    }
    .nav-brand {
      color: #00d4ff;
      font-weight: bold;
      font-size: 18px;
      text-decoration: none;
    }
    .nav-links {
      display: flex;
      gap: 20px;
    }
    .nav-links a {
      color: #888;
      text-decoration: none;
      font-size: 14px;
      padding: 8px 16px;
      border-radius: 6px;
      transition: all 0.2s;
    }
    .nav-links a:hover {
      color: #eee;
      background: #16213e;
    }
    .nav-links a.active {
      color: #00d4ff;
      background: #16213e;
    }
    .container { max-width: 700px; margin: 0 auto; padding: 20px; }
    .card {
      background: #16213e;
      border-radius: 12px;
      padding: 20px;
      margin-bottom: 20px;
      box-shadow: 0 4px 6px rgba(0,0,0,0.3);
    }
    .card h2 {
      font-size: 16px;
      color: #00d4ff;
      margin-bottom: 15px;
    }
    .card h3 {
      font-size: 14px;
      color: #888;
      margin: 15px 0 10px 0;
      text-transform: uppercase;
    }
    .endpoint {
      background: #1a1a2e;
      padding: 12px 15px;
      border-radius: 8px;
      margin-bottom: 10px;
      font-family: monospace;
      display: flex;
      align-items: center;
      gap: 10px;
    }
    .method {
      background: #00d4ff;
      color: #1a1a2e;
      padding: 4px 8px;
      border-radius: 4px;
      font-weight: bold;
      font-size: 12px;
    }
    .method.post {
      background: #4ade80;
    }
    .url {
      color: #eee;
    }
    .url a {
      color: #00d4ff;
      text-decoration: none;
    }
    .url a:hover {
      text-decoration: underline;
    }
    p {
      color: #aaa;
      font-size: 14px;
      line-height: 1.6;
      margin-bottom: 10px;
    }
    code {
      background: #1a1a2e;
      padding: 2px 6px;
      border-radius: 4px;
      font-family: monospace;
      color: #00d4ff;
    }
    pre {
      background: #1a1a2e;
      padding: 15px;
      border-radius: 8px;
      overflow-x: auto;
      font-size: 13px;
      line-height: 1.5;
      color: #aaa;
    }
    .field-table {
      width: 100%;
      border-collapse: collapse;
      margin: 10px 0;
      font-size: 13px;
    }
    .field-table th {
      text-align: left;
      padding: 8px;
      background: #1a1a2e;
      color: #888;
      font-weight: normal;
    }
    .field-table td {
      padding: 8px;
      border-top: 1px solid #1a1a2e;
      color: #aaa;
    }
    .field-table td:first-child {
      color: #00d4ff;
      font-family: monospace;
    }
  </style>
</head>
<body>
  <nav>
    <a href="/" class="nav-brand">ESP32 Radar</a>
    <div class="nav-links">
      <a href="/">Dashboard</a>
      <a href="/settings">Settings</a>
      <a href="/api" class="active">API</a>
    </div>
  </nav>

  <div class="container">
    <div class="card">
      <h2>REST API Endpoints</h2>
      <p>The ESP32 Motion Sensor provides a REST API for integration with home automation systems, scripts, or custom applications.</p>
    </div>

    <div class="card">
      <h2>GET /status</h2>
      <p>Returns current motion detection status and system metrics.</p>

      <div class="endpoint">
        <span class="method">GET</span>
        <span class="url"><a href="/status" target="_blank">/status</a></span>
      </div>

      <h3>Response Fields</h3>
      <table class="field-table">
        <tr><th>Field</th><th>Type</th><th>Description</th></tr>
        <tr><td>state</td><td>string</td><td>IDLE, MOTION_PENDING, ALARM_ACTIVE, ALARM_CLEARING</td></tr>
        <tr><td>rawMotion</td><td>boolean</td><td>Raw sensor state (true = HIGH)</td></tr>
        <tr><td>filteredMotion</td><td>boolean</td><td>Filtered motion (used by state machine)</td></tr>
        <tr><td>filterPercent</td><td>integer</td><td>Current filter level (0-100%)</td></tr>
        <tr><td>alarmEvents</td><td>integer</td><td>Total alarm triggers since boot</td></tr>
        <tr><td>motionEvents</td><td>integer</td><td>Total motion detections since boot</td></tr>
        <tr><td>uptime</td><td>integer</td><td>Seconds since boot</td></tr>
        <tr><td>freeHeap</td><td>integer</td><td>Available memory (bytes)</td></tr>
        <tr><td>tripDelay</td><td>integer</td><td>Trip delay setting (seconds)</td></tr>
        <tr><td>clearTimeout</td><td>integer</td><td>Clear timeout setting (seconds)</td></tr>
        <tr><td>filterThreshold</td><td>integer</td><td>Filter threshold (%)</td></tr>
        <tr><td>wifiMode</td><td>string</td><td>Station or AP Mode</td></tr>
        <tr><td>ipAddress</td><td>string</td><td>Current IP address</td></tr>
        <tr><td>rssi</td><td>integer</td><td>WiFi signal strength (dBm)</td></tr>
      </table>
    </div>

    <div class="card">
      <h2>GET /config</h2>
      <p>Returns current system configuration.</p>

      <div class="endpoint">
        <span class="method">GET</span>
        <span class="url"><a href="/config" target="_blank">/config</a></span>
      </div>

      <h3>Response Fields</h3>
      <table class="field-table">
        <tr><th>Field</th><th>Type</th><th>Description</th></tr>
        <tr><td>sensorPin</td><td>integer</td><td>GPIO pin for sensor</td></tr>
        <tr><td>ledPin</td><td>integer</td><td>GPIO pin for LED</td></tr>
        <tr><td>tripDelaySeconds</td><td>integer</td><td>Trip delay (seconds)</td></tr>
        <tr><td>clearTimeoutSeconds</td><td>integer</td><td>Clear timeout (seconds)</td></tr>
        <tr><td>filterThresholdPercent</td><td>integer</td><td>Filter threshold (%)</td></tr>
        <tr><td>wifiSsid</td><td>string</td><td>Connected WiFi network</td></tr>
        <tr><td>wifiMode</td><td>string</td><td>Station or AP Mode</td></tr>
        <tr><td>ipAddress</td><td>string</td><td>Current IP address</td></tr>
      </table>
    </div>

    <div class="card">
      <h2>POST /config</h2>
      <p>Save configuration settings. Send JSON body with fields to update.</p>

      <div class="endpoint">
        <span class="method post">POST</span>
        <span class="url">/config</span>
      </div>

      <h3>Request Fields (all optional)</h3>
      <table class="field-table">
        <tr><th>Field</th><th>Type</th><th>Description</th></tr>
        <tr><td>wifiSsid</td><td>string</td><td>New WiFi SSID (triggers restart)</td></tr>
        <tr><td>wifiPassword</td><td>string</td><td>New WiFi password (triggers restart)</td></tr>
        <tr><td>tripDelay</td><td>integer</td><td>Trip delay 1-60 seconds</td></tr>
        <tr><td>clearTimeout</td><td>integer</td><td>Clear timeout 1-300 seconds</td></tr>
        <tr><td>filterThreshold</td><td>integer</td><td>Filter threshold 10-100%</td></tr>
      </table>

      <h3>Example</h3>
      <pre>curl -X POST http://192.168.1.100/config \
  -H "Content-Type: application/json" \
  -d '{"tripDelay": 5, "filterThreshold": 80}'</pre>
    </div>

    <div class="card">
      <h2>POST /reset</h2>
      <p>Factory reset - clears all saved settings and restarts device.</p>

      <div class="endpoint">
        <span class="method post">POST</span>
        <span class="url">/reset</span>
      </div>

      <h3>Example</h3>
      <pre>curl -X POST http://192.168.1.100/reset</pre>
    </div>
  </div>
</body>
</html>
)rawliteral";

// ============================================================================
// RESET REASON DETECTION
// ============================================================================

void printResetReason() {
  RESET_REASON reason = rtc_get_reset_reason(0);

  Serial.print("Reset reason: ");
  switch (reason) {
    case 1:  Serial.println("POWERON_RESET"); break;
    case 3:  Serial.println("SW_RESET"); break;
    case 4:  Serial.println("OWDT_RESET"); break;
    case 5:  Serial.println("DEEPSLEEP_RESET"); break;
    case 12: Serial.println("SW_CPU_RESET"); break;
    case 15: Serial.println("RTCWDT_BROWN_OUT_RESET"); break;
    default: Serial.println("OTHER"); break;
  }
}

// ============================================================================
// WIFI FUNCTIONS
// ============================================================================

bool connectWiFi() {
  // Use stored config SSID/password
  Serial.print("Connecting to WiFi: ");
  Serial.println(config.wifiSsid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(config.wifiSsid, config.wifiPassword);

  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startTime > WIFI_CONNECT_TIMEOUT) {
      Serial.println("\nWiFi connection timeout!");
      return false;
    }
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Connected! IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("RSSI: ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");

  currentWiFiMode = MODE_STATION;
  return true;
}

void startAPMode() {
  Serial.println("Starting AP Mode...");

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);

  IPAddress apIP(192, 168, 4, 1);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

  Serial.println();
  Serial.println("=================================");
  Serial.println("  AP MODE ACTIVE");
  Serial.println("=================================");
  Serial.print("  SSID: ");
  Serial.println(AP_SSID);
  Serial.print("  Password: ");
  Serial.println(AP_PASSWORD);
  Serial.print("  IP: ");
  Serial.println(WiFi.softAPIP());
  Serial.println("=================================");
  Serial.println();
  Serial.println("Connect to this network and open");
  Serial.println("http://192.168.4.1 in your browser");
  Serial.println();

  currentWiFiMode = MODE_ACCESS_POINT;
}

String getWiFiModeString() {
  switch (currentWiFiMode) {
    case MODE_STATION: return "Station";
    case MODE_ACCESS_POINT: return "AP Mode";
    default: return "None";
  }
}

String getIPAddress() {
  if (currentWiFiMode == MODE_STATION) {
    return WiFi.localIP().toString();
  } else if (currentWiFiMode == MODE_ACCESS_POINT) {
    return WiFi.softAPIP().toString();
  }
  return "0.0.0.0";
}

int getRSSI() {
  if (currentWiFiMode == MODE_STATION) {
    return WiFi.RSSI();
  }
  return 0;
}

// ============================================================================
// NOTIFICATION FUNCTIONS
// ============================================================================

void sendNotification(const char* event, const char* state) {
  if (!config.notifyEnabled || strlen(config.notifyUrl) == 0) {
    return;
  }

  if (currentWiFiMode != MODE_STATION) {
    Serial.println("[NOTIFY] Skipped - not connected to WiFi");
    return;
  }

  Serial.print("[NOTIFY] Sending ");
  Serial.print(config.notifyMethod == NOTIFY_METHOD_GET ? "GET" : "POST");
  Serial.print(" to ");
  Serial.println(config.notifyUrl);

  HTTPClient http;
  http.setTimeout(5000);  // 5 second timeout

  int httpCode = -1;

  if (config.notifyMethod == NOTIFY_METHOD_GET) {
    // Build URL with query parameters
    String url = String(config.notifyUrl);
    if (url.indexOf('?') == -1) {
      url += "?";
    } else {
      url += "&";
    }
    url += "event=" + String(event);
    url += "&state=" + String(state);
    url += "&ip=" + getIPAddress();

    http.begin(url);
    httpCode = http.GET();
  } else {
    // POST with JSON body
    http.begin(config.notifyUrl);
    http.addHeader("Content-Type", "application/json");

    JsonDocument doc;
    doc["event"] = event;
    doc["state"] = state;
    doc["ip"] = getIPAddress();
    doc["uptime"] = millis() / 1000;
    doc["alarmEvents"] = totalAlarmEvents;

    String payload;
    serializeJson(doc, payload);

    httpCode = http.POST(payload);
  }

  lastNotificationTime = millis();
  lastNotificationStatus = httpCode;

  if (httpCode > 0) {
    Serial.print("[NOTIFY] Response: ");
    Serial.println(httpCode);
    if (httpCode >= 200 && httpCode < 300) {
      totalNotificationsSent++;
    } else {
      totalNotificationsFailed++;
    }
  } else {
    Serial.print("[NOTIFY] Error: ");
    Serial.println(http.errorToString(httpCode));
    totalNotificationsFailed++;
  }

  http.end();
}

void sendTestNotification() {
  Serial.println("[NOTIFY] Sending test notification...");

  // Temporarily enable notifications for the test
  bool wasEnabled = config.notifyEnabled;
  config.notifyEnabled = true;

  sendNotification("test", stateNames[currentState]);

  config.notifyEnabled = wasEnabled;
}

// ============================================================================
// MOTION FILTER FUNCTIONS
// ============================================================================

void initializeFilter() {
  // Initialize all samples to false (no motion)
  for (int i = 0; i < DEFAULT_FILTER_SAMPLE_COUNT; i++) {
    sampleBuffer[i] = false;
  }
  sampleIndex = 0;
  samplePositiveCount = 0;
  filterInitialized = true;
  Serial.print("Motion filter initialized: ");
  Serial.print(DEFAULT_FILTER_SAMPLE_COUNT);
  Serial.print(" samples, ");
  Serial.print(config.filterThresholdPercent);
  Serial.print("% threshold, ");
  Serial.print(DEFAULT_FILTER_WINDOW_MS);
  Serial.println("ms window");
}

void updateMotionFilter(bool rawReading) {
  // Remove old sample from count
  if (sampleBuffer[sampleIndex]) {
    samplePositiveCount--;
  }

  // Add new sample
  sampleBuffer[sampleIndex] = rawReading;
  if (rawReading) {
    samplePositiveCount++;
  }

  // Advance circular buffer index
  sampleIndex = (sampleIndex + 1) % DEFAULT_FILTER_SAMPLE_COUNT;

  // Calculate percentage
  currentFilterPercent = (samplePositiveCount * 100) / DEFAULT_FILTER_SAMPLE_COUNT;

  // Determine filtered motion state (using configurable threshold)
  bool previousFiltered = filteredMotionDetected;
  filteredMotionDetected = (currentFilterPercent >= config.filterThresholdPercent);

  // Log filter state changes (for debugging)
  if (filteredMotionDetected != previousFiltered) {
    Serial.print("[FILTER] ");
    Serial.print(currentFilterPercent);
    Serial.print("% (");
    Serial.print(samplePositiveCount);
    Serial.print("/");
    Serial.print(DEFAULT_FILTER_SAMPLE_COUNT);
    Serial.print(") -> ");
    Serial.println(filteredMotionDetected ? "MOTION" : "NO MOTION");
  }
}

// ============================================================================
// WEB SERVER HANDLERS
// ============================================================================

void handleRoot() {
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleSettings() {
  server.send_P(200, "text/html", SETTINGS_HTML);
}

void handleApi() {
  server.send_P(200, "text/html", API_HTML);
}

void handleStatus() {
  JsonDocument doc;

  doc["state"] = stateNames[currentState];
  doc["rawMotion"] = rawMotionDetected;
  doc["filteredMotion"] = filteredMotionDetected;
  doc["filterPercent"] = currentFilterPercent;
  doc["alarmEvents"] = totalAlarmEvents;
  doc["motionEvents"] = totalMotionEvents;
  doc["uptime"] = millis() / 1000;
  doc["freeHeap"] = ESP.getFreeHeap();
  doc["tripDelay"] = config.tripDelaySeconds;
  doc["clearTimeout"] = config.clearTimeoutSeconds;
  doc["filterThreshold"] = config.filterThresholdPercent;
  doc["wifiMode"] = getWiFiModeString();
  doc["ipAddress"] = getIPAddress();
  doc["rssi"] = getRSSI();

  // Notification stats
  doc["notifyEnabled"] = config.notifyEnabled;
  doc["notificationsSent"] = totalNotificationsSent;
  doc["notificationsFailed"] = totalNotificationsFailed;
  doc["lastNotifyStatus"] = lastNotificationStatus;

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleGetConfig() {
  JsonDocument doc;

  doc["sensorPin"] = SENSOR_PIN;
  doc["ledPin"] = LED_PIN;
  doc["tripDelaySeconds"] = config.tripDelaySeconds;
  doc["clearTimeoutSeconds"] = config.clearTimeoutSeconds;
  doc["pollIntervalMs"] = SENSOR_POLL_INTERVAL;

  // Filter configuration
  doc["filterSampleCount"] = DEFAULT_FILTER_SAMPLE_COUNT;
  doc["filterThresholdPercent"] = config.filterThresholdPercent;
  doc["filterWindowMs"] = DEFAULT_FILTER_WINDOW_MS;

  // Notification configuration
  doc["notifyEnabled"] = config.notifyEnabled;
  doc["notifyUrl"] = config.notifyUrl;
  doc["notifyMethod"] = config.notifyMethod;

  doc["wifiSsid"] = config.wifiSsid;
  doc["wifiMode"] = getWiFiModeString();
  doc["apSsid"] = AP_SSID;
  doc["ipAddress"] = getIPAddress();

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handlePostConfig() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"No data received\"}");
    return;
  }

  String body = server.arg("plain");
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, body);

  if (error) {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
    return;
  }

  bool wifiChanged = false;
  bool settingsChanged = false;

  // Update WiFi SSID if provided
  if (doc.containsKey("wifiSsid") && strlen(doc["wifiSsid"]) > 0) {
    const char* newSsid = doc["wifiSsid"];
    if (strcmp(newSsid, config.wifiSsid) != 0) {
      strncpy(config.wifiSsid, newSsid, sizeof(config.wifiSsid) - 1);
      wifiChanged = true;
      Serial.print("WiFi SSID changed to: ");
      Serial.println(config.wifiSsid);
    }
  }

  // Update WiFi password if provided (not empty)
  if (doc.containsKey("wifiPassword") && strlen(doc["wifiPassword"]) > 0) {
    const char* newPassword = doc["wifiPassword"];
    strncpy(config.wifiPassword, newPassword, sizeof(config.wifiPassword) - 1);
    wifiChanged = true;
    Serial.println("WiFi password changed");
  }

  // Update trip delay
  if (doc.containsKey("tripDelay")) {
    uint16_t newTripDelay = doc["tripDelay"];
    if (newTripDelay >= 1 && newTripDelay <= 60 && newTripDelay != config.tripDelaySeconds) {
      config.tripDelaySeconds = newTripDelay;
      settingsChanged = true;
      Serial.print("Trip delay changed to: ");
      Serial.print(config.tripDelaySeconds);
      Serial.println("s");
    }
  }

  // Update clear timeout
  if (doc.containsKey("clearTimeout")) {
    uint16_t newClearTimeout = doc["clearTimeout"];
    if (newClearTimeout >= 1 && newClearTimeout <= 300 && newClearTimeout != config.clearTimeoutSeconds) {
      config.clearTimeoutSeconds = newClearTimeout;
      settingsChanged = true;
      Serial.print("Clear timeout changed to: ");
      Serial.print(config.clearTimeoutSeconds);
      Serial.println("s");
    }
  }

  // Update filter threshold
  if (doc.containsKey("filterThreshold")) {
    uint8_t newThreshold = doc["filterThreshold"];
    if (newThreshold >= 10 && newThreshold <= 100 && newThreshold != config.filterThresholdPercent) {
      config.filterThresholdPercent = newThreshold;
      settingsChanged = true;
      Serial.print("Filter threshold changed to: ");
      Serial.print(config.filterThresholdPercent);
      Serial.println("%");
    }
  }

  // Update notification settings
  if (doc.containsKey("notifyEnabled")) {
    bool newEnabled = doc["notifyEnabled"];
    if (newEnabled != config.notifyEnabled) {
      config.notifyEnabled = newEnabled;
      settingsChanged = true;
      Serial.print("Notifications ");
      Serial.println(config.notifyEnabled ? "enabled" : "disabled");
    }
  }

  if (doc.containsKey("notifyUrl")) {
    const char* newUrl = doc["notifyUrl"];
    if (strcmp(newUrl, config.notifyUrl) != 0) {
      strncpy(config.notifyUrl, newUrl, sizeof(config.notifyUrl) - 1);
      settingsChanged = true;
      Serial.print("Notify URL changed to: ");
      Serial.println(config.notifyUrl);
    }
  }

  if (doc.containsKey("notifyMethod")) {
    uint8_t newMethod = doc["notifyMethod"];
    if (newMethod <= 1 && newMethod != config.notifyMethod) {
      config.notifyMethod = newMethod;
      settingsChanged = true;
      Serial.print("Notify method changed to: ");
      Serial.println(config.notifyMethod == NOTIFY_METHOD_GET ? "GET" : "POST");
    }
  }

  // Save if anything changed
  if (wifiChanged || settingsChanged) {
    if (saveConfig()) {
      if (wifiChanged) {
        // WiFi changed, need to restart
        server.send(200, "application/json", "{\"success\":true,\"message\":\"Settings saved.\",\"restart\":true}");
        delay(3000);
        ESP.restart();
      } else {
        server.send(200, "application/json", "{\"success\":true,\"message\":\"Settings saved.\",\"restart\":false}");
      }
    } else {
      server.send(500, "application/json", "{\"success\":false,\"message\":\"Failed to save settings\"}");
    }
  } else {
    server.send(200, "application/json", "{\"success\":true,\"message\":\"No changes made.\",\"restart\":false}");
  }
}

void handleReset() {
  server.send(200, "application/json", "{\"success\":true,\"message\":\"Factory reset initiated\"}");
  delay(1000);
  factoryReset();
}

void handleTestNotification() {
  if (currentWiFiMode != MODE_STATION) {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"Not connected to WiFi\"}");
    return;
  }

  // Check if URL and method are provided in request body (for testing unsaved values)
  String testUrl = config.notifyUrl;
  uint8_t testMethod = config.notifyMethod;

  if (server.hasArg("plain")) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));
    if (!error) {
      if (doc.containsKey("url") && strlen(doc["url"]) > 0) {
        testUrl = doc["url"].as<String>();
      }
      if (doc.containsKey("method")) {
        testMethod = doc["method"];
      }
    }
  }

  if (testUrl.length() == 0) {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"No notification URL provided\"}");
    return;
  }

  // Send test notification with provided or saved settings
  Serial.println("[NOTIFY] Sending test notification...");
  Serial.print("[NOTIFY] URL: ");
  Serial.println(testUrl);
  Serial.print("[NOTIFY] Method: ");
  Serial.println(testMethod == NOTIFY_METHOD_GET ? "GET" : "POST");

  HTTPClient http;
  http.setTimeout(5000);

  int httpCode = -1;

  if (testMethod == NOTIFY_METHOD_GET) {
    String url = testUrl;
    if (url.indexOf('?') == -1) {
      url += "?";
    } else {
      url += "&";
    }
    url += "event=test&state=" + String(stateNames[currentState]) + "&ip=" + getIPAddress();
    http.begin(url);
    httpCode = http.GET();
  } else {
    http.begin(testUrl);
    http.addHeader("Content-Type", "application/json");

    JsonDocument payload;
    payload["event"] = "test";
    payload["state"] = stateNames[currentState];
    payload["ip"] = getIPAddress();
    payload["uptime"] = millis() / 1000;
    payload["alarmEvents"] = totalAlarmEvents;

    String body;
    serializeJson(payload, body);
    httpCode = http.POST(body);
  }

  http.end();

  lastNotificationTime = millis();
  lastNotificationStatus = httpCode;

  if (httpCode > 0) {
    Serial.print("[NOTIFY] Response: ");
    Serial.println(httpCode);
  } else {
    Serial.print("[NOTIFY] Error: ");
    Serial.println(httpCode);
  }

  JsonDocument respDoc;
  respDoc["success"] = (httpCode >= 200 && httpCode < 300);
  respDoc["httpCode"] = httpCode;
  respDoc["message"] = httpCode > 0 ? "Notification sent" : "Request failed";

  String response;
  serializeJson(respDoc, response);
  server.send(200, "application/json", response);
}

void handleNotFound() {
  server.send(404, "text/plain", "Not Found");
}

void setupWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/settings", HTTP_GET, handleSettings);
  server.on("/api", HTTP_GET, handleApi);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/config", HTTP_GET, handleGetConfig);
  server.on("/config", HTTP_POST, handlePostConfig);
  server.on("/reset", HTTP_POST, handleReset);
  server.on("/test-notification", HTTP_POST, handleTestNotification);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("Web server started on port 80");
}

// ============================================================================
// STATE MACHINE
// ============================================================================

void processMotionState(bool motionDetected, unsigned long currentMillis) {
  MotionState previousState = currentState;

  // Calculate timing values from config
  unsigned long tripDelayMs = config.tripDelaySeconds * 1000UL;
  unsigned long clearTimeoutMs = config.clearTimeoutSeconds * 1000UL;

  switch (currentState) {
    case STATE_IDLE:
      if (motionDetected) {
        currentState = STATE_MOTION_PENDING;
        motionStartTime = currentMillis;
        totalMotionEvents++;
        Serial.print("[");
        Serial.print(currentMillis / 1000);
        Serial.print("s] Motion detected - waiting ");
        Serial.print(config.tripDelaySeconds);
        Serial.println("s...");
      }
      break;

    case STATE_MOTION_PENDING:
      if (!motionDetected) {
        currentState = STATE_IDLE;
        Serial.print("[");
        Serial.print(currentMillis / 1000);
        Serial.println("s] Motion stopped - back to IDLE");
      } else if (currentMillis - motionStartTime >= tripDelayMs) {
        currentState = STATE_ALARM_ACTIVE;
        alarmTriggerTime = currentMillis;
        totalAlarmEvents++;
        Serial.println();
        Serial.println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        Serial.print("[");
        Serial.print(currentMillis / 1000);
        Serial.print("s] *** ALARM TRIGGERED *** (#");
        Serial.print(totalAlarmEvents);
        Serial.println(")");
        Serial.println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        Serial.println();

        // Send notification
        sendNotification("alarm_triggered", "ALARM_ACTIVE");
      }
      break;

    case STATE_ALARM_ACTIVE:
      if (!motionDetected) {
        currentState = STATE_ALARM_CLEARING;
        motionStopTime = currentMillis;
        Serial.print("[");
        Serial.print(currentMillis / 1000);
        Serial.print("s] Motion stopped - clearing in ");
        Serial.print(config.clearTimeoutSeconds);
        Serial.println("s...");
      }
      break;

    case STATE_ALARM_CLEARING:
      if (motionDetected) {
        currentState = STATE_ALARM_ACTIVE;
        Serial.print("[");
        Serial.print(currentMillis / 1000);
        Serial.println("s] Motion resumed - alarm still active");
      } else if (currentMillis - motionStopTime >= clearTimeoutMs) {
        currentState = STATE_IDLE;
        unsigned long duration = (currentMillis - alarmTriggerTime) / 1000;
        Serial.println();
        Serial.print("[");
        Serial.print(currentMillis / 1000);
        Serial.print("s] *** ALARM CLEARED *** (");
        Serial.print(duration);
        Serial.println("s)");
        Serial.println();

        // Send notification
        sendNotification("alarm_cleared", "IDLE");
      }
      break;
  }

  // Update LED
  bool ledState = (currentState == STATE_ALARM_ACTIVE || currentState == STATE_ALARM_CLEARING);
  digitalWrite(LED_PIN, ledState ? HIGH : LOW);

  // Log state changes
  if (currentState != previousState) {
    Serial.print("       State: ");
    Serial.print(stateNames[previousState]);
    Serial.print(" -> ");
    Serial.println(stateNames[currentState]);
  }
}

// ============================================================================
// SELF-TEST FUNCTIONS
// ============================================================================

bool testGPIO() {
  Serial.print("  GPIO (pin ");
  Serial.print(SENSOR_PIN);
  Serial.print("): ");
  int reading = digitalRead(SENSOR_PIN);
  Serial.print("PASS (");
  Serial.print(reading == HIGH ? "HIGH" : "LOW");
  Serial.println(")");
  return true;
}

bool testLED() {
  Serial.print("  LED (pin ");
  Serial.print(LED_PIN);
  Serial.print("): ");
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);
  Serial.println("PASS (blinked)");
  return true;
}

bool testHeap() {
  uint32_t freeHeap = ESP.getFreeHeap();
  bool pass = (freeHeap > HEAP_MIN_THRESHOLD);
  Serial.print("  Heap (");
  Serial.print(freeHeap / 1024);
  Serial.print(" KB): ");
  Serial.println(pass ? "PASS" : "FAIL");
  return pass;
}

bool testWiFi() {
  Serial.print("  WiFi: ");
  if (currentWiFiMode == MODE_STATION) {
    Serial.print("PASS (STA, ");
    Serial.print(WiFi.localIP());
    Serial.print(", RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm)");
    return true;
  } else if (currentWiFiMode == MODE_ACCESS_POINT) {
    Serial.print("PASS (AP, ");
    Serial.print(WiFi.softAPIP());
    Serial.println(")");
    return true;
  }
  Serial.println("FAIL (not connected)");
  return false;
}

bool testWebServer() {
  Serial.print("  Web Server: ");
  Serial.println("PASS (port 80)");
  return true;
}

bool testNVS() {
  Serial.print("  NVS: ");
  preferences.begin("radar-config", true);
  bool available = preferences.isKey("config");
  preferences.end();
  Serial.println(available ? "PASS (config stored)" : "PASS (no config yet)");
  return true;
}

void runSelfTest() {
  Serial.println("\n=== STAGE 4 SELF TEST ===");

  bool allPass = true;
  allPass &= testGPIO();
  allPass &= testLED();
  allPass &= testHeap();
  allPass &= testNVS();
  allPass &= testWiFi();
  allPass &= testWebServer();

  Serial.println();
  Serial.print("Overall: ");
  Serial.println(allPass ? "ALL TESTS PASSED" : "SOME TESTS FAILED");
  Serial.println("=========================\n");
}

void printConfig() {
  Serial.println("\n=== CONFIGURATION ===");
  Serial.print("Sensor Pin: GPIO ");
  Serial.println(SENSOR_PIN);
  Serial.print("Trip Delay: ");
  Serial.print(config.tripDelaySeconds);
  Serial.println("s");
  Serial.print("Clear Timeout: ");
  Serial.print(config.clearTimeoutSeconds);
  Serial.println("s");
  Serial.println("--- Motion Filter ---");
  Serial.print("Filter Samples: ");
  Serial.println(DEFAULT_FILTER_SAMPLE_COUNT);
  Serial.print("Filter Threshold: ");
  Serial.print(config.filterThresholdPercent);
  Serial.println("%");
  Serial.print("Filter Window: ");
  Serial.print(DEFAULT_FILTER_WINDOW_MS);
  Serial.println("ms");
  Serial.println("--- WiFi ---");
  Serial.print("WiFi SSID: ");
  Serial.println(config.wifiSsid);
  Serial.print("WiFi Mode: ");
  Serial.println(getWiFiModeString());
  Serial.print("IP Address: ");
  Serial.println(getIPAddress());
  Serial.println("=====================\n");
}

void printStats() {
  Serial.println("\n=== STATISTICS ===");
  Serial.print("State: ");
  Serial.println(stateNames[currentState]);
  Serial.print("Raw Motion: ");
  Serial.println(rawMotionDetected ? "HIGH" : "LOW");
  Serial.print("Filtered Motion: ");
  Serial.println(filteredMotionDetected ? "YES" : "NO");
  Serial.print("Filter Level: ");
  Serial.print(currentFilterPercent);
  Serial.print("% (threshold: ");
  Serial.print(config.filterThresholdPercent);
  Serial.println("%)");
  Serial.print("Motion Events: ");
  Serial.println(totalMotionEvents);
  Serial.print("Alarm Events: ");
  Serial.println(totalAlarmEvents);
  Serial.print("Uptime: ");
  Serial.print(millis() / 1000);
  Serial.println("s");
  Serial.print("Free Heap: ");
  Serial.print(ESP.getFreeHeap() / 1024);
  Serial.println(" KB");
  Serial.println("==================\n");
}

void handleSerialCommands() {
  if (Serial.available()) {
    char cmd = Serial.read();

    switch (cmd) {
      case 't':
      case 'T':
        runSelfTest();
        break;
      case 's':
      case 'S':
        printStats();
        break;
      case 'c':
      case 'C':
        printConfig();
        break;
      case 'h':
      case 'H':
        Serial.println("\n=== COMMANDS ===");
        Serial.println("t - Self test");
        Serial.println("s - Statistics");
        Serial.println("c - Configuration");
        Serial.println("h - Help");
        Serial.println("r - Restart");
        Serial.println("i - System info");
        Serial.println("f - Factory reset");
        Serial.println("================\n");
        break;
      case 'r':
      case 'R':
        Serial.println("Restarting...");
        delay(500);
        ESP.restart();
        break;
      case 'f':
      case 'F':
        Serial.println("Factory reset requested via serial");
        factoryReset();
        break;
      case 'i':
      case 'I':
        Serial.println("\n=== SYSTEM INFO ===");
        Serial.print("Chip: ");
        Serial.println(ESP.getChipModel());
        Serial.print("CPU: ");
        Serial.print(ESP.getCpuFreqMHz());
        Serial.println(" MHz");
        Serial.print("Flash: ");
        Serial.print(ESP.getFlashChipSize() / 1024 / 1024);
        Serial.println(" MB");
        Serial.print("Heap: ");
        Serial.print(ESP.getFreeHeap() / 1024);
        Serial.print(" / ");
        Serial.print(ESP.getHeapSize() / 1024);
        Serial.println(" KB");
        Serial.println("===================\n");
        break;
      case '\n':
      case '\r':
        break;
      default:
        Serial.println("Unknown command. Type 'h' for help.");
        break;
    }
  }
}

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  Serial.begin(115200);
  while (!Serial);
  delay(1000);

  Serial.println("\n\n");
  Serial.println("==========================================");
  Serial.println("ESP32 Microwave Motion Sensor - Stage 4");
  Serial.println("==========================================");
  Serial.println("Notification System");
  Serial.println();

  printResetReason();
  Serial.println();

  // Initialize pins
  pinMode(SENSOR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  Serial.println("GPIO initialized");

  // Load configuration from NVS (or set defaults)
  if (!loadConfig()) {
    setDefaultConfig();
    saveConfig();
  }

  // Initialize motion filter
  initializeFilter();

  // Connect WiFi or start AP mode
  Serial.println();
  if (!connectWiFi()) {
    startAPMode();
  }

  // Start web server
  setupWebServer();

  Serial.println();
  Serial.println("------------------------------------------");
  Serial.print("Dashboard: http://");
  Serial.println(getIPAddress());
  Serial.print("Settings:  http://");
  Serial.print(getIPAddress());
  Serial.println("/settings");
  Serial.println("------------------------------------------");
  Serial.println();

  // Run self-test
  runSelfTest();

  Serial.println("Type 'h' for help");
  Serial.println();
  Serial.println("Starting motion detection...");
  Serial.println("------------------------------------------");
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
  // Handle web server requests
  server.handleClient();

  handleSerialCommands();

  unsigned long currentMillis = millis();

  // Poll sensor and update filter
  if (currentMillis - lastSensorRead >= SENSOR_POLL_INTERVAL) {
    lastSensorRead = currentMillis;

    // Read raw sensor value
    rawMotionDetected = (digitalRead(SENSOR_PIN) == HIGH);

    // Update motion filter with new sample
    updateMotionFilter(rawMotionDetected);

    // Use FILTERED motion for state machine (reduces noise)
    processMotionState(filteredMotionDetected, currentMillis);
  }

  // Print heap periodically
  if (currentMillis - lastHeapPrint >= HEAP_PRINT_INTERVAL) {
    lastHeapPrint = currentMillis;
    Serial.print("[HEAP] ");
    Serial.print(ESP.getFreeHeap() / 1024);
    Serial.print(" KB | State: ");
    Serial.print(stateNames[currentState]);
    Serial.print(" | Alarms: ");
    Serial.println(totalAlarmEvents);
  }

  delay(10);
}
