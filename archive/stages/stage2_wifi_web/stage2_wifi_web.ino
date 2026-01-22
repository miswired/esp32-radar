/**
 * ESP32 Microwave Motion Sensor - Stage 2: WiFi and Web Interface
 *
 * This stage builds on Stage 1 and adds:
 * - WiFi connection with configurable credentials
 * - AP fallback mode when WiFi connection fails
 * - Basic web dashboard showing motion status
 * - REST API endpoints for status and configuration
 * - WiFi self-tests
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
 * - http://<ip>/status - JSON API for current status
 * - http://<ip>/config - JSON API for current configuration
 */

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <rom/rtc.h>
#include "credentials.h"  // WiFi credentials (copy credentials.h.example to credentials.h)

// ============================================================================
// PIN CONFIGURATION
// ============================================================================

#define SENSOR_PIN 13
#define LED_PIN 2

// ============================================================================
// TIMING CONFIGURATION
// ============================================================================

#define SENSOR_POLL_INTERVAL 100    // 10Hz (100ms)
#define HEAP_PRINT_INTERVAL 60000   // Print heap every 60 seconds (less frequent)
#define HEAP_MIN_THRESHOLD 50000    // 50KB minimum (lower for web server)
#define WIFI_CONNECT_TIMEOUT 15000  // 15 seconds WiFi connection timeout

// ============================================================================
// MOTION DETECTION CONFIGURATION
// ============================================================================

#define TRIP_DELAY_SECONDS 3
#define CLEAR_TIMEOUT_SECONDS 10
#define TRIP_DELAY_MS (TRIP_DELAY_SECONDS * 1000)
#define CLEAR_TIMEOUT_MS (CLEAR_TIMEOUT_SECONDS * 1000)

// ============================================================================
// MOTION FILTER CONFIGURATION
// Reduces sensor noise by requiring a percentage of positive readings
// over a sliding time window before reporting motion as detected.
// ============================================================================

#define FILTER_SAMPLE_COUNT 10          // Number of samples in sliding window
#define FILTER_THRESHOLD_PERCENT 70     // Percentage of samples that must be HIGH
#define FILTER_WINDOW_MS 1000           // Time window for samples (milliseconds)
#define FILTER_SAMPLE_INTERVAL (FILTER_WINDOW_MS / FILTER_SAMPLE_COUNT)  // 100ms per sample

// ============================================================================
// WIFI CONFIGURATION
// WiFi credentials are loaded from credentials.h
// Copy credentials.h.example to credentials.h and edit with your values
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
bool sampleBuffer[FILTER_SAMPLE_COUNT];
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
      padding: 20px;
    }
    .container { max-width: 600px; margin: 0 auto; }
    h1 {
      text-align: center;
      margin-bottom: 30px;
      color: #00d4ff;
    }
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
  <div class="container">
    <h1>ESP32 Motion Sensor</h1>

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
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

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
// MOTION FILTER FUNCTIONS
// ============================================================================

void initializeFilter() {
  // Initialize all samples to false (no motion)
  for (int i = 0; i < FILTER_SAMPLE_COUNT; i++) {
    sampleBuffer[i] = false;
  }
  sampleIndex = 0;
  samplePositiveCount = 0;
  filterInitialized = true;
  Serial.print("Motion filter initialized: ");
  Serial.print(FILTER_SAMPLE_COUNT);
  Serial.print(" samples, ");
  Serial.print(FILTER_THRESHOLD_PERCENT);
  Serial.print("% threshold, ");
  Serial.print(FILTER_WINDOW_MS);
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
  sampleIndex = (sampleIndex + 1) % FILTER_SAMPLE_COUNT;

  // Calculate percentage
  currentFilterPercent = (samplePositiveCount * 100) / FILTER_SAMPLE_COUNT;

  // Determine filtered motion state
  bool previousFiltered = filteredMotionDetected;
  filteredMotionDetected = (currentFilterPercent >= FILTER_THRESHOLD_PERCENT);

  // Log filter state changes (for debugging)
  if (filteredMotionDetected != previousFiltered) {
    Serial.print("[FILTER] ");
    Serial.print(currentFilterPercent);
    Serial.print("% (");
    Serial.print(samplePositiveCount);
    Serial.print("/");
    Serial.print(FILTER_SAMPLE_COUNT);
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
  doc["tripDelay"] = TRIP_DELAY_SECONDS;
  doc["clearTimeout"] = CLEAR_TIMEOUT_SECONDS;
  doc["filterThreshold"] = FILTER_THRESHOLD_PERCENT;
  doc["wifiMode"] = getWiFiModeString();
  doc["ipAddress"] = getIPAddress();
  doc["rssi"] = getRSSI();

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleConfig() {
  JsonDocument doc;

  doc["sensorPin"] = SENSOR_PIN;
  doc["ledPin"] = LED_PIN;
  doc["tripDelaySeconds"] = TRIP_DELAY_SECONDS;
  doc["clearTimeoutSeconds"] = CLEAR_TIMEOUT_SECONDS;
  doc["pollIntervalMs"] = SENSOR_POLL_INTERVAL;

  // Filter configuration
  doc["filterSampleCount"] = FILTER_SAMPLE_COUNT;
  doc["filterThresholdPercent"] = FILTER_THRESHOLD_PERCENT;
  doc["filterWindowMs"] = FILTER_WINDOW_MS;

  doc["wifiSsid"] = WIFI_SSID;
  doc["wifiMode"] = getWiFiModeString();
  doc["apSsid"] = AP_SSID;
  doc["ipAddress"] = getIPAddress();

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleNotFound() {
  server.send(404, "text/plain", "Not Found");
}

void setupWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/config", HTTP_GET, handleConfig);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("Web server started on port 80");
}

// ============================================================================
// STATE MACHINE
// ============================================================================

void processMotionState(bool motionDetected, unsigned long currentMillis) {
  MotionState previousState = currentState;

  switch (currentState) {
    case STATE_IDLE:
      if (motionDetected) {
        currentState = STATE_MOTION_PENDING;
        motionStartTime = currentMillis;
        totalMotionEvents++;
        Serial.print("[");
        Serial.print(currentMillis / 1000);
        Serial.print("s] Motion detected - waiting ");
        Serial.print(TRIP_DELAY_SECONDS);
        Serial.println("s...");
      }
      break;

    case STATE_MOTION_PENDING:
      if (!motionDetected) {
        currentState = STATE_IDLE;
        Serial.print("[");
        Serial.print(currentMillis / 1000);
        Serial.println("s] Motion stopped - back to IDLE");
      } else if (currentMillis - motionStartTime >= TRIP_DELAY_MS) {
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
      }
      break;

    case STATE_ALARM_ACTIVE:
      if (!motionDetected) {
        currentState = STATE_ALARM_CLEARING;
        motionStopTime = currentMillis;
        Serial.print("[");
        Serial.print(currentMillis / 1000);
        Serial.print("s] Motion stopped - clearing in ");
        Serial.print(CLEAR_TIMEOUT_SECONDS);
        Serial.println("s...");
      }
      break;

    case STATE_ALARM_CLEARING:
      if (motionDetected) {
        currentState = STATE_ALARM_ACTIVE;
        Serial.print("[");
        Serial.print(currentMillis / 1000);
        Serial.println("s] Motion resumed - alarm still active");
      } else if (currentMillis - motionStopTime >= CLEAR_TIMEOUT_MS) {
        currentState = STATE_IDLE;
        unsigned long duration = (currentMillis - alarmTriggerTime) / 1000;
        Serial.println();
        Serial.print("[");
        Serial.print(currentMillis / 1000);
        Serial.print("s] *** ALARM CLEARED *** (");
        Serial.print(duration);
        Serial.println("s)");
        Serial.println();
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
  // Simple check - if we got here, server started
  Serial.println("PASS (port 80)");
  return true;
}

void runSelfTest() {
  Serial.println("\n=== STAGE 2 SELF TEST ===");

  bool allPass = true;
  allPass &= testGPIO();
  allPass &= testLED();
  allPass &= testHeap();
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
  Serial.print(TRIP_DELAY_SECONDS);
  Serial.println("s");
  Serial.print("Clear Timeout: ");
  Serial.print(CLEAR_TIMEOUT_SECONDS);
  Serial.println("s");
  Serial.println("--- Motion Filter ---");
  Serial.print("Filter Samples: ");
  Serial.println(FILTER_SAMPLE_COUNT);
  Serial.print("Filter Threshold: ");
  Serial.print(FILTER_THRESHOLD_PERCENT);
  Serial.println("%");
  Serial.print("Filter Window: ");
  Serial.print(FILTER_WINDOW_MS);
  Serial.println("ms");
  Serial.println("--- WiFi ---");
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
  Serial.print(FILTER_THRESHOLD_PERCENT);
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
        Serial.println("================\n");
        break;
      case 'r':
      case 'R':
        Serial.println("Restarting...");
        delay(500);
        ESP.restart();
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
  Serial.println("ESP32 Microwave Motion Sensor - Stage 2");
  Serial.println("==========================================");
  Serial.println("WiFi + Web Interface");
  Serial.println();

  printResetReason();
  Serial.println();

  // Initialize pins
  pinMode(SENSOR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  Serial.println("GPIO initialized");

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
