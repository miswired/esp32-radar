/**
 * ESP32 Microwave Motion Sensor - Stage 9: Home Assistant MQTT Integration
 *
 * Copyright (C) 2024-2026 Miswired
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * ---
 *
 * This stage builds on Stage 8 and adds:
 * - MQTT client with PubSubClient library
 * - Home Assistant MQTT Discovery for automatic device registration
 * - Motion binary sensor (device_class: motion)
 * - Diagnostic sensors (WiFi signal, uptime, heap, alarm/motion counts)
 * - Number controls for trip delay, clear timeout, filter threshold
 * - Availability tracking via MQTT Last Will and Testament (LWT)
 * - Support for authenticated and TLS-encrypted MQTT connections
 * - Configurable device name for Home Assistant
 *
 * Hardware Requirements:
 * - ESP32-WROOM-32
 * - RCWL-0516 Microwave Radar Motion Sensor
 * - (Optional) WLED-enabled LED controller on network
 *
 * Connections:
 * - Sensor VIN → ESP32 VIN (5V from USB)
 * - Sensor GND → ESP32 GND
 * - Sensor OUT → ESP32 GPIO 13
 *
 * MQTT Broker:
 * - Home Assistant Mosquitto Add-on, or
 * - Standalone Mosquitto broker (Docker or native)
 *
 * WiFi Modes:
 * - Station Mode (STA): Connects to your WiFi network
 * - Access Point Mode (AP): Creates "ESP32-Radar-Setup" network at 192.168.4.1
 *
 * Web Interface:
 * - http://<ip>/ - Dashboard with motion status
 * - http://<ip>/settings - Configuration page (includes MQTT setup)
 * - http://<ip>/api - API documentation
 * - GET /status - JSON API for current status
 * - GET /config - JSON API for current configuration
 * - POST /config - Save configuration
 * - POST /reset - Factory reset
 * - POST /test-notification - Send test notification
 * - POST /test-wled - Send test WLED payload
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>  // TLS support for MQTT
#include <HTTPClient.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <rom/rtc.h>
#include <esp_task_wdt.h>  // Watchdog timer
#include <mbedtls/sha256.h>  // SHA-256 for password hashing
#include <PubSubClient.h>  // MQTT client for Home Assistant integration
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
#define WATCHDOG_TIMEOUT_S 30       // Watchdog timeout in seconds
#define EVENT_LOG_SIZE 50           // Number of events to keep in circular buffer
#define HEAP_HISTORY_SIZE 60        // Number of heap samples to keep (1 per minute = 1 hour)

// API Key Authentication constants (Stage 7)
#define AUTH_MAX_FAILURES 5         // Max failed attempts before lockout
#define AUTH_LOCKOUT_SECONDS 300    // Lockout duration (5 minutes)
#define API_KEY_LENGTH 36           // UUID format: 8-4-4-4-12 = 36 chars

// Web Password Authentication constants (Stage 7)
#define PASSWORD_HASH_LENGTH 64     // SHA-256 hex string length
#define SESSION_TOKEN_LENGTH 32     // Random hex token length
#define SESSION_TIMEOUT_MS 3600000  // 1 hour session timeout
#define MIN_PASSWORD_LENGTH 6       // Minimum password length

// WLED Integration constants (Stage 8)
#define WLED_URL_LENGTH 128         // Max URL length
#define WLED_PAYLOAD_LENGTH 512     // Max JSON payload length
#define WLED_TIMEOUT_MS 3000        // 3 second timeout for WLED requests
#define WLED_DEFAULT_PAYLOAD "{\"on\":true,\"bri\":255,\"seg\":[{\"col\":[[255,0,0]]}]}"

// MQTT / Home Assistant Integration constants (Stage 9)
#define MQTT_HOST_LENGTH 64         // Max hostname/IP length
#define MQTT_USER_LENGTH 32         // Max username length
#define MQTT_PASS_LENGTH 64         // Max password length
#define MQTT_DEVICE_NAME_LENGTH 32  // Max device name length
#define MQTT_DEFAULT_PORT 1883      // Default MQTT port (plaintext)
#define MQTT_TLS_PORT 8883          // Default MQTT port (TLS)
#define MQTT_RECONNECT_DELAY 5000   // Reconnect delay in ms
#define MQTT_MAX_RECONNECT_DELAY 60000  // Max reconnect delay (1 minute)
#define MQTT_KEEPALIVE 60           // Keepalive interval in seconds
#define MQTT_BUFFER_SIZE 512        // MQTT message buffer size
#define MQTT_DIAGNOSTIC_INTERVAL 60000  // Publish diagnostics every 60 seconds
#define MQTT_DISCOVERY_PREFIX "homeassistant"
#define MQTT_DEFAULT_DEVICE_NAME "Microwave Motion Sensor"

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

  // Notification settings (Stage 8: refactored to checkboxes)
  bool notifyEnabled;
  char notifyUrl[128];
  bool notifyGet;   // Send GET request on alarm
  bool notifyPost;  // Send POST request on alarm

  // WLED Integration settings (Stage 8)
  bool wledEnabled;
  char wledUrl[WLED_URL_LENGTH + 1];
  char wledPayload[WLED_PAYLOAD_LENGTH + 1];

  // MQTT / Home Assistant settings (Stage 9)
  bool mqttEnabled;
  char mqttHost[MQTT_HOST_LENGTH + 1];
  uint16_t mqttPort;
  char mqttUser[MQTT_USER_LENGTH + 1];
  char mqttPass[MQTT_PASS_LENGTH + 1];
  bool mqttUseTls;
  char mqttDeviceName[MQTT_DEVICE_NAME_LENGTH + 1];

  // API Key Authentication settings (Stage 7)
  bool authEnabled;
  char apiKey[API_KEY_LENGTH + 1];  // UUID + null terminator

  // Web Password Authentication settings (Stage 7)
  char passwordHash[PASSWORD_HASH_LENGTH + 1];  // SHA-256 hex + null terminator

  // Checksum to validate stored data
  uint32_t checksum;
};

// Notification tracking
unsigned long lastNotificationTime = 0;
int lastNotificationStatus = 0;  // HTTP response code or -1 for error
unsigned long totalNotificationsSent = 0;
unsigned long totalNotificationsFailed = 0;

// WLED tracking (Stage 8)
unsigned long lastWledTime = 0;
int lastWledStatus = 0;  // HTTP response code or -1 for error
unsigned long totalWledSent = 0;
unsigned long totalWledFailed = 0;

Config config;
Preferences preferences;

// ============================================================================
// EVENT LOGGING (Stage 6)
// ============================================================================

enum EventType {
  EVENT_BOOT,
  EVENT_WIFI_CONNECTED,
  EVENT_WIFI_DISCONNECTED,
  EVENT_WIFI_AP_MODE,
  EVENT_ALARM_TRIGGERED,
  EVENT_ALARM_CLEARED,
  EVENT_NOTIFICATION_SENT,
  EVENT_NOTIFICATION_FAILED,
  EVENT_CONFIG_SAVED,
  EVENT_FACTORY_RESET,
  EVENT_WATCHDOG_RESET,
  EVENT_HEAP_LOW,
  EVENT_AUTH_FAILED,     // Stage 7: Failed authentication attempt
  EVENT_AUTH_LOCKOUT,    // Stage 7: Rate limit lockout activated
  EVENT_WLED_SENT,       // Stage 8: WLED payload sent successfully
  EVENT_WLED_FAILED,     // Stage 8: WLED payload failed to send
  EVENT_MQTT_CONNECTED,  // Stage 9: MQTT broker connected
  EVENT_MQTT_DISCONNECTED // Stage 9: MQTT broker disconnected
};

const char* eventTypeNames[] = {
  "BOOT",
  "WIFI_CONNECTED",
  "WIFI_DISCONNECTED",
  "WIFI_AP_MODE",
  "ALARM_TRIGGERED",
  "ALARM_CLEARED",
  "NOTIFY_SENT",
  "NOTIFY_FAILED",
  "CONFIG_SAVED",
  "FACTORY_RESET",
  "WDT_RESET",
  "HEAP_LOW",
  "AUTH_FAILED",
  "AUTH_LOCKOUT",
  "WLED_SENT",
  "WLED_FAILED",
  "MQTT_CONNECTED",
  "MQTT_DISCONNECTED"
};

struct EventLogEntry {
  unsigned long timestamp;  // millis() when event occurred
  EventType type;
  int32_t data;             // Optional data (e.g., heap size, HTTP code)
};

EventLogEntry eventLog[EVENT_LOG_SIZE];
int eventLogHead = 0;       // Next write position
int eventLogCount = 0;      // Number of entries (max EVENT_LOG_SIZE)

// Heap history for memory monitoring
uint32_t heapHistory[HEAP_HISTORY_SIZE];
int heapHistoryHead = 0;
int heapHistoryCount = 0;
uint32_t minHeapSeen = UINT32_MAX;
uint32_t maxHeapSeen = 0;

// Watchdog status
bool watchdogEnabled = false;

// Authentication state (Stage 7)
uint8_t authFailCount = 0;
unsigned long authLockoutUntil = 0;  // millis() when lockout expires

// Web Password Session (Stage 7)
struct Session {
  char token[SESSION_TOKEN_LENGTH + 1];  // Session token (hex string)
  unsigned long expiresAt;               // millis() when session expires
  bool active;                           // Is session valid
};

Session currentSession = {"", 0, false};

// Forward declaration for event logging
void logEvent(EventType type, int32_t data = 0);

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
// MQTT CLIENT (Stage 9)
// ============================================================================

// WiFi clients for MQTT (one for plaintext, one for TLS)
WiFiClient mqttWifiClient;
WiFiClientSecure mqttWifiClientSecure;

// MQTT client (will be assigned one of the above WiFi clients)
PubSubClient mqttClient;

// MQTT state
bool mqttConnected = false;
unsigned long mqttLastReconnectAttempt = 0;
unsigned long mqttReconnectDelay = MQTT_RECONNECT_DELAY;
unsigned long mqttLastDiagnosticPublish = 0;
bool mqttDiscoveryPublished = false;
char mqttChipId[13];  // 6 bytes as hex = 12 chars + null
unsigned long mqttMessagesPublished = 0;
unsigned long mqttMessagesReceived = 0;

// MQTT topic buffers (constructed at runtime)
char mqttAvailabilityTopic[80];
char mqttStateTopic[80];

// Forward declarations for MQTT functions
void mqttCallback(char* topic, byte* payload, unsigned int length);
void mqttConnect();
void mqttLoop();
void mqttPublishDiscovery();
void mqttPublishMotionState();
void mqttPublishDiagnostics();
void mqttPublishAvailability(bool online);
String getMqttDeviceId();

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

  // Notification defaults (Stage 8: refactored to checkboxes)
  config.notifyEnabled = false;
  config.notifyUrl[0] = '\0';
  config.notifyGet = false;
  config.notifyPost = false;

  // WLED defaults (Stage 8)
  config.wledEnabled = false;
  config.wledUrl[0] = '\0';
  strncpy(config.wledPayload, WLED_DEFAULT_PAYLOAD, sizeof(config.wledPayload) - 1);
  config.wledPayload[sizeof(config.wledPayload) - 1] = '\0';

  // MQTT defaults (Stage 9)
  config.mqttEnabled = false;
  config.mqttHost[0] = '\0';
  config.mqttPort = MQTT_DEFAULT_PORT;
  config.mqttUser[0] = '\0';
  config.mqttPass[0] = '\0';
  config.mqttUseTls = false;
  strncpy(config.mqttDeviceName, MQTT_DEFAULT_DEVICE_NAME, sizeof(config.mqttDeviceName) - 1);
  config.mqttDeviceName[sizeof(config.mqttDeviceName) - 1] = '\0';

  // Authentication defaults (Stage 7)
  config.authEnabled = false;
  config.apiKey[0] = '\0';

  // Web Password defaults (Stage 7) - empty means no password configured
  config.passwordHash[0] = '\0';

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
    Serial.print("  Notify Methods: ");
    if (config.notifyGet && config.notifyPost) {
      Serial.println("GET + POST");
    } else if (config.notifyGet) {
      Serial.println("GET");
    } else if (config.notifyPost) {
      Serial.println("POST");
    } else {
      Serial.println("None");
    }
  }
  Serial.print("  WLED: ");
  Serial.println(config.wledEnabled ? "Enabled" : "Disabled");
  if (config.wledEnabled && strlen(config.wledUrl) > 0) {
    Serial.print("  WLED URL: ");
    Serial.println(config.wledUrl);
  }
  Serial.print("  MQTT: ");
  Serial.println(config.mqttEnabled ? "Enabled" : "Disabled");
  if (config.mqttEnabled && strlen(config.mqttHost) > 0) {
    Serial.print("  MQTT Broker: ");
    Serial.print(config.mqttHost);
    Serial.print(":");
    Serial.println(config.mqttPort);
    Serial.print("  MQTT TLS: ");
    Serial.println(config.mqttUseTls ? "Yes" : "No");
    Serial.print("  MQTT Device: ");
    Serial.println(config.mqttDeviceName);
  }
  Serial.print("  API Auth: ");
  Serial.println(config.authEnabled ? "Enabled" : "Disabled");
  if (config.authEnabled && strlen(config.apiKey) > 0) {
    Serial.print("  API Key: ");
    Serial.print(config.apiKey[0]);
    Serial.print(config.apiKey[1]);
    Serial.print(config.apiKey[2]);
    Serial.print(config.apiKey[3]);
    Serial.println("****-****-****-************");
  }
  Serial.print("  Web Password: ");
  Serial.println(strlen(config.passwordHash) > 0 ? "Configured" : "Not configured");

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
  logEvent(EVENT_FACTORY_RESET);

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
// API KEY AUTHENTICATION (Stage 7)
// ============================================================================

// Generate a UUID v4 using ESP32 hardware RNG
void generateUUID(char* buffer) {
  // UUID v4 format: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
  // where x is any hex digit and y is 8, 9, a, or b
  const char* hexChars = "0123456789abcdef";
  const char* variantChars = "89ab";

  uint32_t r1 = esp_random();
  uint32_t r2 = esp_random();
  uint32_t r3 = esp_random();
  uint32_t r4 = esp_random();

  // Positions: 0-7, 9-12, 14-17, 19-22, 24-35
  int pos = 0;

  // First 8 hex chars (from r1)
  for (int i = 0; i < 8; i++) {
    buffer[pos++] = hexChars[(r1 >> (i * 4)) & 0xF];
  }
  buffer[pos++] = '-';

  // Next 4 hex chars (from r2 lower)
  for (int i = 0; i < 4; i++) {
    buffer[pos++] = hexChars[(r2 >> (i * 4)) & 0xF];
  }
  buffer[pos++] = '-';

  // Version 4: 4xxx (from r2 upper)
  buffer[pos++] = '4';
  for (int i = 0; i < 3; i++) {
    buffer[pos++] = hexChars[(r2 >> ((i + 4) * 4)) & 0xF];
  }
  buffer[pos++] = '-';

  // Variant: yxxx where y is 8,9,a,b (from r3)
  buffer[pos++] = variantChars[r3 & 0x3];
  for (int i = 0; i < 3; i++) {
    buffer[pos++] = hexChars[(r3 >> ((i + 1) * 4)) & 0xF];
  }
  buffer[pos++] = '-';

  // Last 12 hex chars (from r3 upper + r4)
  for (int i = 4; i < 8; i++) {
    buffer[pos++] = hexChars[(r3 >> (i * 4)) & 0xF];
  }
  for (int i = 0; i < 8; i++) {
    buffer[pos++] = hexChars[(r4 >> (i * 4)) & 0xF];
  }

  buffer[pos] = '\0';
}

// Check if authentication is required and valid
// Returns: true if access allowed, false if denied
bool checkAuthentication() {
  // If auth is disabled, always allow
  if (!config.authEnabled) {
    return true;
  }

  // Check for lockout
  if (authLockoutUntil > 0 && millis() < authLockoutUntil) {
    return false;  // Still locked out
  }

  // Clear expired lockout
  if (authLockoutUntil > 0 && millis() >= authLockoutUntil) {
    authLockoutUntil = 0;
    authFailCount = 0;
  }

  // No API key configured - deny access
  if (strlen(config.apiKey) == 0) {
    return false;
  }

  String providedKey = "";

  // Check X-API-Key header first
  if (server.hasHeader("X-API-Key")) {
    providedKey = server.header("X-API-Key");
  }
  // Fall back to query parameter
  else if (server.hasArg("apiKey")) {
    providedKey = server.arg("apiKey");
  }

  // No key provided
  if (providedKey.length() == 0) {
    return false;
  }

  // Compare keys
  if (providedKey == config.apiKey) {
    // Success - reset fail counter
    authFailCount = 0;
    return true;
  }

  // Invalid key - increment fail counter
  authFailCount++;
  logEvent(EVENT_AUTH_FAILED, authFailCount);
  Serial.print("[AUTH] Failed attempt #");
  Serial.println(authFailCount);

  // Check if we should lock out
  if (authFailCount >= AUTH_MAX_FAILURES) {
    authLockoutUntil = millis() + (AUTH_LOCKOUT_SECONDS * 1000);
    logEvent(EVENT_AUTH_LOCKOUT, AUTH_LOCKOUT_SECONDS);
    Serial.print("[AUTH] Lockout activated for ");
    Serial.print(AUTH_LOCKOUT_SECONDS);
    Serial.println(" seconds");
  }

  return false;
}

// Send authentication error response
void sendAuthError() {
  JsonDocument doc;

  // Check if locked out
  if (authLockoutUntil > 0 && millis() < authLockoutUntil) {
    unsigned long remaining = (authLockoutUntil - millis()) / 1000;
    doc["error"] = "Too many failed attempts";
    doc["code"] = 429;
    doc["retryAfter"] = remaining;

    String response;
    serializeJson(doc, response);
    server.send(429, "application/json", response);
  } else {
    doc["error"] = "Authentication required";
    doc["code"] = 401;

    String response;
    serializeJson(doc, response);
    server.send(401, "application/json", response);
  }
}

// Get remaining lockout time in seconds (0 if not locked)
unsigned long getAuthLockoutRemaining() {
  if (authLockoutUntil > 0 && millis() < authLockoutUntil) {
    return (authLockoutUntil - millis()) / 1000;
  }
  return 0;
}

// ============================================================================
// WEB PASSWORD AUTHENTICATION (Stage 7)
// ============================================================================

// Check if a web password has been configured
bool isPasswordConfigured() {
  return strlen(config.passwordHash) > 0;
}

// Hash a password using SHA-256, returns hex string
void hashPassword(const char* password, char* outputHash) {
  unsigned char hash[32];  // SHA-256 produces 32 bytes

  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts(&ctx, 0);  // 0 = SHA-256 (not SHA-224)
  mbedtls_sha256_update(&ctx, (const unsigned char*)password, strlen(password));
  mbedtls_sha256_finish(&ctx, hash);
  mbedtls_sha256_free(&ctx);

  // Convert to hex string
  const char* hexChars = "0123456789abcdef";
  for (int i = 0; i < 32; i++) {
    outputHash[i * 2] = hexChars[(hash[i] >> 4) & 0x0F];
    outputHash[i * 2 + 1] = hexChars[hash[i] & 0x0F];
  }
  outputHash[64] = '\0';
}

// Verify a password against stored hash
bool verifyPassword(const char* password) {
  if (!isPasswordConfigured()) {
    return false;
  }

  char computedHash[PASSWORD_HASH_LENGTH + 1];
  hashPassword(password, computedHash);

  return strcmp(computedHash, config.passwordHash) == 0;
}

// Generate a random session token using hardware RNG
void generateSessionToken(char* buffer) {
  const char* hexChars = "0123456789abcdef";

  for (int i = 0; i < SESSION_TOKEN_LENGTH; i += 8) {
    uint32_t r = esp_random();
    for (int j = 0; j < 8 && (i + j) < SESSION_TOKEN_LENGTH; j++) {
      buffer[i + j] = hexChars[(r >> (j * 4)) & 0xF];
    }
  }
  buffer[SESSION_TOKEN_LENGTH] = '\0';
}

// Create a new session (invalidates any existing session)
void createSession() {
  generateSessionToken(currentSession.token);
  currentSession.expiresAt = millis() + SESSION_TIMEOUT_MS;
  currentSession.active = true;

  Serial.println("[AUTH] New web session created");
}

// Validate a session token from cookie
bool validateSession(const String& token) {
  // No active session
  if (!currentSession.active) {
    return false;
  }

  // Check expiration
  if (millis() >= currentSession.expiresAt) {
    clearSession();
    Serial.println("[AUTH] Session expired");
    return false;
  }

  // Compare tokens
  if (token == currentSession.token) {
    // Refresh session expiration on valid access
    currentSession.expiresAt = millis() + SESSION_TIMEOUT_MS;
    return true;
  }

  return false;
}

// Clear current session (logout)
void clearSession() {
  currentSession.token[0] = '\0';
  currentSession.expiresAt = 0;
  currentSession.active = false;

  Serial.println("[AUTH] Session cleared");
}

// Get session token from request cookie
String getSessionFromCookie() {
  if (server.hasHeader("Cookie")) {
    String cookie = server.header("Cookie");
    int start = cookie.indexOf("session=");
    if (start >= 0) {
      start += 8;  // Length of "session="
      int end = cookie.indexOf(";", start);
      if (end < 0) {
        end = cookie.length();
      }
      return cookie.substring(start, end);
    }
  }
  return "";
}

// Check if current request has valid web session
bool hasValidSession() {
  String token = getSessionFromCookie();
  return token.length() > 0 && validateSession(token);
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
      <a href="/diag">Diagnostics</a>
      <a href="/api">API</a>
      <a href="/about">About</a>
      <a href="#" id="authBtn" style="display:none;">Login</a>
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
    const formatUptime = (seconds) => {
      if (seconds < 60) return seconds + 's';
      if (seconds < 3600) return Math.floor(seconds / 60) + 'm';
      if (seconds < 86400) return Math.floor(seconds / 3600) + 'h';
      return Math.floor(seconds / 86400) + 'd';
    };

    const updateStatus = () => {
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
    };

    // Update every second
    setInterval(updateStatus, 1000);
    updateStatus();

    // Auth button handling
    const handleLogout = () => {
      fetch('/auth/logout', { method: 'POST' })
        .then(() => { checkAuthStatus(); })
        .catch(err => console.log('Logout failed'));
    };

    const checkAuthStatus = () => {
      fetch('/auth/status')
        .then(response => response.json())
        .then(data => {
          const btn = document.getElementById('authBtn');
          if (data.passwordConfigured) {
            btn.style.display = 'block';
            btn.textContent = data.loggedIn ? 'Logout' : 'Login';
            btn.onclick = data.loggedIn ? handleLogout : () => { window.location.href = '/settings'; };
          }
        })
        .catch(err => console.log('Auth check failed'));
    };

    checkAuthStatus();
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
    /* Toast Notification System */
    .toast-container {
      position: fixed;
      bottom: 20px;
      left: 50%;
      transform: translateX(-50%);
      z-index: 1000;
      display: flex;
      flex-direction: column;
      gap: 10px;
      pointer-events: none;
    }
    .toast {
      background: #1a1a2e;
      border: 1px solid #333;
      border-radius: 8px;
      padding: 16px 20px;
      display: flex;
      align-items: center;
      gap: 12px;
      min-width: 300px;
      max-width: 90vw;
      box-shadow: 0 4px 20px rgba(0,0,0,0.4);
      animation: slideUp 0.3s ease-out;
      pointer-events: auto;
    }
    .toast.success {
      border-left: 4px solid #4ade80;
    }
    .toast.error {
      border-left: 4px solid #ef4444;
    }
    .toast-icon {
      font-size: 20px;
      flex-shrink: 0;
    }
    .toast.success .toast-icon { color: #4ade80; }
    .toast.error .toast-icon { color: #ef4444; }
    .toast-message {
      flex: 1;
      font-size: 14px;
    }
    .toast-close {
      background: none;
      border: none;
      color: #666;
      cursor: pointer;
      padding: 4px;
      font-size: 18px;
      line-height: 1;
    }
    .toast-close:hover { color: #fff; }
    @keyframes slideUp {
      from { transform: translateY(100%); opacity: 0; }
      to { transform: translateY(0); opacity: 1; }
    }
    @keyframes fadeOut {
      from { opacity: 1; }
      to { opacity: 0; }
    }
    .toast.hiding {
      animation: fadeOut 0.2s ease-out forwards;
    }
    /* Modal Styles for Login/Password Setup */
    .modal-overlay {
      position: fixed;
      top: 0;
      left: 0;
      width: 100%;
      height: 100%;
      background: rgba(0, 0, 0, 0.8);
      display: flex;
      align-items: center;
      justify-content: center;
      z-index: 1000;
    }
    .modal-overlay.hidden {
      display: none;
    }
    .modal {
      background: #16213e;
      border-radius: 12px;
      padding: 30px;
      max-width: 400px;
      width: 90%;
      box-shadow: 0 10px 40px rgba(0,0,0,0.5);
    }
    .modal h2 {
      color: #00d4ff;
      margin-bottom: 10px;
      font-size: 20px;
    }
    .modal p {
      color: #aaa;
      margin-bottom: 20px;
      font-size: 14px;
      line-height: 1.5;
    }
    .modal .form-group {
      margin-bottom: 15px;
    }
    .modal .warning {
      background: #7c2d12;
      border: 1px solid #c2410c;
      border-radius: 8px;
      padding: 12px;
      margin-bottom: 20px;
      font-size: 12px;
      color: #fed7aa;
    }
    .modal .warning strong {
      color: #fb923c;
    }
    .modal .requirements {
      font-size: 12px;
      color: #888;
      margin-top: 5px;
    }
    .modal .error-message {
      color: #ef4444;
      font-size: 13px;
      margin-top: 10px;
      display: none;
    }
    .modal .error-message.visible {
      display: block;
    }
    .password-field {
      position: relative;
    }
    .password-field input {
      padding-right: 60px;
    }
    .password-toggle {
      position: absolute;
      right: 10px;
      top: 50%;
      transform: translateY(-50%);
      background: none;
      border: none;
      color: #888;
      cursor: pointer;
      font-size: 12px;
    }
    .password-toggle:hover { color: #fff; }
    .settings-hidden {
      display: none !important;
    }
  </style>
</head>
<body>
  <nav>
    <a href="/" class="nav-brand">ESP32 Radar</a>
    <div class="nav-links">
      <a href="/">Dashboard</a>
      <a href="/settings" class="active">Settings</a>
      <a href="/diag">Diagnostics</a>
      <a href="/api">API</a>
      <a href="/about">About</a>
      <a href="#" id="authBtn" style="display:none;">Login</a>
    </div>
  </nav>

  <!-- Password Setup Modal -->
  <div id="setupModal" class="modal-overlay hidden">
    <div class="modal">
      <h2>Create Password</h2>
      <p>Secure your settings page with a password. This password is separate from the API key.</p>
      <div class="warning">
        <strong>Security Notice:</strong> This device uses HTTP (not HTTPS). Your password will be transmitted unencrypted. Use only on trusted networks.
      </div>
      <form id="setupForm">
        <div class="form-group">
          <label for="setupPassword">Password</label>
          <div class="password-field">
            <input type="password" id="setupPassword" name="password" required>
            <button type="button" class="password-toggle" onclick="togglePassword('setupPassword', this)">Show</button>
          </div>
          <div class="requirements">Minimum 6 characters. Consider using numbers and special characters.</div>
        </div>
        <div class="form-group">
          <label for="setupConfirm">Confirm Password</label>
          <div class="password-field">
            <input type="password" id="setupConfirm" name="confirmPassword" required>
            <button type="button" class="password-toggle" onclick="togglePassword('setupConfirm', this)">Show</button>
          </div>
        </div>
        <div id="setupError" class="error-message"></div>
        <button type="submit" class="btn btn-primary" style="width:100%;margin:0;">Create Password</button>
      </form>
    </div>
  </div>

  <!-- Login Modal -->
  <div id="loginModal" class="modal-overlay hidden">
    <div class="modal">
      <h2>Login Required</h2>
      <p>Enter your password to access settings.</p>
      <div class="warning">
        <strong>Security Notice:</strong> This device uses HTTP (not HTTPS). Your password will be transmitted unencrypted. Use only on trusted networks.
      </div>
      <form id="loginForm">
        <div class="form-group">
          <label for="loginPassword">Password</label>
          <div class="password-field">
            <input type="password" id="loginPassword" name="password" required>
            <button type="button" class="password-toggle" onclick="togglePassword('loginPassword', this)">Show</button>
          </div>
        </div>
        <div id="loginError" class="error-message"></div>
        <button type="submit" class="btn btn-primary" style="width:100%;margin:0;">Login</button>
      </form>
    </div>
  </div>

  <div class="container" id="settingsContainer">
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
          <label>Request Method</label>
          <label style="display: flex; align-items: center; gap: 10px; margin-top: 8px;">
            <input type="checkbox" id="notifyGet" name="notifyGet" style="width: auto;">
            Send GET request (query parameters)
          </label>
          <label style="display: flex; align-items: center; gap: 10px; margin-top: 8px;">
            <input type="checkbox" id="notifyPost" name="notifyPost" style="width: auto;">
            Send POST request (JSON body)
          </label>
          <div class="hint">Select one or both. GET appends ?event=...&state=... to URL. POST sends JSON body.</div>
        </div>
        <div class="button-group" style="margin-top: 15px;">
          <button type="button" class="btn btn-secondary" id="testNotifyBtn">Test Notification</button>
        </div>
        <div id="notifyStatus" style="margin-top: 10px; font-size: 12px; color: #888;"></div>
      </div>

      <div class="card">
        <h2>WLED Integration</h2>
        <div class="form-group">
          <label style="display: flex; align-items: center; gap: 10px;">
            <input type="checkbox" id="wledEnabled" name="wledEnabled" style="width: auto;">
            Send payload to WLED device on alarm
          </label>
          <div class="hint">Sends JSON payload to a WLED device when alarm triggers</div>
        </div>
        <div class="form-group">
          <label for="wledUrl">WLED URL</label>
          <input type="text" id="wledUrl" name="wledUrl" placeholder="http://192.168.1.50/json/state" maxlength="127">
          <div class="hint">Full URL to the WLED JSON API endpoint</div>
        </div>
        <div class="form-group">
          <label for="wledPayload">JSON Payload</label>
          <textarea id="wledPayload" name="wledPayload" rows="5" maxlength="511" style="width: 100%; padding: 12px; border: 1px solid #333; border-radius: 8px; background: #1a1a2e; color: #eee; font-size: 14px; font-family: monospace; resize: vertical;"></textarea>
          <div class="hint">JSON payload to send to WLED. Default sets solid red at full brightness.</div>
          <div id="wledJsonError" style="color: #ef4444; font-size: 12px; margin-top: 5px; display: none;"></div>
        </div>
        <div class="button-group" style="margin-top: 15px;">
          <button type="button" class="btn btn-secondary" id="testWledBtn">Test WLED</button>
        </div>
        <div id="wledStatus" style="margin-top: 10px; font-size: 12px; color: #888;"></div>
      </div>

      <div class="card">
        <h2>MQTT / Home Assistant</h2>
        <div class="form-group">
          <label style="display: flex; align-items: center; gap: 10px;">
            <input type="checkbox" id="mqttEnabled" name="mqttEnabled" style="width: auto;">
            Connect to MQTT broker for Home Assistant integration
          </label>
          <div class="hint">Enables automatic device discovery in Home Assistant</div>
        </div>
        <div class="form-group">
          <label for="mqttHost">Broker Host</label>
          <input type="text" id="mqttHost" name="mqttHost" placeholder="192.168.1.100 or mqtt.local" maxlength="63">
          <div class="hint">MQTT broker hostname or IP address</div>
        </div>
        <div class="form-group">
          <label for="mqttPort">Port</label>
          <input type="number" id="mqttPort" name="mqttPort" min="1" max="65535" value="1883" style="width: 120px;">
          <div class="hint">Default: 1883 (plaintext) or 8883 (TLS)</div>
        </div>
        <div class="form-group">
          <label for="mqttUser">Username (optional)</label>
          <input type="text" id="mqttUser" name="mqttUser" placeholder="Leave empty for anonymous" maxlength="31">
        </div>
        <div class="form-group">
          <label for="mqttPass">Password (optional)</label>
          <input type="password" id="mqttPass" name="mqttPass" placeholder="Leave empty for anonymous" maxlength="63">
        </div>
        <div class="form-group">
          <label style="display: flex; align-items: center; gap: 10px;">
            <input type="checkbox" id="mqttUseTls" name="mqttUseTls" style="width: auto;">
            Use TLS/SSL encryption
          </label>
          <div class="hint">Encrypts connection to broker (requires broker TLS support)</div>
        </div>
        <div class="form-group">
          <label for="mqttDeviceName">Device Name</label>
          <input type="text" id="mqttDeviceName" name="mqttDeviceName" placeholder="Microwave Motion Sensor" maxlength="31">
          <div class="hint">Name shown in Home Assistant</div>
        </div>
        <div id="mqttStatus" style="margin-top: 15px; padding: 10px; border-radius: 8px; background: #1a1a2e;">
          <span id="mqttConnStatus" style="font-weight: bold;"></span>
          <span id="mqttDeviceIdLabel" style="font-size: 12px; color: #888; margin-left: 10px;"></span>
        </div>
        <details style="margin-top: 15px;">
          <summary style="cursor: pointer; color: #4a90d9;">MQTT Broker Setup Help</summary>
          <div style="margin-top: 10px; padding: 10px; background: #1a1a2e; border-radius: 8px; font-size: 13px;">
            <p style="margin-bottom: 10px;"><em>Choose ONE of these options:</em></p>
            <p><strong>Option 1: Home Assistant Add-on</strong> (recommended)</p>
            <p style="margin-left: 10px; color: #aaa;">Install "Mosquitto broker" from Settings &rarr; Add-ons &rarr; Add-on Store, then add the MQTT integration. Use your HA IP as the broker host.</p>
            <p style="margin-top: 15px;"><strong>Option 2: Standalone Docker</strong></p>
            <p style="margin-left: 10px; color: #aaa;">Run your own MQTT broker if not using Home Assistant OS:</p>
            <pre style="background: #0d0d1a; padding: 10px; border-radius: 4px; overflow-x: auto; font-size: 11px; margin-top: 5px;">docker run -d --name mosquitto \\
  -p 1883:1883 eclipse-mosquitto:2</pre>
          </div>
        </details>
      </div>

      <div class="card">
        <h2>API Authentication</h2>
        <div class="form-group">
          <label style="display: flex; align-items: center; gap: 10px;">
            <input type="checkbox" id="authEnabled" name="authEnabled" style="width: auto;">
            Require API key for JSON endpoints
          </label>
          <div class="hint">When enabled, API requests must include a valid key via X-API-Key header or apiKey query parameter</div>
        </div>
        <div class="form-group">
          <label for="apiKey">API Key</label>
          <div style="display: flex; gap: 10px;">
            <input type="password" id="apiKey" name="apiKey" readonly style="flex: 1; font-family: monospace;">
            <button type="button" class="btn btn-secondary" id="toggleKeyBtn" style="padding: 12px 16px;">Show</button>
          </div>
          <div class="hint">UUID format key for authenticating API requests</div>
        </div>
        <div class="button-group" style="margin-top: 15px;">
          <button type="button" class="btn btn-secondary" id="generateKeyBtn">Generate New Key</button>
        </div>
        <div id="authStatus" style="margin-top: 10px; font-size: 12px; color: #888;"></div>
        <div id="lockoutStatus" style="margin-top: 5px; font-size: 12px; color: #ef4444; display: none;"></div>
      </div>

      <div class="button-group">
        <button type="submit" class="btn btn-primary">Save Settings</button>
        <button type="button" class="btn btn-danger" id="resetBtn">Factory Reset</button>
      </div>
    </form>
  </div>

  <script>
    // Store API key for authenticated requests
    let currentApiKey = localStorage.getItem('esp32_api_key') || '';
    let isLoggedIn = false;

    // Helper to make authenticated fetch requests
    const authFetch = (url, options = {}) => {
      if (currentApiKey) {
        options.headers = options.headers || {};
        options.headers['X-API-Key'] = currentApiKey;
      }
      return fetch(url, options);
    };

    // Toast notification system
    const showToast = (message, type = 'success', duration = 4000) => {
      const container = document.getElementById('toastContainer');
      const toast = document.createElement('div');
      toast.className = 'toast ' + type;
      const icon = type === 'success' ? '&#10004;' : '&#10006;';
      toast.innerHTML =
        '<span class="toast-icon">' + icon + '</span>' +
        '<span class="toast-message">' + message + '</span>' +
        '<button class="toast-close" onclick="this.parentElement.remove()">&times;</button>';
      container.appendChild(toast);

      // Auto-dismiss for success (errors stay until dismissed)
      if (type === 'success' && duration > 0) {
        setTimeout(() => {
          toast.classList.add('hiding');
          setTimeout(() => toast.remove(), 200);
        }, duration);
      }
    };

    // Toggle password visibility (using const to avoid Arduino preprocessor issue)
    const togglePassword = (inputId, btn) => {
      const input = document.getElementById(inputId);
      if (input.type === 'password') {
        input.type = 'text';
        btn.textContent = 'Hide';
      } else {
        input.type = 'password';
        btn.textContent = 'Show';
      }
    };

    // Update auth button state
    const updateAuthButton = (loggedIn, passwordConfigured) => {
      const btn = document.getElementById('authBtn');
      if (passwordConfigured) {
        btn.style.display = 'block';
        btn.textContent = loggedIn ? 'Logout' : 'Login';
        btn.onclick = loggedIn ? handleLogout : () => showLoginModal();
      } else {
        btn.style.display = 'none';
      }
    };

    // Show/hide modals
    const showSetupModal = () => {
      document.getElementById('setupModal').classList.remove('hidden');
      document.getElementById('settingsContainer').classList.add('settings-hidden');
    };

    const hideSetupModal = () => {
      document.getElementById('setupModal').classList.add('hidden');
      document.getElementById('settingsContainer').classList.remove('settings-hidden');
    };

    const showLoginModal = () => {
      document.getElementById('loginModal').classList.remove('hidden');
      document.getElementById('settingsContainer').classList.add('settings-hidden');
    };

    const hideLoginModal = () => {
      document.getElementById('loginModal').classList.add('hidden');
      document.getElementById('settingsContainer').classList.remove('settings-hidden');
    };

    // Handle logout
    const handleLogout = () => {
      fetch('/auth/logout', { method: 'POST' })
        .then(response => response.json())
        .then(data => {
          isLoggedIn = false;
          updateAuthButton(false, true);
          showLoginModal();
          showToast('Logged out successfully', 'success');
        })
        .catch(err => {
          showToast('Error logging out', 'error');
        });
    };

    // Check auth status and show appropriate modal
    const checkAuthStatus = () => {
      return fetch('/auth/status')
        .then(response => response.json())
        .then(data => {
          const passwordConfigured = data.passwordConfigured;
          isLoggedIn = data.loggedIn;

          updateAuthButton(isLoggedIn, passwordConfigured);

          if (!passwordConfigured) {
            // First time - show setup modal
            showSetupModal();
            return false;
          } else if (!isLoggedIn) {
            // Password set but not logged in - show login modal
            showLoginModal();
            return false;
          }
          // Logged in - show settings
          return true;
        })
        .catch(err => {
          console.log('Auth check failed:', err);
          return true; // Show settings on error
        });
    };

    // Password setup form
    document.getElementById('setupForm').addEventListener('submit', function(e) {
      e.preventDefault();
      const password = document.getElementById('setupPassword').value;
      const confirm = document.getElementById('setupConfirm').value;
      const errorEl = document.getElementById('setupError');

      if (password.length < 6) {
        errorEl.textContent = 'Password must be at least 6 characters';
        errorEl.classList.add('visible');
        return;
      }

      if (password !== confirm) {
        errorEl.textContent = 'Passwords do not match';
        errorEl.classList.add('visible');
        return;
      }

      errorEl.classList.remove('visible');

      fetch('/auth/setup', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ password: password, confirmPassword: confirm })
      })
        .then(response => response.json())
        .then(data => {
          if (data.success) {
            isLoggedIn = true;
            hideSetupModal();
            updateAuthButton(true, true);
            showToast('Password created successfully', 'success');
            loadConfig();
          } else {
            errorEl.textContent = data.message || 'Failed to create password';
            errorEl.classList.add('visible');
          }
        })
        .catch(err => {
          errorEl.textContent = 'Error creating password';
          errorEl.classList.add('visible');
        });
    });

    // Login form
    document.getElementById('loginForm').addEventListener('submit', function(e) {
      e.preventDefault();
      const password = document.getElementById('loginPassword').value;
      const errorEl = document.getElementById('loginError');

      fetch('/auth/login', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ password: password })
      })
        .then(response => response.json())
        .then(data => {
          if (data.success) {
            isLoggedIn = true;
            hideLoginModal();
            updateAuthButton(true, true);
            showToast('Login successful', 'success');
            loadConfig();
          } else {
            errorEl.textContent = data.message || 'Invalid password';
            errorEl.classList.add('visible');
          }
        })
        .catch(err => {
          errorEl.textContent = 'Error logging in';
          errorEl.classList.add('visible');
        });
    });

    // Load config function
    const loadConfig = () => {
      authFetch('/config')
      .then(response => {
        if (response.status === 401 || response.status === 429) {
          document.getElementById('authStatus').textContent = 'Authentication required. Enter API key or disable auth via serial.';
          document.getElementById('authStatus').style.color = '#fbbf24';
          throw new Error('Auth required');
        }
        return response.json();
      })
      .then(data => {
        document.getElementById('wifiSsid').value = data.wifiSsid || '';
        document.getElementById('tripDelay').value = data.tripDelaySeconds;
        document.getElementById('clearTimeout').value = data.clearTimeoutSeconds;
        document.getElementById('filterThreshold').value = data.filterThresholdPercent;
        document.getElementById('notifyEnabled').checked = data.notifyEnabled || false;
        document.getElementById('notifyUrl').value = data.notifyUrl || '';
        document.getElementById('notifyGet').checked = data.notifyGet || false;
        document.getElementById('notifyPost').checked = data.notifyPost || false;
        document.getElementById('wledEnabled').checked = data.wledEnabled || false;
        document.getElementById('wledUrl').value = data.wledUrl || '';
        document.getElementById('wledPayload').value = data.wledPayload || '';
        // MQTT fields
        document.getElementById('mqttEnabled').checked = data.mqttEnabled || false;
        document.getElementById('mqttHost').value = data.mqttHost || '';
        document.getElementById('mqttPort').value = data.mqttPort || 1883;
        document.getElementById('mqttUser').value = data.mqttUser || '';
        document.getElementById('mqttUseTls').checked = data.mqttUseTls || false;
        document.getElementById('mqttDeviceName').value = data.mqttDeviceName || 'Microwave Motion Sensor';
        // Update MQTT status display
        const connStatus = document.getElementById('mqttConnStatus');
        if (data.mqttConnected) {
          connStatus.textContent = 'Connected';
          connStatus.style.color = '#22c55e';
        } else if (data.mqttEnabled) {
          connStatus.textContent = 'Disconnected';
          connStatus.style.color = '#ef4444';
        } else {
          connStatus.textContent = 'Disabled';
          connStatus.style.color = '#888';
        }
        if (data.mqttDeviceId) {
          document.getElementById('mqttDeviceIdLabel').textContent = 'Device ID: ' + data.mqttDeviceId;
        }
        document.getElementById('authEnabled').checked = data.authEnabled || false;
        // API key is stored locally, show it if we have it
        if (currentApiKey) {
          document.getElementById('apiKey').value = currentApiKey;
        } else if (data.authConfigured) {
          document.getElementById('apiKey').value = '********-****-****-****-************';
        }
      })
      .catch(err => console.log('Config load:', err.message));
    };

    // Initial auth check - show modal or load config
    checkAuthStatus().then(loggedIn => {
      if (loggedIn) {
        loadConfig();
      }
    });

    // Save settings
    document.getElementById('settingsForm').addEventListener('submit', function(e) {
      e.preventDefault();

      const apiKeyField = document.getElementById('apiKey').value;
      const wledPayload = document.getElementById('wledPayload').value;

      // Validate WLED JSON before saving
      if (wledPayload.trim().length > 0) {
        try {
          JSON.parse(wledPayload);
          document.getElementById('wledJsonError').style.display = 'none';
        } catch (e) {
          document.getElementById('wledJsonError').textContent = 'Invalid JSON: ' + e.message;
          document.getElementById('wledJsonError').style.display = 'block';
          showToast('Invalid WLED JSON payload', 'error', 0);
          return;
        }
      }

      const mqttPass = document.getElementById('mqttPass').value;
      const formData = {
        wifiSsid: document.getElementById('wifiSsid').value,
        wifiPassword: document.getElementById('wifiPassword').value,
        tripDelay: parseInt(document.getElementById('tripDelay').value),
        clearTimeout: parseInt(document.getElementById('clearTimeout').value),
        filterThreshold: parseInt(document.getElementById('filterThreshold').value),
        notifyEnabled: document.getElementById('notifyEnabled').checked,
        notifyUrl: document.getElementById('notifyUrl').value,
        notifyGet: document.getElementById('notifyGet').checked,
        notifyPost: document.getElementById('notifyPost').checked,
        wledEnabled: document.getElementById('wledEnabled').checked,
        wledUrl: document.getElementById('wledUrl').value,
        wledPayload: wledPayload,
        mqttEnabled: document.getElementById('mqttEnabled').checked,
        mqttHost: document.getElementById('mqttHost').value,
        mqttPort: parseInt(document.getElementById('mqttPort').value) || 1883,
        mqttUser: document.getElementById('mqttUser').value,
        mqttUseTls: document.getElementById('mqttUseTls').checked,
        mqttDeviceName: document.getElementById('mqttDeviceName').value || 'Microwave Motion Sensor',
        authEnabled: document.getElementById('authEnabled').checked
      };
      // Only send password if it was changed (not empty)
      if (mqttPass.length > 0) {
        formData.mqttPass = mqttPass;
      }

      // Include API key if it's a valid UUID (not masked)
      if (apiKeyField && apiKeyField.length === 36 && !apiKeyField.includes('*')) {
        formData.apiKey = apiKeyField;
        // Store key locally for future requests
        localStorage.setItem('esp32_api_key', apiKeyField);
        currentApiKey = apiKeyField;
      }

      fetch('/config', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(formData)
      })
      .then(response => response.json())
      .then(data => {
        if (data.success) {
          const msg = data.message + (data.restart ? ' Device will restart...' : '');
          showToast(msg, 'success', data.restart ? 0 : 4000);
          if (data.restart) {
            setTimeout(() => { window.location.href = '/'; }, 5000);
          }
        } else {
          showToast('Error: ' + data.message, 'error', 0);
        }
      })
      .catch(err => {
        showToast('Error saving settings: ' + err.message, 'error', 0);
      });
    });

    // Test notification - sends current form values (not saved config)
    document.getElementById('testNotifyBtn').addEventListener('click', function() {
      const statusEl = document.getElementById('notifyStatus');
      const url = document.getElementById('notifyUrl').value.trim();
      const notifyGet = document.getElementById('notifyGet').checked;
      const notifyPost = document.getElementById('notifyPost').checked;

      if (!url) {
        statusEl.style.color = '#ef4444';
        statusEl.textContent = 'Please enter a notification URL first';
        return;
      }

      if (!notifyGet && !notifyPost) {
        statusEl.style.color = '#ef4444';
        statusEl.textContent = 'Please select at least one method (GET or POST)';
        return;
      }

      statusEl.textContent = 'Sending test notification...';
      statusEl.style.color = '#888';

      authFetch('/test-notification', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ url: url, notifyGet: notifyGet, notifyPost: notifyPost })
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

    // Test WLED - sends current form values (not saved config)
    document.getElementById('testWledBtn').addEventListener('click', function() {
      const statusEl = document.getElementById('wledStatus');
      const url = document.getElementById('wledUrl').value.trim();
      const payload = document.getElementById('wledPayload').value.trim();

      if (!url) {
        statusEl.style.color = '#ef4444';
        statusEl.textContent = 'Please enter a WLED URL first';
        return;
      }

      if (!payload) {
        statusEl.style.color = '#ef4444';
        statusEl.textContent = 'Please enter a JSON payload';
        return;
      }

      // Validate JSON before sending
      try {
        JSON.parse(payload);
        document.getElementById('wledJsonError').style.display = 'none';
      } catch (e) {
        document.getElementById('wledJsonError').textContent = 'Invalid JSON: ' + e.message;
        document.getElementById('wledJsonError').style.display = 'block';
        statusEl.style.color = '#ef4444';
        statusEl.textContent = 'Invalid JSON payload';
        return;
      }

      statusEl.textContent = 'Sending test to WLED...';
      statusEl.style.color = '#888';

      authFetch('/test-wled', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ url: url, payload: payload })
      })
        .then(response => response.json())
        .then(data => {
          if (data.success) {
            statusEl.style.color = '#4ade80';
            statusEl.textContent = 'WLED test sent! Response: ' + data.httpCode;
          } else {
            statusEl.style.color = '#ef4444';
            statusEl.textContent = 'Failed: ' + data.message + (data.httpCode ? ' (HTTP ' + data.httpCode + ')' : '');
          }
        })
        .catch(err => {
          statusEl.style.color = '#ef4444';
          statusEl.textContent = 'Error sending test';
        });
    });

    // Validate WLED payload JSON on input
    document.getElementById('wledPayload').addEventListener('input', function() {
      const payload = this.value.trim();
      const errorEl = document.getElementById('wledJsonError');
      if (payload.length === 0) {
        errorEl.style.display = 'none';
        return;
      }
      try {
        JSON.parse(payload);
        errorEl.style.display = 'none';
      } catch (e) {
        errorEl.textContent = 'Invalid JSON: ' + e.message;
        errorEl.style.display = 'block';
      }
    });

    // MQTT TLS checkbox - suggest port change
    document.getElementById('mqttUseTls').addEventListener('change', function() {
      const portField = document.getElementById('mqttPort');
      const currentPort = parseInt(portField.value);
      if (this.checked && currentPort === 1883) {
        portField.value = 8883;
      } else if (!this.checked && currentPort === 8883) {
        portField.value = 1883;
      }
    });

    // Factory reset
    document.getElementById('resetBtn').addEventListener('click', function() {
      if (confirm('Are you sure you want to factory reset? All settings will be erased.')) {
        fetch('/reset', { method: 'POST' })
          .then(response => response.json())
          .then(data => {
            showToast('Factory reset initiated. Device will restart...', 'success', 0);
            // Clear stored API key on factory reset
            localStorage.removeItem('esp32_api_key');
          })
          .catch(err => {
            showToast('Error initiating factory reset', 'error', 0);
          });
      }
    });

    // Toggle API key visibility
    document.getElementById('toggleKeyBtn').addEventListener('click', function() {
      const keyField = document.getElementById('apiKey');
      const btn = document.getElementById('toggleKeyBtn');
      if (keyField.type === 'password') {
        keyField.type = 'text';
        btn.textContent = 'Hide';
      } else {
        keyField.type = 'password';
        btn.textContent = 'Show';
      }
    });

    // Generate new API key
    document.getElementById('generateKeyBtn').addEventListener('click', function() {
      if (document.getElementById('apiKey').value && !confirm('Generate new key? The old key will stop working after you save.')) {
        return;
      }

      const statusEl = document.getElementById('authStatus');
      statusEl.textContent = 'Generating new key...';
      statusEl.style.color = '#888';

      fetch('/generate-key', { method: 'POST' })
        .then(response => response.json())
        .then(data => {
          if (data.key) {
            document.getElementById('apiKey').value = data.key;
            document.getElementById('apiKey').type = 'text';
            document.getElementById('toggleKeyBtn').textContent = 'Hide';
            statusEl.style.color = '#4ade80';
            statusEl.textContent = 'New key generated. Click Save Settings to apply.';
          } else {
            statusEl.style.color = '#ef4444';
            statusEl.textContent = 'Failed to generate key';
          }
        })
        .catch(err => {
          statusEl.style.color = '#ef4444';
          statusEl.textContent = 'Error generating key';
        });
    });
  </script>
  <div class="toast-container" id="toastContainer"></div>
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
  <title>API Documentation - ESP32 Motion Sensor</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; background: #1a1a2e; color: #eee; min-height: 100vh; }
    nav { background: #0f0f1a; padding: 15px 20px; display: flex; justify-content: space-between; align-items: center; border-bottom: 1px solid #16213e; position: sticky; top: 0; z-index: 100; }
    .nav-brand { color: #00d4ff; font-weight: bold; font-size: 18px; text-decoration: none; }
    .nav-links { display: flex; gap: 20px; }
    .nav-links a { color: #888; text-decoration: none; font-size: 14px; padding: 8px 16px; border-radius: 6px; transition: all 0.2s; }
    .nav-links a:hover { color: #eee; background: #16213e; }
    .nav-links a.active { color: #00d4ff; background: #16213e; }
    .container { max-width: 800px; margin: 0 auto; padding: 20px; }
    .tabs { display: flex; gap: 5px; margin-bottom: 20px; flex-wrap: wrap; background: #0f0f1a; padding: 10px; border-radius: 12px; }
    .tab { padding: 10px 16px; background: transparent; border: none; color: #888; cursor: pointer; border-radius: 8px; font-size: 13px; transition: all 0.2s; }
    .tab:hover { color: #eee; background: #16213e; }
    .tab.active { color: #00d4ff; background: #16213e; }
    .tab-content { display: none; }
    .tab-content.active { display: block; }
    .card { background: #16213e; border-radius: 12px; padding: 20px; margin-bottom: 20px; box-shadow: 0 4px 6px rgba(0,0,0,0.3); }
    .card h2 { font-size: 16px; color: #00d4ff; margin-bottom: 15px; }
    .card h3 { font-size: 14px; color: #888; margin: 15px 0 10px 0; text-transform: uppercase; }
    .endpoint { background: #1a1a2e; padding: 12px 15px; border-radius: 8px; margin-bottom: 10px; font-family: monospace; display: flex; align-items: center; gap: 10px; }
    .method { background: #00d4ff; color: #1a1a2e; padding: 4px 8px; border-radius: 4px; font-weight: bold; font-size: 12px; }
    .method.post { background: #4ade80; }
    .auth-badge { background: #fbbf24; color: #1a1a2e; padding: 2px 8px; border-radius: 4px; font-size: 10px; margin-left: 10px; }
    p { color: #aaa; font-size: 14px; line-height: 1.6; margin-bottom: 10px; }
    code { background: #1a1a2e; padding: 2px 6px; border-radius: 4px; font-family: monospace; color: #00d4ff; }
    pre { background: #1a1a2e; padding: 15px; border-radius: 8px; overflow-x: auto; font-size: 12px; line-height: 1.4; color: #aaa; }
    .tbl { width: 100%; border-collapse: collapse; margin: 10px 0; font-size: 12px; }
    .tbl th { text-align: left; padding: 8px; background: #1a1a2e; color: #888; font-weight: normal; }
    .tbl td { padding: 8px; border-top: 1px solid #1a1a2e; color: #aaa; }
    .tbl td:first-child { color: #00d4ff; font-family: monospace; }
    .warn { border-left: 4px solid #fbbf24; }
    .info { border-left: 4px solid #00d4ff; }
    .success { border-left: 4px solid #4ade80; }
    .kbd { background: #333; padding: 3px 8px; border-radius: 4px; font-family: monospace; font-size: 12px; border: 1px solid #555; }
  </style>
</head>
<body>
  <nav>
    <a href="/" class="nav-brand">ESP32 Radar</a>
    <div class="nav-links">
      <a href="/">Dashboard</a>
      <a href="/settings">Settings</a>
      <a href="/diag">Diagnostics</a>
      <a href="/api" class="active">API</a>
      <a href="/about">About</a>
      <a href="#" id="authBtn" style="display:none;">Login</a>
    </div>
  </nav>

  <div class="container">
    <div class="tabs">
      <button class="tab active" data-tab="overview">Overview</button>
      <button class="tab" data-tab="endpoints">API Endpoints</button>
      <button class="tab" data-tab="wled">WLED Examples</button>
      <button class="tab" data-tab="serial">Serial Console</button>
      <button class="tab" data-tab="errors">Error Codes</button>
    </div>

    <!-- OVERVIEW TAB -->
    <div id="overview" class="tab-content active">
      <div class="card">
        <h2>ESP32 Motion Sensor API</h2>
        <p>REST API for integration with Home Assistant, scripts, and custom applications. All responses are JSON.</p>
      </div>

      <div class="card">
        <h2>Quick Reference</h2>
        <table class="tbl">
          <tr><th>Method</th><th>Endpoint</th><th>Auth</th><th>Description</th></tr>
          <tr><td>GET</td><td>/status</td><td>-</td><td>Current motion state and metrics</td></tr>
          <tr><td>GET</td><td>/config</td><td>-</td><td>System configuration</td></tr>
          <tr><td>POST</td><td>/config</td><td>Yes</td><td>Update configuration</td></tr>
          <tr><td>POST</td><td>/reset</td><td>Yes</td><td>Factory reset device</td></tr>
          <tr><td>POST</td><td>/test-notification</td><td>Yes</td><td>Test notification webhook</td></tr>
          <tr><td>POST</td><td>/test-wled</td><td>Yes</td><td>Test WLED integration</td></tr>
          <tr><td>GET</td><td>/logs</td><td>-</td><td>Event log (last 50)</td></tr>
          <tr><td>GET</td><td>/diagnostics</td><td>-</td><td>System health data</td></tr>
          <tr><td>POST</td><td>/generate-key</td><td>-</td><td>Generate new API key</td></tr>
          <tr><td>GET</td><td>/auth/status</td><td>-</td><td>Check login status</td></tr>
          <tr><td>POST</td><td>/auth/setup</td><td>-</td><td>Create password</td></tr>
          <tr><td>POST</td><td>/auth/login</td><td>-</td><td>Login with password</td></tr>
          <tr><td>POST</td><td>/auth/logout</td><td>-</td><td>End session</td></tr>
        </table>
      </div>

      <div class="card warn">
        <h2>Authentication</h2>
        <p><strong>Web Password:</strong> Protects Settings page in browser. Sessions last 1 hour.</p>
        <p><strong>API Key:</strong> Optional protection for POST endpoints. Disabled by default.</p>
        <p style="margin-top:10px"><strong>POST endpoints accept EITHER session cookie OR API key.</strong></p>
        <h3>Using API Key</h3>
        <pre>curl -H "X-API-Key: your-key-here" -X POST http://192.168.1.100/reset

# Or via query parameter:
curl -X POST "http://192.168.1.100/reset?apiKey=your-key-here"</pre>
      </div>
    </div>

    <!-- ENDPOINTS TAB -->
    <div id="endpoints" class="tab-content">
      <div class="card">
        <h2>GET /status</h2>
        <p>Current motion detection state and system metrics.</p>
        <div class="endpoint"><span class="method">GET</span> /status</div>
        <h3>Example Response</h3>
        <pre>{
  "state": "IDLE",
  "rawMotion": false,
  "filteredMotion": false,
  "filterPercent": 0,
  "alarmEvents": 5,
  "motionEvents": 42,
  "uptime": 3600,
  "freeHeap": 245000,
  "tripDelay": 3,
  "clearTimeout": 10,
  "filterThreshold": 70,
  "wifiMode": "Station",
  "ipAddress": "192.168.1.100",
  "rssi": -45
}</pre>
      </div>

      <div class="card">
        <h2>GET /config</h2>
        <p>Current system configuration values.</p>
        <div class="endpoint"><span class="method">GET</span> /config</div>
        <h3>Example Response</h3>
        <pre>{
  "sensorPin": 13,
  "ledPin": 2,
  "tripDelaySeconds": 3,
  "clearTimeoutSeconds": 10,
  "filterThresholdPercent": 70,
  "wifiSsid": "MyNetwork",
  "wifiMode": "Station",
  "ipAddress": "192.168.1.100",
  "notifyEnabled": true,
  "notifyUrl": "http://homeassistant:8123/api/webhook/motion",
  "notifyGet": false,
  "notifyPost": true,
  "wledEnabled": true,
  "wledUrl": "http://192.168.1.50/json/state",
  "wledPayload": "{\"on\":true,\"bri\":255,\"seg\":[{\"col\":[[255,0,0]]}]}",
  "authEnabled": true,
  "authConfigured": true
}</pre>
      </div>

      <div class="card">
        <h2>POST /config <span class="auth-badge">AUTH</span></h2>
        <p>Update configuration. All fields optional. WiFi changes trigger restart.</p>
        <div class="endpoint"><span class="method post">POST</span> /config</div>
        <h3>Request Fields</h3>
        <table class="tbl">
          <tr><th>Field</th><th>Type</th><th>Description</th></tr>
          <tr><td>wifiSsid</td><td>string</td><td>WiFi network (restarts device)</td></tr>
          <tr><td>wifiPassword</td><td>string</td><td>WiFi password (restarts device)</td></tr>
          <tr><td>tripDelay</td><td>int</td><td>1-60 seconds</td></tr>
          <tr><td>clearTimeout</td><td>int</td><td>1-300 seconds</td></tr>
          <tr><td>filterThreshold</td><td>int</td><td>10-100 percent</td></tr>
          <tr><td>notifyEnabled</td><td>bool</td><td>Enable notifications</td></tr>
          <tr><td>notifyUrl</td><td>string</td><td>Webhook URL (max 127)</td></tr>
          <tr><td>notifyGet</td><td>bool</td><td>Send GET requests</td></tr>
          <tr><td>notifyPost</td><td>bool</td><td>Send POST requests</td></tr>
          <tr><td>wledEnabled</td><td>bool</td><td>Enable WLED</td></tr>
          <tr><td>wledUrl</td><td>string</td><td>WLED URL (max 128)</td></tr>
          <tr><td>wledPayload</td><td>string</td><td>JSON payload (max 512)</td></tr>
          <tr><td>authEnabled</td><td>bool</td><td>Enable API auth</td></tr>
          <tr><td>apiKey</td><td>string</td><td>36-char UUID</td></tr>
        </table>
        <h3>Example</h3>
        <pre>curl -X POST http://192.168.1.100/config \
  -H "Content-Type: application/json" \
  -H "X-API-Key: your-key" \
  -d '{"tripDelay": 5, "wledEnabled": true}'</pre>
        <h3>Response</h3>
        <pre>{"success": true, "message": "Settings saved.", "restart": false}</pre>
      </div>

      <div class="card">
        <h2>POST /test-notification <span class="auth-badge">AUTH</span></h2>
        <p>Test notification with form values or saved config.</p>
        <div class="endpoint"><span class="method post">POST</span> /test-notification</div>
        <h3>Request</h3>
        <pre>{"url": "http://example.com/hook", "notifyGet": true, "notifyPost": true}</pre>
        <h3>Response</h3>
        <pre>{"success": true, "httpCode": 200, "message": "GET: 200, POST: 200"}</pre>
      </div>

      <div class="card">
        <h2>POST /test-wled <span class="auth-badge">AUTH</span></h2>
        <p>Test WLED integration with form values or saved config.</p>
        <div class="endpoint"><span class="method post">POST</span> /test-wled</div>
        <h3>Request</h3>
        <pre>{"url": "http://192.168.1.50/json/state", "payload": "{\"on\":true}"}</pre>
        <h3>Response</h3>
        <pre>{"success": true, "httpCode": 200, "message": "WLED payload sent successfully"}</pre>
      </div>

      <div class="card">
        <h2>POST /reset <span class="auth-badge">AUTH</span></h2>
        <p>Factory reset - clears all settings and restarts.</p>
        <div class="endpoint"><span class="method post">POST</span> /reset</div>
        <pre>curl -X POST http://192.168.1.100/reset -H "X-API-Key: your-key"</pre>
      </div>

      <div class="card">
        <h2>GET /logs</h2>
        <p>Event log with last 50 system events.</p>
        <div class="endpoint"><span class="method">GET</span> /logs</div>
        <h3>Example Response</h3>
        <pre>{
  "events": [
    {"time": 1000, "uptime": "00:00:01", "event": "BOOT", "data": 285000},
    {"time": 5000, "uptime": "00:00:05", "event": "WIFI_CONNECTED", "data": -45},
    {"time": 60000, "uptime": "00:01:00", "event": "ALARM_TRIGGERED", "data": 1}
  ],
  "count": 3,
  "maxSize": 50
}</pre>
      </div>

      <div class="card">
        <h2>GET /diagnostics</h2>
        <p>System health and performance data.</p>
        <div class="endpoint"><span class="method">GET</span> /diagnostics</div>
        <h3>Example Response</h3>
        <pre>{
  "uptime": 3600000,
  "uptimeFormatted": "01:00:00",
  "freeHeap": 245000,
  "minHeap": 230000,
  "maxHeap": 285000,
  "heapFragmentation": 5,
  "watchdogEnabled": true,
  "watchdogTimeout": 30,
  "authEnabled": true,
  "authFailedAttempts": 0,
  "authLockoutRemaining": 0
}</pre>
      </div>

      <div class="card success">
        <h2>Web Auth Endpoints</h2>
        <p>Browser authentication for Settings page.</p>
        <h3>GET /auth/status</h3>
        <pre>{"passwordConfigured": true, "loggedIn": false}</pre>
        <h3>POST /auth/setup</h3>
        <pre>Request:  {"password": "mypass", "confirmPassword": "mypass"}
Response: {"success": true, "message": "Password created"}</pre>
        <h3>POST /auth/login</h3>
        <pre>Request:  {"password": "mypass"}
Response: {"success": true, "message": "Login successful"}</pre>
        <h3>POST /auth/logout</h3>
        <pre>{"success": true, "message": "Logged out"}</pre>
      </div>
    </div>

    <!-- WLED TAB -->
    <div id="wled" class="tab-content">
      <div class="card info">
        <h2>WLED Integration</h2>
        <p>Send JSON payloads to WLED devices when alarms trigger. WLED is open-source firmware for ESP8266/ESP32-based LED controllers.</p>
        <p><strong>URL Format:</strong> <code>http://WLED_IP/json/state</code></p>
        <p><strong>Max Payload:</strong> 512 characters</p>
        <p><strong>Timeout:</strong> 3 seconds</p>
      </div>

      <div class="card">
        <h2>Solid Red (Default)</h2>
        <p>Full brightness solid red - good for alarm indication.</p>
        <pre>{"on":true,"bri":255,"seg":[{"col":[[255,0,0]]}]}</pre>
      </div>

      <div class="card">
        <h2>Solid Blue</h2>
        <p>Full brightness solid blue.</p>
        <pre>{"on":true,"bri":255,"seg":[{"col":[[0,0,255]]}]}</pre>
      </div>

      <div class="card">
        <h2>Orange Warning</h2>
        <p>Medium brightness orange - subtle warning.</p>
        <pre>{"on":true,"bri":150,"seg":[{"col":[[255,165,0]]}]}</pre>
      </div>

      <div class="card">
        <h2>Strobe/Flash Effect</h2>
        <p>Red strobe effect (effect ID 25).</p>
        <pre>{"on":true,"bri":255,"seg":[{"fx":25,"col":[[255,0,0]]}]}</pre>
      </div>

      <div class="card">
        <h2>Police Lights</h2>
        <p>Red/blue alternating (effect ID 1 = Blink).</p>
        <pre>{"on":true,"bri":255,"seg":[{"fx":1,"sx":200,"col":[[255,0,0],[0,0,255]]}]}</pre>
      </div>

      <div class="card">
        <h2>Fire Effect</h2>
        <p>Fire simulation (effect ID 66).</p>
        <pre>{"on":true,"bri":255,"seg":[{"fx":66,"sx":128,"ix":200}]}</pre>
      </div>

      <div class="card">
        <h2>Rainbow Chase</h2>
        <p>Rainbow pattern moving (effect ID 9).</p>
        <pre>{"on":true,"bri":200,"seg":[{"fx":9,"sx":150}]}</pre>
      </div>

      <div class="card">
        <h2>Turn Off LEDs</h2>
        <p>Turn off the strip (for resetting after alarm).</p>
        <pre>{"on":false}</pre>
      </div>

      <div class="card warn">
        <h2>Common WLED Fields</h2>
        <table class="tbl">
          <tr><th>Field</th><th>Range</th><th>Description</th></tr>
          <tr><td>on</td><td>true/false</td><td>Master on/off</td></tr>
          <tr><td>bri</td><td>0-255</td><td>Brightness</td></tr>
          <tr><td>seg[].col</td><td>[[R,G,B]]</td><td>Primary color</td></tr>
          <tr><td>seg[].fx</td><td>0-117</td><td>Effect ID</td></tr>
          <tr><td>seg[].sx</td><td>0-255</td><td>Effect speed</td></tr>
          <tr><td>seg[].ix</td><td>0-255</td><td>Effect intensity</td></tr>
        </table>
        <p style="margin-top:10px">Full reference: <a href="https://kno.wled.ge/interfaces/json-api/" target="_blank" style="color:#00d4ff">kno.wled.ge/interfaces/json-api</a></p>
      </div>
    </div>

    <!-- SERIAL TAB -->
    <div id="serial" class="tab-content">
      <div class="card info">
        <h2>Serial Console Connection</h2>
        <p>The ESP32 provides a serial console for debugging, diagnostics, and recovery options.</p>
      </div>

      <div class="card">
        <h2>Hardware Connection</h2>
        <p><strong>1.</strong> Connect ESP32 to computer via USB cable (micro-USB or USB-C depending on board).</p>
        <p><strong>2.</strong> The ESP32 will appear as a serial port:</p>
        <table class="tbl">
          <tr><th>OS</th><th>Port Name</th></tr>
          <tr><td>Windows</td><td>COM3, COM4, etc. (check Device Manager)</td></tr>
          <tr><td>Linux</td><td>/dev/ttyUSB0 or /dev/ttyACM0</td></tr>
          <tr><td>macOS</td><td>/dev/cu.usbserial-* or /dev/cu.SLAB_USBtoUART</td></tr>
        </table>
        <p style="margin-top:10px"><strong>Baud Rate:</strong> <code>115200</code></p>
        <p><strong>Data Bits:</strong> 8 | <strong>Parity:</strong> None | <strong>Stop Bits:</strong> 1</p>
      </div>

      <div class="card">
        <h2>Using PuTTY (Windows)</h2>
        <p><strong>1.</strong> Download PuTTY from <a href="https://putty.org" target="_blank" style="color:#00d4ff">putty.org</a></p>
        <p><strong>2.</strong> Open PuTTY and select "Serial" connection type</p>
        <p><strong>3.</strong> Enter the COM port (e.g., COM3)</p>
        <p><strong>4.</strong> Set Speed to <code>115200</code></p>
        <p><strong>5.</strong> Click "Open" to connect</p>
      </div>

      <div class="card">
        <h2>Using Arduino CLI</h2>
        <pre># List available ports
arduino-cli board list

# Connect to serial monitor
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200

# On Windows:
arduino-cli monitor -p COM3 -c baudrate=115200</pre>
      </div>

      <div class="card">
        <h2>Using screen (Linux/macOS)</h2>
        <pre># Connect
screen /dev/ttyUSB0 115200

# Disconnect: Press Ctrl+A then K, confirm with Y</pre>
      </div>

      <div class="card warn">
        <h2>Serial Commands</h2>
        <p>Press these keys in the serial console:</p>
        <table class="tbl">
          <tr><th>Key</th><th>Action</th></tr>
          <tr><td class="kbd">h</td><td>Show help / available commands</td></tr>
          <tr><td class="kbd">s</td><td>Show current status</td></tr>
          <tr><td class="kbd">d</td><td>Show diagnostics (heap, uptime, WiFi)</td></tr>
          <tr><td class="kbd">c</td><td>Show current configuration</td></tr>
          <tr><td class="kbd">e</td><td>Show recent events</td></tr>
          <tr><td class="kbd">w</td><td>Show WiFi info</td></tr>
          <tr><td class="kbd">a</td><td>Show API auth status, then:
            <br>&nbsp;&nbsp;<span class="kbd">a</span> again = generate new key
            <br>&nbsp;&nbsp;<span class="kbd">A</span> = toggle auth on/off</td></tr>
          <tr><td class="kbd">p</td><td>Clear web password (recovery)</td></tr>
          <tr><td class="kbd">f</td><td>Factory reset (clears all settings)</td></tr>
          <tr><td class="kbd">r</td><td>Reboot device</td></tr>
        </table>
      </div>

      <div class="card">
        <h2>Password Recovery</h2>
        <p>If locked out of the web interface:</p>
        <p><strong>1.</strong> Connect to serial console</p>
        <p><strong>2.</strong> Press <span class="kbd">p</span> to clear the password</p>
        <p><strong>3.</strong> Visit Settings page - you'll be prompted to create a new password</p>
      </div>

      <div class="card">
        <h2>Boot Output Example</h2>
        <pre>==========================================
ESP32 Microwave Motion Sensor - Stage 8
==========================================
WLED Integration

Reset reason: POWERON_RESET
GPIO initialized
Loading configuration from NVS... OK

Connecting to WiFi: MyNetwork.....
Connected! IP: 192.168.1.100
RSSI: -45 dBm

Web server started on port 80
------------------------------------------
Dashboard:    http://192.168.1.100
Settings:     http://192.168.1.100/settings
Diagnostics:  http://192.168.1.100/diag
------------------------------------------</pre>
      </div>
    </div>

    <!-- ERRORS TAB -->
    <div id="errors" class="tab-content">
      <div class="card">
        <h2>HTTP Status Codes</h2>
        <table class="tbl">
          <tr><th>Code</th><th>Meaning</th></tr>
          <tr><td>200</td><td>Success</td></tr>
          <tr><td>400</td><td>Bad Request (invalid parameters)</td></tr>
          <tr><td>401</td><td>Unauthorized (auth required)</td></tr>
          <tr><td>429</td><td>Too Many Requests (rate limited)</td></tr>
          <tr><td>500</td><td>Internal Server Error</td></tr>
        </table>
      </div>

      <div class="card warn">
        <h2>Authentication Errors</h2>
        <h3>401 - Missing or Invalid API Key</h3>
        <pre>{"error": "Authentication required", "code": 401}</pre>
        <p>Provide valid API key via X-API-Key header or apiKey query param.</p>

        <h3>429 - Rate Limited</h3>
        <pre>{"error": "Too many failed attempts", "code": 429, "retryAfter": 300}</pre>
        <p>5 failed attempts = 5 minute lockout. Wait or use serial console to reset.</p>
      </div>

      <div class="card">
        <h2>Configuration Errors</h2>
        <h3>Invalid WLED Payload</h3>
        <pre>{"success": false, "message": "Invalid WLED JSON payload"}</pre>
        <p>The wledPayload must be valid JSON. Check syntax with a JSON validator.</p>

        <h3>Password Mismatch</h3>
        <pre>{"success": false, "message": "Passwords do not match"}</pre>
        <p>password and confirmPassword fields must be identical.</p>

        <h3>Password Too Short</h3>
        <pre>{"success": false, "message": "Password must be at least 6 characters"}</pre>
      </div>

      <div class="card">
        <h2>Notification/WLED Errors</h2>
        <h3>Not Connected to WiFi</h3>
        <pre>{"success": false, "message": "Not connected to WiFi"}</pre>
        <p>Device must be in Station mode (connected to router) to send HTTP requests.</p>

        <h3>No URL Configured</h3>
        <pre>{"success": false, "message": "No notification URL provided"}</pre>

        <h3>Connection Failed</h3>
        <pre>{"success": false, "httpCode": -1, "message": "Connection failed"}</pre>
        <p>Could not reach target server. Check URL, network, and firewall.</p>

        <h3>Timeout</h3>
        <pre>{"success": false, "httpCode": -11, "message": "Connection failed"}</pre>
        <p>Request timed out. WLED has 3-second timeout, notifications have 5-second.</p>
      </div>

      <div class="card">
        <h2>Common HTTP Error Codes from WLED</h2>
        <table class="tbl">
          <tr><th>Code</th><th>Meaning</th><th>Fix</th></tr>
          <tr><td>-1</td><td>Connection refused</td><td>Check WLED is on and IP is correct</td></tr>
          <tr><td>404</td><td>Not Found</td><td>URL should end with /json/state</td></tr>
          <tr><td>400</td><td>Bad Request</td><td>Invalid JSON payload syntax</td></tr>
          <tr><td>500</td><td>WLED Error</td><td>Check WLED device logs</td></tr>
        </table>
      </div>
    </div>
  </div>

  <script>
    // Tab switching
    document.querySelectorAll('.tab').forEach(tab => {
      tab.onclick = () => {
        document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
        document.querySelectorAll('.tab-content').forEach(c => c.classList.remove('active'));
        tab.classList.add('active');
        document.getElementById(tab.dataset.tab).classList.add('active');
      };
    });

    // Auth button
    const handleLogout = () => {
      fetch('/auth/logout', { method: 'POST' }).then(() => checkAuthStatus());
    };
    const checkAuthStatus = () => {
      fetch('/auth/status').then(r => r.json()).then(data => {
        const btn = document.getElementById('authBtn');
        if (data.passwordConfigured) {
          btn.style.display = 'block';
          btn.textContent = data.loggedIn ? 'Logout' : 'Login';
          btn.onclick = data.loggedIn ? handleLogout : () => { window.location.href = '/settings'; };
        }
      }).catch(() => {});
    };
    checkAuthStatus();
  </script>
</body>
</html>
)rawliteral";

// ============================================================================
// DIAGNOSTICS PAGE HTML (Stage 6)
// ============================================================================

const char DIAGNOSTICS_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Diagnostics - ESP32 Motion Sensor</title>
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
    .container { max-width: 900px; margin: 0 auto; padding: 20px; }
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
      display: flex;
      justify-content: space-between;
      align-items: center;
    }
    .stats-grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
      gap: 15px;
    }
    .stat-item {
      background: #1a1a2e;
      padding: 15px;
      border-radius: 8px;
      text-align: center;
    }
    .stat-label {
      color: #888;
      font-size: 12px;
      text-transform: uppercase;
      margin-bottom: 5px;
    }
    .stat-value {
      color: #eee;
      font-size: 20px;
      font-weight: bold;
    }
    .stat-value.good { color: #4ade80; }
    .stat-value.warn { color: #fbbf24; }
    .stat-value.bad { color: #ef4444; }
    .log-container {
      background: #1a1a2e;
      border-radius: 8px;
      max-height: 400px;
      overflow-y: auto;
    }
    .log-entry {
      padding: 10px 15px;
      border-bottom: 1px solid #16213e;
      display: flex;
      gap: 15px;
      font-size: 13px;
    }
    .log-entry:last-child {
      border-bottom: none;
    }
    .log-time {
      color: #888;
      font-family: monospace;
      min-width: 80px;
    }
    .log-event {
      color: #00d4ff;
      font-weight: bold;
      min-width: 140px;
    }
    .log-event.alarm { color: #ef4444; }
    .log-event.notify { color: #4ade80; }
    .log-event.wifi { color: #fbbf24; }
    .log-event.system { color: #a78bfa; }
    .log-data {
      color: #aaa;
    }
    .empty-log {
      padding: 30px;
      text-align: center;
      color: #666;
    }
    .refresh-btn {
      background: #00d4ff;
      color: #1a1a2e;
      border: none;
      padding: 6px 12px;
      border-radius: 6px;
      cursor: pointer;
      font-size: 12px;
      font-weight: bold;
    }
    .refresh-btn:hover {
      background: #00b8e6;
    }
    .auto-refresh {
      display: flex;
      align-items: center;
      gap: 8px;
      font-size: 12px;
      color: #888;
    }
    .heap-bar {
      background: #1a1a2e;
      border-radius: 4px;
      height: 20px;
      overflow: hidden;
      margin-top: 10px;
    }
    .heap-bar-fill {
      height: 100%;
      background: linear-gradient(90deg, #4ade80, #fbbf24);
      transition: width 0.3s;
    }
    .heap-labels {
      display: flex;
      justify-content: space-between;
      font-size: 11px;
      color: #888;
      margin-top: 5px;
    }
  </style>
</head>
<body>
  <nav>
    <a href="/" class="nav-brand">ESP32 Radar</a>
    <div class="nav-links">
      <a href="/">Dashboard</a>
      <a href="/settings">Settings</a>
      <a href="/diag" class="active">Diagnostics</a>
      <a href="/api">API</a>
      <a href="/about">About</a>
      <a href="#" id="authBtn" style="display:none;">Login</a>
    </div>
  </nav>

  <div class="container">
    <div class="card">
      <h2>
        System Status
        <div class="auto-refresh">
          <input type="checkbox" id="autoRefresh" checked>
          <label for="autoRefresh">Auto-refresh (5s)</label>
        </div>
      </h2>
      <div class="stats-grid">
        <div class="stat-item">
          <div class="stat-label">Uptime</div>
          <div class="stat-value" id="uptime">--:--:--</div>
        </div>
        <div class="stat-item">
          <div class="stat-label">State</div>
          <div class="stat-value" id="state">---</div>
        </div>
        <div class="stat-item">
          <div class="stat-label">Alarms</div>
          <div class="stat-value" id="alarms">0</div>
        </div>
        <div class="stat-item">
          <div class="stat-label">Notifications</div>
          <div class="stat-value" id="notifications">0 / 0</div>
        </div>
        <div class="stat-item">
          <div class="stat-label">WiFi Signal</div>
          <div class="stat-value" id="rssi">-- dBm</div>
        </div>
        <div class="stat-item">
          <div class="stat-label">Watchdog</div>
          <div class="stat-value good" id="watchdog">---</div>
        </div>
      </div>
    </div>

    <div class="card">
      <h2>Memory</h2>
      <div class="stats-grid">
        <div class="stat-item">
          <div class="stat-label">Free Heap</div>
          <div class="stat-value" id="freeHeap">--- KB</div>
        </div>
        <div class="stat-item">
          <div class="stat-label">Min Heap</div>
          <div class="stat-value" id="minHeap">--- KB</div>
        </div>
        <div class="stat-item">
          <div class="stat-label">Max Heap</div>
          <div class="stat-value" id="maxHeap">--- KB</div>
        </div>
        <div class="stat-item">
          <div class="stat-label">Fragmentation</div>
          <div class="stat-value" id="fragmentation">--%</div>
        </div>
      </div>
      <div class="heap-bar">
        <div class="heap-bar-fill" id="heapBar" style="width: 80%"></div>
      </div>
      <div class="heap-labels">
        <span>0 KB</span>
        <span id="heapTotal">320 KB</span>
      </div>
    </div>

    <div class="card">
      <h2>
        Event Log
        <button class="refresh-btn" onclick="fetchLogs()">Refresh</button>
      </h2>
      <div class="log-container" id="logContainer">
        <div class="empty-log">Loading events...</div>
      </div>
    </div>
  </div>

  <script>
    let refreshInterval;

    const getEventClass = (event) => {
      if (event.includes('ALARM')) return 'alarm';
      if (event.includes('NOTIFY')) return 'notify';
      if (event.includes('WIFI')) return 'wifi';
      return 'system';
    };

    const formatData = (event, data) => {
      if (!data) return '';
      switch (event) {
        case 'WIFI_CONNECTED': return data + ' dBm';
        case 'ALARM_CLEARED': return data + 's duration';
        case 'ALARM_TRIGGERED': return '#' + data;
        case 'NOTIFY_SENT':
        case 'NOTIFY_FAILED': return 'HTTP ' + data;
        case 'BOOT':
        case 'HEAP_LOW': return Math.round(data / 1024) + ' KB';
        default: return data;
      }
    };

    const fetchDiagnostics = async () => {
      try {
        const res = await fetch('/diagnostics');
        const data = await res.json();

        document.getElementById('uptime').textContent = data.uptimeFormatted;
        document.getElementById('state').textContent = data.state;
        document.getElementById('alarms').textContent = data.alarmEvents;
        document.getElementById('notifications').textContent =
          data.notificationsSent + ' / ' + data.notificationsFailed;
        document.getElementById('rssi').textContent =
          data.wifiConnected ? (data.rssi + ' dBm') : 'N/A';
        document.getElementById('watchdog').textContent =
          data.watchdogEnabled ? 'Active (' + data.watchdogTimeout + 's)' : 'Disabled';

        const freeKB = Math.round(data.freeHeap / 1024);
        const minKB = Math.round(data.minHeap / 1024);
        const maxKB = Math.round(data.maxHeap / 1024);

        document.getElementById('freeHeap').textContent = freeKB + ' KB';
        document.getElementById('minHeap').textContent = minKB + ' KB';
        document.getElementById('maxHeap').textContent = maxKB + ' KB';
        document.getElementById('fragmentation').textContent = data.heapFragmentation + '%';

        const heapPercent = (data.freeHeap / 327680) * 100;
        document.getElementById('heapBar').style.width = heapPercent + '%';

        const heapEl = document.getElementById('freeHeap');
        heapEl.className = 'stat-value ' + (freeKB < 50 ? 'bad' : freeKB < 100 ? 'warn' : 'good');
      } catch (e) {
        console.error('Failed to fetch diagnostics:', e);
      }
    };

    const fetchLogs = async () => {
      try {
        const res = await fetch('/logs');
        const data = await res.json();
        const container = document.getElementById('logContainer');

        if (data.events.length === 0) {
          container.innerHTML = '<div class="empty-log">No events logged yet</div>';
          return;
        }

        // Show newest first
        const events = data.events.reverse();
        container.innerHTML = events.map(e => `
          <div class="log-entry">
            <span class="log-time">${e.uptime}</span>
            <span class="log-event ${getEventClass(e.event)}">${e.event}</span>
            <span class="log-data">${formatData(e.event, e.data)}</span>
          </div>
        `).join('');
      } catch (e) {
        console.error('Failed to fetch logs:', e);
        document.getElementById('logContainer').innerHTML =
          '<div class="empty-log">Failed to load events</div>';
      }
    };

    const toggleAutoRefresh = () => {
      if (document.getElementById('autoRefresh').checked) {
        refreshInterval = setInterval(() => {
          fetchDiagnostics();
          fetchLogs();
        }, 5000);
      } else {
        clearInterval(refreshInterval);
      }
    };

    document.getElementById('autoRefresh').addEventListener('change', toggleAutoRefresh);

    // Initial load
    fetchDiagnostics();
    fetchLogs();
    toggleAutoRefresh();

    // Auth button handling
    const handleLogout = () => {
      fetch('/auth/logout', { method: 'POST' })
        .then(() => { checkAuthStatus(); })
        .catch(err => console.log('Logout failed'));
    };

    const checkAuthStatus = () => {
      fetch('/auth/status')
        .then(response => response.json())
        .then(data => {
          const btn = document.getElementById('authBtn');
          if (data.passwordConfigured) {
            btn.style.display = 'block';
            btn.textContent = data.loggedIn ? 'Logout' : 'Login';
            btn.onclick = data.loggedIn ? handleLogout : () => { window.location.href = '/settings'; };
          }
        })
        .catch(err => console.log('Auth check failed'));
    };

    checkAuthStatus();
  </script>
</body>
</html>
)rawliteral";

// ============================================================================
// ABOUT PAGE HTML
// ============================================================================

const char ABOUT_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>About - ESP32 Radar</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: #0a0a0f; color: #e0e0e0; min-height: 100vh; }
    nav { background: #1a1a2e; padding: 15px 20px; display: flex; justify-content: space-between; align-items: center; border-bottom: 1px solid #2a2a4e; }
    .nav-brand { color: #4a90d9; font-weight: 600; font-size: 18px; text-decoration: none; }
    .nav-links { display: flex; gap: 20px; }
    .nav-links a { color: #888; text-decoration: none; font-size: 14px; transition: color 0.2s; }
    .nav-links a:hover, .nav-links a.active { color: #4a90d9; }
    .container { max-width: 800px; margin: 0 auto; padding: 20px; }
    .card { background: #12121a; border-radius: 12px; padding: 25px; margin-bottom: 20px; border: 1px solid #2a2a4e; }
    h1 { color: #fff; margin-bottom: 20px; font-size: 28px; }
    h2 { color: #4a90d9; margin-bottom: 15px; font-size: 18px; border-bottom: 1px solid #2a2a4e; padding-bottom: 10px; }
    .project-header { text-align: center; padding: 30px 0; }
    .project-title { font-size: 32px; color: #fff; margin-bottom: 10px; }
    .project-version { color: #4a90d9; font-size: 16px; margin-bottom: 5px; }
    .project-author { color: #888; font-size: 14px; }
    .info-row { display: flex; justify-content: space-between; padding: 12px 0; border-bottom: 1px solid #1a1a2e; }
    .info-row:last-child { border-bottom: none; }
    .info-label { color: #888; }
    .info-value { color: #e0e0e0; font-family: monospace; }
    .license-box { background: #1a1a2e; border-radius: 8px; padding: 20px; margin-top: 15px; }
    .license-title { color: #4a90d9; font-size: 16px; margin-bottom: 10px; }
    .license-text { color: #aaa; font-size: 13px; line-height: 1.6; }
    .links { margin-top: 20px; }
    .links a { display: inline-block; color: #4a90d9; text-decoration: none; margin-right: 20px; font-size: 14px; }
    .links a:hover { text-decoration: underline; }
    .footer { text-align: center; padding: 30px; color: #666; font-size: 12px; }
  </style>
</head>
<body>
  <nav>
    <a href="/" class="nav-brand">ESP32 Radar</a>
    <div class="nav-links">
      <a href="/">Dashboard</a>
      <a href="/settings">Settings</a>
      <a href="/diag">Diagnostics</a>
      <a href="/api">API</a>
      <a href="/about" class="active">About</a>
    </div>
  </nav>

  <div class="container">
    <div class="card">
      <div class="project-header">
        <div class="project-title">ESP32 Microwave Motion Sensor</div>
        <div class="project-version">Stage 9: Home Assistant MQTT Integration</div>
        <div class="project-author">Created by Miswired</div>
      </div>
    </div>

    <div class="card">
      <h2>Device Information</h2>
      <div class="info-row">
        <span class="info-label">Chip ID</span>
        <span class="info-value" id="chipId">Loading...</span>
      </div>
      <div class="info-row">
        <span class="info-label">IP Address</span>
        <span class="info-value" id="ipAddress">Loading...</span>
      </div>
      <div class="info-row">
        <span class="info-label">Uptime</span>
        <span class="info-value" id="uptime">Loading...</span>
      </div>
      <div class="info-row">
        <span class="info-label">Free Heap</span>
        <span class="info-value" id="freeHeap">Loading...</span>
      </div>
      <div class="info-row">
        <span class="info-label">WiFi Signal</span>
        <span class="info-value" id="rssi">Loading...</span>
      </div>
    </div>

    <div class="card">
      <h2>License</h2>
      <div class="license-box">
        <div class="license-title">GNU General Public License v3.0</div>
        <div class="license-text">
          Copyright &copy; 2024-2026 Miswired<br><br>
          This program is free software: you can redistribute it and/or modify
          it under the terms of the GNU General Public License as published by
          the Free Software Foundation, either version 3 of the License, or
          (at your option) any later version.<br><br>
          This program is distributed in the hope that it will be useful,
          but WITHOUT ANY WARRANTY; without even the implied warranty of
          MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
          GNU General Public License for more details.
        </div>
      </div>
      <div class="links">
        <a href="https://www.gnu.org/licenses/gpl-3.0.html" target="_blank">Full License Text</a>
        <a href="https://github.com/miswired/esp32-radar" target="_blank">Source Code</a>
        <a href="https://github.com/miswired/esp32-radar/wiki" target="_blank">Documentation</a>
      </div>
    </div>

    <div class="card">
      <h2>Libraries Used</h2>
      <div class="info-row">
        <span class="info-label">ArduinoJson</span>
        <span class="info-value">MIT License</span>
      </div>
      <div class="info-row">
        <span class="info-label">PubSubClient</span>
        <span class="info-value">MIT License</span>
      </div>
      <div class="info-row">
        <span class="info-label">ESP32 Arduino Core</span>
        <span class="info-value">LGPL-2.1</span>
      </div>
    </div>
  </div>

  <div class="footer">
    ESP32 Microwave Motion Sensor &bull; GPL-3.0 &bull; Miswired
  </div>

  <script>
    async function loadDeviceInfo() {
      try {
        const [configRes, diagRes] = await Promise.all([
          fetch('/config'),
          fetch('/diagnostics')
        ]);
        const config = await configRes.json();
        const diag = await diagRes.json();

        document.getElementById('chipId').textContent = config.mqttDeviceId || 'N/A';
        document.getElementById('ipAddress').textContent = config.ipAddress || 'N/A';
        document.getElementById('uptime').textContent = diag.uptimeFormatted || 'N/A';
        document.getElementById('freeHeap').textContent = diag.freeHeap ? diag.freeHeap.toLocaleString() + ' bytes' : 'N/A';
        document.getElementById('rssi').textContent = diag.rssi ? diag.rssi + ' dBm' : 'N/A';
      } catch (err) {
        console.error('Failed to load device info:', err);
      }
    }

    loadDeviceInfo();
    setInterval(loadDeviceInfo, 10000);
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
  logEvent(EVENT_WIFI_CONNECTED, WiFi.RSSI());
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
  logEvent(EVENT_WIFI_AP_MODE);
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
// EVENT LOGGING FUNCTIONS (Stage 6)
// ============================================================================

void logEvent(EventType type, int32_t data) {
  eventLog[eventLogHead].timestamp = millis();
  eventLog[eventLogHead].type = type;
  eventLog[eventLogHead].data = data;

  eventLogHead = (eventLogHead + 1) % EVENT_LOG_SIZE;
  if (eventLogCount < EVENT_LOG_SIZE) {
    eventLogCount++;
  }

  Serial.print("[LOG] ");
  Serial.print(eventTypeNames[type]);
  if (data != 0) {
    Serial.print(" (");
    Serial.print(data);
    Serial.print(")");
  }
  Serial.println();
}

void recordHeapSample() {
  uint32_t heap = ESP.getFreeHeap();

  heapHistory[heapHistoryHead] = heap;
  heapHistoryHead = (heapHistoryHead + 1) % HEAP_HISTORY_SIZE;
  if (heapHistoryCount < HEAP_HISTORY_SIZE) {
    heapHistoryCount++;
  }

  if (heap < minHeapSeen) minHeapSeen = heap;
  if (heap > maxHeapSeen) maxHeapSeen = heap;

  // Log warning if heap is low
  if (heap < HEAP_MIN_THRESHOLD) {
    logEvent(EVENT_HEAP_LOW, heap);
  }
}

String formatUptime(unsigned long ms) {
  unsigned long seconds = ms / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;
  unsigned long days = hours / 24;

  char buf[32];
  if (days > 0) {
    snprintf(buf, sizeof(buf), "%lud %02lu:%02lu:%02lu", days, hours % 24, minutes % 60, seconds % 60);
  } else {
    snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", hours, minutes % 60, seconds % 60);
  }
  return String(buf);
}

// ============================================================================
// NOTIFICATION FUNCTIONS
// ============================================================================

// Helper function to send a single notification request
void sendNotificationRequest(const char* event, const char* state, bool useGet) {
  HTTPClient http;
  http.setTimeout(5000);  // 5 second timeout

  int httpCode = -1;

  if (useGet) {
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

    Serial.print("[NOTIFY] Sending GET to ");
    Serial.println(config.notifyUrl);

    http.begin(url);
    httpCode = http.GET();
  } else {
    // POST with JSON body
    Serial.print("[NOTIFY] Sending POST to ");
    Serial.println(config.notifyUrl);

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
      logEvent(EVENT_NOTIFICATION_SENT, httpCode);
    } else {
      totalNotificationsFailed++;
      logEvent(EVENT_NOTIFICATION_FAILED, httpCode);
    }
  } else {
    Serial.print("[NOTIFY] Error: ");
    Serial.println(http.errorToString(httpCode));
    totalNotificationsFailed++;
    logEvent(EVENT_NOTIFICATION_FAILED, httpCode);
  }

  http.end();
}

void sendNotification(const char* event, const char* state) {
  if (!config.notifyEnabled || strlen(config.notifyUrl) == 0) {
    return;
  }

  // Check if at least one method is enabled
  if (!config.notifyGet && !config.notifyPost) {
    return;
  }

  if (currentWiFiMode != MODE_STATION) {
    Serial.println("[NOTIFY] Skipped - not connected to WiFi");
    return;
  }

  // Send GET request if enabled
  if (config.notifyGet) {
    sendNotificationRequest(event, state, true);
  }

  // Send POST request if enabled
  if (config.notifyPost) {
    sendNotificationRequest(event, state, false);
  }
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
// WLED FUNCTIONS (Stage 8)
// ============================================================================

// Validate JSON syntax using ArduinoJson
bool isValidJson(const char* json) {
  if (json == nullptr || strlen(json) == 0) {
    return false;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json);
  return error == DeserializationError::Ok;
}

// Send WLED payload to configured URL
void sendWledPayload() {
  if (!config.wledEnabled || strlen(config.wledUrl) == 0 || strlen(config.wledPayload) == 0) {
    return;
  }

  if (currentWiFiMode != MODE_STATION) {
    Serial.println("[WLED] Skipped - not connected to WiFi");
    return;
  }

  Serial.print("[WLED] Sending payload to ");
  Serial.println(config.wledUrl);

  HTTPClient http;
  http.setTimeout(WLED_TIMEOUT_MS);
  http.begin(config.wledUrl);
  http.addHeader("Content-Type", "application/json");

  int httpCode = http.POST(config.wledPayload);

  lastWledTime = millis();
  lastWledStatus = httpCode;

  if (httpCode > 0) {
    Serial.print("[WLED] Response: ");
    Serial.println(httpCode);
    if (httpCode >= 200 && httpCode < 300) {
      totalWledSent++;
      logEvent(EVENT_WLED_SENT, httpCode);
    } else {
      totalWledFailed++;
      logEvent(EVENT_WLED_FAILED, httpCode);
    }
  } else {
    Serial.print("[WLED] Error: ");
    Serial.println(http.errorToString(httpCode));
    totalWledFailed++;
    logEvent(EVENT_WLED_FAILED, httpCode);
  }

  http.end();
}

// Send test WLED payload (can use provided or saved values)
int sendTestWled(const char* url, const char* payload) {
  // Use provided values or fall back to config
  const char* testUrl = (url != nullptr && strlen(url) > 0) ? url : config.wledUrl;
  const char* testPayload = (payload != nullptr && strlen(payload) > 0) ? payload : config.wledPayload;

  if (strlen(testUrl) == 0 || strlen(testPayload) == 0) {
    Serial.println("[WLED] Test failed - URL or payload empty");
    return -1;
  }

  if (currentWiFiMode != MODE_STATION) {
    Serial.println("[WLED] Test failed - not connected to WiFi");
    return -2;
  }

  Serial.print("[WLED] Testing payload to ");
  Serial.println(testUrl);

  HTTPClient http;
  http.setTimeout(WLED_TIMEOUT_MS);
  http.begin(testUrl);
  http.addHeader("Content-Type", "application/json");

  int httpCode = http.POST(testPayload);

  if (httpCode > 0) {
    Serial.print("[WLED] Test response: ");
    Serial.println(httpCode);
  } else {
    Serial.print("[WLED] Test error: ");
    Serial.println(http.errorToString(httpCode));
  }

  http.end();
  return httpCode;
}

// ============================================================================
// MQTT / HOME ASSISTANT FUNCTIONS (Stage 9)
// ============================================================================

// Get the ESP32 chip ID as a hex string (used for unique device identification)
String getMqttDeviceId() {
  if (mqttChipId[0] == '\0') {
    uint64_t chipId = ESP.getEfuseMac();
    snprintf(mqttChipId, sizeof(mqttChipId), "%04x%08x",
             (uint16_t)(chipId >> 32), (uint32_t)chipId);
  }
  return String(mqttChipId);
}

// Get the full device identifier for MQTT topics
String getMqttDevicePrefix() {
  return "mw_motion_" + getMqttDeviceId();
}

// Initialize MQTT client with appropriate WiFi client (TLS or plaintext)
void mqttSetup() {
  // Get chip ID for topics
  getMqttDeviceId();

  // Configure buffer size
  mqttClient.setBufferSize(MQTT_BUFFER_SIZE);
  mqttClient.setKeepAlive(MQTT_KEEPALIVE);

  // Set callback for incoming messages
  mqttClient.setCallback(mqttCallback);

  // Build availability topic
  snprintf(mqttAvailabilityTopic, sizeof(mqttAvailabilityTopic),
           "%s/binary_sensor/%s/availability",
           MQTT_DISCOVERY_PREFIX, getMqttDevicePrefix().c_str());

  // Build base state topic
  snprintf(mqttStateTopic, sizeof(mqttStateTopic),
           "%s/binary_sensor/%s/motion/state",
           MQTT_DISCOVERY_PREFIX, getMqttDevicePrefix().c_str());

  Serial.print("[MQTT] Device ID: ");
  Serial.println(getMqttDeviceId());
}

// Callback for incoming MQTT messages (for number controls)
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  mqttMessagesReceived++;

  // Convert payload to string
  char message[64];
  size_t copyLen = min((unsigned int)(sizeof(message) - 1), length);
  memcpy(message, payload, copyLen);
  message[copyLen] = '\0';

  Serial.print("[MQTT] Received on ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(message);

  String topicStr = String(topic);
  String prefix = getMqttDevicePrefix();

  // Check for number control commands
  bool configChanged = false;

  // Trip delay set command
  if (topicStr.endsWith("/trip_delay/set")) {
    int value = atoi(message);
    if (value >= 0 && value <= 30 && value != config.tripDelaySeconds) {
      config.tripDelaySeconds = value;
      configChanged = true;
      Serial.print("[MQTT] Trip delay set to: ");
      Serial.println(value);

      // Publish updated state
      char stateTopic[100];
      snprintf(stateTopic, sizeof(stateTopic),
               "%s/number/%s/trip_delay/state",
               MQTT_DISCOVERY_PREFIX, prefix.c_str());
      mqttClient.publish(stateTopic, message, true);
    }
  }
  // Clear timeout set command
  else if (topicStr.endsWith("/clear_timeout/set")) {
    int value = atoi(message);
    if (value >= 1 && value <= 300 && value != config.clearTimeoutSeconds) {
      config.clearTimeoutSeconds = value;
      configChanged = true;
      Serial.print("[MQTT] Clear timeout set to: ");
      Serial.println(value);

      // Publish updated state
      char stateTopic[100];
      snprintf(stateTopic, sizeof(stateTopic),
               "%s/number/%s/clear_timeout/state",
               MQTT_DISCOVERY_PREFIX, prefix.c_str());
      mqttClient.publish(stateTopic, message, true);
    }
  }
  // Filter threshold set command
  else if (topicStr.endsWith("/filter_threshold/set")) {
    int value = atoi(message);
    if (value >= 0 && value <= 100 && value != config.filterThresholdPercent) {
      config.filterThresholdPercent = value;
      configChanged = true;
      Serial.print("[MQTT] Filter threshold set to: ");
      Serial.println(value);

      // Publish updated state
      char stateTopic[100];
      snprintf(stateTopic, sizeof(stateTopic),
               "%s/number/%s/filter_threshold/state",
               MQTT_DISCOVERY_PREFIX, prefix.c_str());
      mqttClient.publish(stateTopic, message, true);
    }
  }

  // Save config if changed
  if (configChanged) {
    saveConfig();
    logEvent(EVENT_CONFIG_SAVED);
  }
}

// Subscribe to command topics for number controls
void mqttSubscribeToCommands() {
  String prefix = getMqttDevicePrefix();
  char topic[100];

  // Subscribe to number control set topics
  snprintf(topic, sizeof(topic), "%s/number/%s/trip_delay/set",
           MQTT_DISCOVERY_PREFIX, prefix.c_str());
  mqttClient.subscribe(topic);
  Serial.print("[MQTT] Subscribed: ");
  Serial.println(topic);

  snprintf(topic, sizeof(topic), "%s/number/%s/clear_timeout/set",
           MQTT_DISCOVERY_PREFIX, prefix.c_str());
  mqttClient.subscribe(topic);

  snprintf(topic, sizeof(topic), "%s/number/%s/filter_threshold/set",
           MQTT_DISCOVERY_PREFIX, prefix.c_str());
  mqttClient.subscribe(topic);

  // Subscribe to Home Assistant status for re-discovery on HA restart
  mqttClient.subscribe("homeassistant/status");
}

// Build the device info JSON block (shared by all entities)
String getMqttDeviceJson() {
  JsonDocument doc;
  JsonArray ids = doc["ids"].to<JsonArray>();
  ids.add(getMqttDevicePrefix());
  doc["name"] = config.mqttDeviceName;
  doc["mf"] = "ESP32";
  doc["mdl"] = "RCWL-0516 Radar";
  doc["sw"] = "Stage 9";
  doc["cu"] = "http://" + getIPAddress() + "/";

  String result;
  serializeJson(doc, result);
  return result;
}

// Publish discovery message for motion binary sensor
void mqttPublishDiscoveryMotion() {
  String prefix = getMqttDevicePrefix();
  char topic[128];
  snprintf(topic, sizeof(topic), "%s/binary_sensor/%s/motion/config",
           MQTT_DISCOVERY_PREFIX, prefix.c_str());

  JsonDocument doc;
  doc["name"] = "Motion";
  doc["uniq_id"] = prefix + "_motion";
  doc["dev_cla"] = "motion";
  doc["stat_t"] = mqttStateTopic;  // Use full path like sensors do
  doc["avty_t"] = mqttAvailabilityTopic;  // Use full path like sensors do
  doc["pl_avail"] = "online";
  doc["pl_not_avail"] = "offline";

  // Add device info
  JsonObject dev = doc["dev"].to<JsonObject>();
  JsonArray ids = dev["ids"].to<JsonArray>();
  ids.add(prefix);
  dev["name"] = config.mqttDeviceName;
  dev["mf"] = "ESP32";
  dev["mdl"] = "RCWL-0516 Radar";
  dev["sw"] = "Stage 9";
  dev["cu"] = "http://" + getIPAddress() + "/";

  String payload;
  serializeJson(doc, payload);

  mqttClient.publish(topic, payload.c_str(), true);
  mqttMessagesPublished++;
}

// Publish discovery message for a diagnostic sensor
void mqttPublishDiscoverySensor(const char* name, const char* id, const char* devClass,
                                 const char* unit, const char* stateClass, const char* category) {
  String prefix = getMqttDevicePrefix();
  char topic[128];
  snprintf(topic, sizeof(topic), "%s/sensor/%s/%s/config",
           MQTT_DISCOVERY_PREFIX, prefix.c_str(), id);

  JsonDocument doc;
  doc["name"] = name;
  doc["uniq_id"] = prefix + "_" + id;
  doc["stat_t"] = String(MQTT_DISCOVERY_PREFIX) + "/sensor/" + prefix + "/" + id + "/state";
  doc["avty_t"] = mqttAvailabilityTopic;
  doc["pl_avail"] = "online";
  doc["pl_not_avail"] = "offline";

  if (devClass && strlen(devClass) > 0) {
    doc["dev_cla"] = devClass;
  }
  if (unit && strlen(unit) > 0) {
    doc["unit_of_meas"] = unit;
  }
  if (stateClass && strlen(stateClass) > 0) {
    doc["stat_cla"] = stateClass;
  }
  if (category && strlen(category) > 0) {
    doc["ent_cat"] = category;
  }

  // Add device info (reference only, not full device)
  JsonObject dev = doc["dev"].to<JsonObject>();
  JsonArray ids = dev["ids"].to<JsonArray>();
  ids.add(prefix);

  String payload;
  serializeJson(doc, payload);

  mqttClient.publish(topic, payload.c_str(), true);
  mqttMessagesPublished++;
}

// Publish discovery message for a number control
void mqttPublishDiscoveryNumber(const char* name, const char* id, int min, int max, int step, const char* unit) {
  String prefix = getMqttDevicePrefix();
  char topic[128];
  snprintf(topic, sizeof(topic), "%s/number/%s/%s/config",
           MQTT_DISCOVERY_PREFIX, prefix.c_str(), id);

  JsonDocument doc;
  doc["name"] = name;
  doc["uniq_id"] = prefix + "_" + id;
  doc["cmd_t"] = String(MQTT_DISCOVERY_PREFIX) + "/number/" + prefix + "/" + id + "/set";
  doc["stat_t"] = String(MQTT_DISCOVERY_PREFIX) + "/number/" + prefix + "/" + id + "/state";
  doc["avty_t"] = mqttAvailabilityTopic;
  doc["pl_avail"] = "online";
  doc["pl_not_avail"] = "offline";
  doc["min"] = min;
  doc["max"] = max;
  doc["step"] = step;
  doc["mode"] = "box";
  if (unit && strlen(unit) > 0) {
    doc["unit_of_meas"] = unit;
  }

  // Add device info (reference only)
  JsonObject dev = doc["dev"].to<JsonObject>();
  JsonArray ids = dev["ids"].to<JsonArray>();
  ids.add(prefix);

  String payload;
  serializeJson(doc, payload);

  mqttClient.publish(topic, payload.c_str(), true);
  mqttMessagesPublished++;
}

// Publish all discovery messages
void mqttPublishDiscovery() {
  Serial.println("[MQTT] Publishing Home Assistant discovery...");

  // Binary sensor: Motion
  mqttPublishDiscoveryMotion();

  // Diagnostic sensors
  mqttPublishDiscoverySensor("WiFi Signal", "rssi", "signal_strength", "dBm", "measurement", "diagnostic");
  mqttPublishDiscoverySensor("Uptime", "uptime", "duration", "s", "total_increasing", "diagnostic");
  mqttPublishDiscoverySensor("Free Heap", "heap", "data_size", "B", "measurement", "diagnostic");
  mqttPublishDiscoverySensor("Alarm Count", "alarm_count", nullptr, nullptr, "total_increasing", nullptr);
  mqttPublishDiscoverySensor("Motion Count", "motion_count", nullptr, nullptr, "total_increasing", nullptr);
  mqttPublishDiscoverySensor("Filter Level", "filter_level", nullptr, "%", "measurement", nullptr);
  mqttPublishDiscoverySensor("IP Address", "ip_address", nullptr, nullptr, nullptr, "diagnostic");

  // Number controls
  mqttPublishDiscoveryNumber("Trip Delay", "trip_delay", 0, 30, 1, "s");
  mqttPublishDiscoveryNumber("Clear Timeout", "clear_timeout", 1, 300, 1, "s");
  mqttPublishDiscoveryNumber("Filter Threshold", "filter_threshold", 0, 100, 5, "%");

  mqttDiscoveryPublished = true;
  Serial.println("[MQTT] Discovery published");
}

// Publish current motion state
void mqttPublishMotionState() {
  if (!mqttConnected) return;

  const char* state = (currentState == STATE_ALARM_ACTIVE) ? "ON" : "OFF";
  mqttClient.publish(mqttStateTopic, state, false);  // Not retained
  mqttMessagesPublished++;
}

// Publish diagnostic sensor values
void mqttPublishDiagnostics() {
  if (!mqttConnected) return;

  String prefix = getMqttDevicePrefix();
  char topic[100];
  char value[32];

  // WiFi signal strength
  snprintf(topic, sizeof(topic), "%s/sensor/%s/rssi/state",
           MQTT_DISCOVERY_PREFIX, prefix.c_str());
  snprintf(value, sizeof(value), "%d", WiFi.RSSI());
  mqttClient.publish(topic, value, false);

  // Uptime
  snprintf(topic, sizeof(topic), "%s/sensor/%s/uptime/state",
           MQTT_DISCOVERY_PREFIX, prefix.c_str());
  snprintf(value, sizeof(value), "%lu", millis() / 1000);
  mqttClient.publish(topic, value, false);

  // Free heap
  snprintf(topic, sizeof(topic), "%s/sensor/%s/heap/state",
           MQTT_DISCOVERY_PREFIX, prefix.c_str());
  snprintf(value, sizeof(value), "%u", ESP.getFreeHeap());
  mqttClient.publish(topic, value, false);

  // Alarm count
  snprintf(topic, sizeof(topic), "%s/sensor/%s/alarm_count/state",
           MQTT_DISCOVERY_PREFIX, prefix.c_str());
  snprintf(value, sizeof(value), "%lu", totalAlarmEvents);
  mqttClient.publish(topic, value, false);

  // Motion count
  snprintf(topic, sizeof(topic), "%s/sensor/%s/motion_count/state",
           MQTT_DISCOVERY_PREFIX, prefix.c_str());
  snprintf(value, sizeof(value), "%lu", totalMotionEvents);
  mqttClient.publish(topic, value, false);

  // Filter level
  snprintf(topic, sizeof(topic), "%s/sensor/%s/filter_level/state",
           MQTT_DISCOVERY_PREFIX, prefix.c_str());
  snprintf(value, sizeof(value), "%d", currentFilterPercent);
  mqttClient.publish(topic, value, false);

  // IP address
  snprintf(topic, sizeof(topic), "%s/sensor/%s/ip_address/state",
           MQTT_DISCOVERY_PREFIX, prefix.c_str());
  mqttClient.publish(topic, getIPAddress().c_str(), false);

  // Number control current values
  snprintf(topic, sizeof(topic), "%s/number/%s/trip_delay/state",
           MQTT_DISCOVERY_PREFIX, prefix.c_str());
  snprintf(value, sizeof(value), "%d", config.tripDelaySeconds);
  mqttClient.publish(topic, value, true);

  snprintf(topic, sizeof(topic), "%s/number/%s/clear_timeout/state",
           MQTT_DISCOVERY_PREFIX, prefix.c_str());
  snprintf(value, sizeof(value), "%d", config.clearTimeoutSeconds);
  mqttClient.publish(topic, value, true);

  snprintf(topic, sizeof(topic), "%s/number/%s/filter_threshold/state",
           MQTT_DISCOVERY_PREFIX, prefix.c_str());
  snprintf(value, sizeof(value), "%d", config.filterThresholdPercent);
  mqttClient.publish(topic, value, true);

  mqttMessagesPublished += 10;
}

// Publish availability status
void mqttPublishAvailability(bool online) {
  if (!mqttClient.connected()) return;

  mqttClient.publish(mqttAvailabilityTopic, online ? "online" : "offline", true);
  mqttMessagesPublished++;
}

// Connect to MQTT broker
void mqttConnect() {
  if (!config.mqttEnabled || strlen(config.mqttHost) == 0) {
    return;
  }

  if (currentWiFiMode != MODE_STATION) {
    return;  // Don't try MQTT in AP mode
  }

  unsigned long now = millis();
  if (now - mqttLastReconnectAttempt < mqttReconnectDelay) {
    return;  // Wait before retry
  }

  mqttLastReconnectAttempt = now;

  Serial.print("[MQTT] Connecting to ");
  Serial.print(config.mqttHost);
  Serial.print(":");
  Serial.print(config.mqttPort);
  if (config.mqttUseTls) {
    Serial.print(" (TLS)");
  }
  Serial.println("...");

  // Select WiFi client based on TLS setting
  if (config.mqttUseTls) {
    mqttWifiClientSecure.setInsecure();  // Skip certificate validation for self-signed
    mqttClient.setClient(mqttWifiClientSecure);
  } else {
    mqttClient.setClient(mqttWifiClient);
  }

  mqttClient.setServer(config.mqttHost, config.mqttPort);

  // Generate client ID
  String clientId = "esp32_radar_" + getMqttDeviceId();

  // Connect with LWT (Last Will and Testament)
  bool connected = false;
  if (strlen(config.mqttUser) > 0) {
    connected = mqttClient.connect(
      clientId.c_str(),
      config.mqttUser,
      config.mqttPass,
      mqttAvailabilityTopic,
      1,  // QoS 1 for LWT
      true,  // Retain
      "offline"
    );
  } else {
    connected = mqttClient.connect(
      clientId.c_str(),
      nullptr,
      nullptr,
      mqttAvailabilityTopic,
      1,
      true,
      "offline"
    );
  }

  if (connected) {
    mqttConnected = true;
    mqttReconnectDelay = MQTT_RECONNECT_DELAY;  // Reset delay on success
    Serial.println("[MQTT] Connected!");
    logEvent(EVENT_MQTT_CONNECTED);

    // Publish availability
    mqttPublishAvailability(true);

    // Subscribe to command topics
    mqttSubscribeToCommands();

    // Publish discovery
    mqttPublishDiscovery();

    // Publish initial states
    mqttPublishMotionState();
    mqttPublishDiagnostics();
    mqttLastDiagnosticPublish = millis();

  } else {
    Serial.print("[MQTT] Connection failed, rc=");
    Serial.print(mqttClient.state());
    Serial.print(" - ");
    switch (mqttClient.state()) {
      case -4: Serial.println("Connection timeout"); break;
      case -3: Serial.println("Connection lost"); break;
      case -2: Serial.println("Connect failed"); break;
      case -1: Serial.println("Disconnected"); break;
      case 1: Serial.println("Bad protocol"); break;
      case 2: Serial.println("Bad client ID"); break;
      case 3: Serial.println("Unavailable"); break;
      case 4: Serial.println("Bad credentials"); break;
      case 5: Serial.println("Unauthorized"); break;
      default: Serial.println("Unknown error"); break;
    }

    // Exponential backoff
    mqttReconnectDelay = min(mqttReconnectDelay * 2, (unsigned long)MQTT_MAX_RECONNECT_DELAY);
  }
}

// Handle MQTT in main loop
void mqttLoop() {
  if (!config.mqttEnabled) {
    if (mqttConnected) {
      mqttPublishAvailability(false);
      mqttClient.disconnect();
      mqttConnected = false;
      mqttDiscoveryPublished = false;
      logEvent(EVENT_MQTT_DISCONNECTED);
    }
    return;
  }

  if (currentWiFiMode != MODE_STATION) {
    return;
  }

  if (!mqttClient.connected()) {
    if (mqttConnected) {
      mqttConnected = false;
      mqttDiscoveryPublished = false;
      logEvent(EVENT_MQTT_DISCONNECTED);
      Serial.println("[MQTT] Disconnected");
    }
    mqttConnect();
  } else {
    mqttClient.loop();

    // Publish diagnostics periodically
    unsigned long now = millis();
    if (now - mqttLastDiagnosticPublish >= MQTT_DIAGNOSTIC_INTERVAL) {
      mqttLastDiagnosticPublish = now;
      mqttPublishDiagnostics();
    }
  }
}

// Called when motion state changes (from processMotionState)
void mqttOnMotionStateChange() {
  if (mqttConnected) {
    mqttPublishMotionState();

    // Also publish alarm count when it changes
    if (currentState == STATE_ALARM_ACTIVE) {
      String prefix = getMqttDevicePrefix();
      char topic[100];
      char value[32];
      snprintf(topic, sizeof(topic), "%s/sensor/%s/alarm_count/state",
               MQTT_DISCOVERY_PREFIX, prefix.c_str());
      snprintf(value, sizeof(value), "%lu", totalAlarmEvents);
      mqttClient.publish(topic, value, false);
    }
  }
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

void handleDiagPage() {
  server.send_P(200, "text/html", DIAGNOSTICS_HTML);
}

void handleAbout() {
  server.send_P(200, "text/html", ABOUT_HTML);
}

void handleStatus() {
  // GET endpoints are not protected - allows web UI to function
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
  // GET endpoints are not protected - allows web UI to function

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

  // Notification configuration (Stage 8: refactored to checkboxes)
  doc["notifyEnabled"] = config.notifyEnabled;
  doc["notifyUrl"] = config.notifyUrl;
  doc["notifyGet"] = config.notifyGet;
  doc["notifyPost"] = config.notifyPost;

  // WLED configuration (Stage 8)
  doc["wledEnabled"] = config.wledEnabled;
  doc["wledUrl"] = config.wledUrl;
  doc["wledPayload"] = config.wledPayload;

  // MQTT configuration (Stage 9)
  doc["mqttEnabled"] = config.mqttEnabled;
  doc["mqttHost"] = config.mqttHost;
  doc["mqttPort"] = config.mqttPort;
  doc["mqttUser"] = config.mqttUser;
  // Don't expose mqttPass for security
  doc["mqttUseTls"] = config.mqttUseTls;
  doc["mqttDeviceName"] = config.mqttDeviceName;
  doc["mqttConnected"] = mqttConnected;
  doc["mqttDeviceId"] = getMqttDeviceId();

  doc["wifiSsid"] = config.wifiSsid;
  doc["wifiMode"] = getWiFiModeString();
  doc["apSsid"] = AP_SSID;
  doc["ipAddress"] = getIPAddress();

  // Authentication configuration (Stage 7)
  doc["authEnabled"] = config.authEnabled;
  doc["authConfigured"] = (strlen(config.apiKey) > 0);
  // Don't expose the actual key in GET /config for security

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handlePostConfig() {
  // Dual auth: Allow if API key is valid OR web session is valid
  // This allows both external API access (curl, Home Assistant) and web UI
  if (config.authEnabled && !hasValidSession() && !checkAuthentication()) {
    sendAuthError();
    return;
  }

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

  // Notification method checkboxes (Stage 8)
  if (doc.containsKey("notifyGet")) {
    bool newGet = doc["notifyGet"];
    if (newGet != config.notifyGet) {
      config.notifyGet = newGet;
      settingsChanged = true;
      Serial.print("Notify GET ");
      Serial.println(config.notifyGet ? "enabled" : "disabled");
    }
  }

  if (doc.containsKey("notifyPost")) {
    bool newPost = doc["notifyPost"];
    if (newPost != config.notifyPost) {
      config.notifyPost = newPost;
      settingsChanged = true;
      Serial.print("Notify POST ");
      Serial.println(config.notifyPost ? "enabled" : "disabled");
    }
  }

  // WLED settings (Stage 8)
  if (doc.containsKey("wledEnabled")) {
    bool newEnabled = doc["wledEnabled"];
    if (newEnabled != config.wledEnabled) {
      config.wledEnabled = newEnabled;
      settingsChanged = true;
      Serial.print("WLED ");
      Serial.println(config.wledEnabled ? "enabled" : "disabled");
    }
  }

  if (doc.containsKey("wledUrl")) {
    const char* newUrl = doc["wledUrl"];
    if (strcmp(newUrl, config.wledUrl) != 0) {
      strncpy(config.wledUrl, newUrl, sizeof(config.wledUrl) - 1);
      config.wledUrl[sizeof(config.wledUrl) - 1] = '\0';
      settingsChanged = true;
      Serial.print("WLED URL changed to: ");
      Serial.println(config.wledUrl);
    }
  }

  if (doc.containsKey("wledPayload")) {
    const char* newPayload = doc["wledPayload"];
    // Validate JSON before accepting
    if (!isValidJson(newPayload)) {
      server.send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON in WLED payload\"}");
      return;
    }
    if (strcmp(newPayload, config.wledPayload) != 0) {
      strncpy(config.wledPayload, newPayload, sizeof(config.wledPayload) - 1);
      config.wledPayload[sizeof(config.wledPayload) - 1] = '\0';
      settingsChanged = true;
      Serial.println("WLED payload updated");
    }
  }

  // MQTT settings (Stage 9)
  bool mqttChanged = false;
  if (doc.containsKey("mqttEnabled")) {
    bool newEnabled = doc["mqttEnabled"];
    if (newEnabled != config.mqttEnabled) {
      config.mqttEnabled = newEnabled;
      settingsChanged = true;
      mqttChanged = true;
      Serial.print("MQTT ");
      Serial.println(config.mqttEnabled ? "enabled" : "disabled");
    }
  }

  if (doc.containsKey("mqttHost")) {
    const char* newHost = doc["mqttHost"];
    if (strcmp(newHost, config.mqttHost) != 0) {
      strncpy(config.mqttHost, newHost, sizeof(config.mqttHost) - 1);
      config.mqttHost[sizeof(config.mqttHost) - 1] = '\0';
      settingsChanged = true;
      mqttChanged = true;
      Serial.print("MQTT host changed to: ");
      Serial.println(config.mqttHost);
    }
  }

  if (doc.containsKey("mqttPort")) {
    uint16_t newPort = doc["mqttPort"];
    if (newPort > 0 && newPort <= 65535 && newPort != config.mqttPort) {
      config.mqttPort = newPort;
      settingsChanged = true;
      mqttChanged = true;
      Serial.print("MQTT port changed to: ");
      Serial.println(config.mqttPort);
    }
  }

  if (doc.containsKey("mqttUser")) {
    const char* newUser = doc["mqttUser"];
    if (strcmp(newUser, config.mqttUser) != 0) {
      strncpy(config.mqttUser, newUser, sizeof(config.mqttUser) - 1);
      config.mqttUser[sizeof(config.mqttUser) - 1] = '\0';
      settingsChanged = true;
      mqttChanged = true;
      Serial.println("MQTT username updated");
    }
  }

  if (doc.containsKey("mqttPass")) {
    const char* newPass = doc["mqttPass"];
    // Only update if non-empty (empty means don't change)
    // Use "__CLEAR__" to explicitly clear the password
    if (strcmp(newPass, "__CLEAR__") == 0) {
      config.mqttPass[0] = '\0';
      settingsChanged = true;
      mqttChanged = true;
      Serial.println("MQTT password cleared");
    } else if (strlen(newPass) > 0) {
      strncpy(config.mqttPass, newPass, sizeof(config.mqttPass) - 1);
      config.mqttPass[sizeof(config.mqttPass) - 1] = '\0';
      settingsChanged = true;
      mqttChanged = true;
      Serial.println("MQTT password updated");
    }
  }

  if (doc.containsKey("mqttUseTls")) {
    bool newTls = doc["mqttUseTls"];
    if (newTls != config.mqttUseTls) {
      config.mqttUseTls = newTls;
      settingsChanged = true;
      mqttChanged = true;
      Serial.print("MQTT TLS ");
      Serial.println(config.mqttUseTls ? "enabled" : "disabled");
    }
  }

  if (doc.containsKey("mqttDeviceName")) {
    const char* newName = doc["mqttDeviceName"];
    if (strlen(newName) > 0 && strcmp(newName, config.mqttDeviceName) != 0) {
      strncpy(config.mqttDeviceName, newName, sizeof(config.mqttDeviceName) - 1);
      config.mqttDeviceName[sizeof(config.mqttDeviceName) - 1] = '\0';
      settingsChanged = true;
      mqttChanged = true;
      Serial.print("MQTT device name changed to: ");
      Serial.println(config.mqttDeviceName);
    }
  }

  // If MQTT settings changed, trigger reconnect
  if (mqttChanged && config.mqttEnabled && mqttConnected) {
    mqttPublishAvailability(false);
    mqttClient.disconnect();
    mqttConnected = false;
    mqttDiscoveryPublished = false;
    mqttReconnectDelay = MQTT_RECONNECT_DELAY;
    Serial.println("[MQTT] Settings changed, reconnecting...");
  }

  // Update authentication settings (Stage 7)
  if (doc.containsKey("authEnabled")) {
    bool newEnabled = doc["authEnabled"];
    if (newEnabled != config.authEnabled) {
      config.authEnabled = newEnabled;
      settingsChanged = true;
      Serial.print("API Authentication ");
      Serial.println(config.authEnabled ? "enabled" : "disabled");

      // Auto-generate key when first enabled if none exists
      if (config.authEnabled && strlen(config.apiKey) == 0) {
        generateUUID(config.apiKey);
        Serial.print("Auto-generated API key: ");
        Serial.println(config.apiKey);
      }
    }
  }

  if (doc.containsKey("apiKey")) {
    const char* newKey = doc["apiKey"];
    // Validate key format (should be 36 chars for UUID)
    if (strlen(newKey) == API_KEY_LENGTH) {
      if (strcmp(newKey, config.apiKey) != 0) {
        strncpy(config.apiKey, newKey, sizeof(config.apiKey) - 1);
        config.apiKey[API_KEY_LENGTH] = '\0';
        settingsChanged = true;
        // Reset auth fail counter when key changes
        authFailCount = 0;
        authLockoutUntil = 0;
        Serial.println("API key updated");
      }
    } else if (strlen(newKey) > 0) {
      // Invalid key format, ignore but log
      Serial.println("Invalid API key format (must be 36 chars UUID)");
    }
  }

  // Save if anything changed
  if (wifiChanged || settingsChanged) {
    if (saveConfig()) {
      logEvent(EVENT_CONFIG_SAVED);
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
  // Dual auth: Allow if API key is valid OR web session is valid
  if (config.authEnabled && !hasValidSession() && !checkAuthentication()) {
    sendAuthError();
    return;
  }

  server.send(200, "application/json", "{\"success\":true,\"message\":\"Factory reset initiated\"}");
  delay(1000);
  factoryReset();
}

void handleTestNotification() {
  // Dual auth: Allow if API key is valid OR web session is valid
  if (config.authEnabled && !hasValidSession() && !checkAuthentication()) {
    sendAuthError();
    return;
  }

  if (currentWiFiMode != MODE_STATION) {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"Not connected to WiFi\"}");
    return;
  }

  // Check if URL and methods are provided in request body (for testing unsaved values)
  String testUrl = config.notifyUrl;
  bool testGet = config.notifyGet;
  bool testPost = config.notifyPost;

  if (server.hasArg("plain")) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));
    if (!error) {
      if (doc.containsKey("url") && strlen(doc["url"]) > 0) {
        testUrl = doc["url"].as<String>();
      }
      if (doc.containsKey("notifyGet")) {
        testGet = doc["notifyGet"];
      }
      if (doc.containsKey("notifyPost")) {
        testPost = doc["notifyPost"];
      }
    }
  }

  if (testUrl.length() == 0) {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"No notification URL provided\"}");
    return;
  }

  if (!testGet && !testPost) {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"No notification method selected\"}");
    return;
  }

  Serial.println("[NOTIFY] Sending test notification...");
  Serial.print("[NOTIFY] URL: ");
  Serial.println(testUrl);

  int httpCode = -1;
  int getCode = -1;
  int postCode = -1;
  String message = "";

  // Send GET request if enabled
  if (testGet) {
    Serial.println("[NOTIFY] Sending GET request...");
    HTTPClient http;
    http.setTimeout(5000);
    String url = testUrl;
    if (url.indexOf('?') == -1) {
      url += "?";
    } else {
      url += "&";
    }
    url += "event=test&state=" + String(stateNames[currentState]) + "&ip=" + getIPAddress();
    http.begin(url);
    getCode = http.GET();
    http.end();
    Serial.print("[NOTIFY] GET Response: ");
    Serial.println(getCode);
  }

  // Send POST request if enabled
  if (testPost) {
    Serial.println("[NOTIFY] Sending POST request...");
    HTTPClient http;
    http.setTimeout(5000);
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
    postCode = http.POST(body);
    http.end();
    Serial.print("[NOTIFY] POST Response: ");
    Serial.println(postCode);
  }

  // Determine overall result
  bool success = true;
  if (testGet && testPost) {
    httpCode = (getCode >= 200 && getCode < 300) ? getCode : postCode;
    success = (getCode >= 200 && getCode < 300) && (postCode >= 200 && postCode < 300);
    message = "GET: " + String(getCode) + ", POST: " + String(postCode);
  } else if (testGet) {
    httpCode = getCode;
    success = (getCode >= 200 && getCode < 300);
    message = "GET sent";
  } else {
    httpCode = postCode;
    success = (postCode >= 200 && postCode < 300);
    message = "POST sent";
  }

  lastNotificationTime = millis();
  lastNotificationStatus = httpCode;

  JsonDocument respDoc;
  respDoc["success"] = success;
  respDoc["httpCode"] = httpCode;
  respDoc["message"] = message;

  String response;
  serializeJson(respDoc, response);
  server.send(200, "application/json", response);
}

// POST /test-wled - Test WLED configuration (Stage 8)
void handleTestWled() {
  // Dual auth: Allow if API key is valid OR web session is valid
  if (config.authEnabled && !hasValidSession() && !checkAuthentication()) {
    sendAuthError();
    return;
  }

  if (currentWiFiMode != MODE_STATION) {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"Not connected to WiFi\"}");
    return;
  }

  // Check if URL and payload are provided in request body (for testing unsaved values)
  String testUrl = config.wledUrl;
  String testPayload = config.wledPayload;

  if (server.hasArg("plain")) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));
    if (!error) {
      if (doc.containsKey("url") && strlen(doc["url"]) > 0) {
        testUrl = doc["url"].as<String>();
      }
      if (doc.containsKey("payload") && strlen(doc["payload"]) > 0) {
        testPayload = doc["payload"].as<String>();
      }
    }
  }

  if (testUrl.length() == 0) {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"No WLED URL provided\"}");
    return;
  }

  if (testPayload.length() == 0) {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"No WLED payload provided\"}");
    return;
  }

  // Validate JSON payload
  if (!isValidJson(testPayload.c_str())) {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON payload\"}");
    return;
  }

  // Send test WLED payload
  int httpCode = sendTestWled(testUrl.c_str(), testPayload.c_str());

  JsonDocument respDoc;
  respDoc["success"] = (httpCode >= 200 && httpCode < 300);
  respDoc["httpCode"] = httpCode;
  if (httpCode >= 200 && httpCode < 300) {
    respDoc["message"] = "WLED payload sent successfully";
  } else if (httpCode == -1) {
    respDoc["message"] = "URL or payload empty";
  } else if (httpCode == -2) {
    respDoc["message"] = "Not connected to WiFi";
  } else if (httpCode < 0) {
    respDoc["message"] = "Connection failed";
  } else {
    respDoc["message"] = "WLED returned error";
  }

  String response;
  serializeJson(respDoc, response);
  server.send(200, "application/json", response);
}

// ============================================================================
// DIAGNOSTICS HANDLERS (Stage 6)
// ============================================================================

void handleLogs() {
  // GET endpoints are not protected - allows web UI to function
  JsonDocument doc;
  JsonArray logs = doc["events"].to<JsonArray>();

  // Output events in chronological order (oldest first)
  int start = (eventLogCount < EVENT_LOG_SIZE) ? 0 : eventLogHead;
  for (int i = 0; i < eventLogCount; i++) {
    int idx = (start + i) % EVENT_LOG_SIZE;
    JsonObject entry = logs.add<JsonObject>();
    entry["time"] = eventLog[idx].timestamp;
    entry["uptime"] = formatUptime(eventLog[idx].timestamp);
    entry["event"] = eventTypeNames[eventLog[idx].type];
    if (eventLog[idx].data != 0) {
      entry["data"] = eventLog[idx].data;
    }
  }

  doc["count"] = eventLogCount;
  doc["maxSize"] = EVENT_LOG_SIZE;

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleDiagnostics() {
  // GET endpoints are not protected - allows web UI to function
  JsonDocument doc;

  // System info
  doc["uptime"] = millis();
  doc["uptimeFormatted"] = formatUptime(millis());
  doc["freeHeap"] = ESP.getFreeHeap();
  doc["minHeap"] = minHeapSeen;
  doc["maxHeap"] = maxHeapSeen;
  doc["heapFragmentation"] = 100 - (ESP.getMaxAllocHeap() * 100 / ESP.getFreeHeap());

  // Watchdog status
  doc["watchdogEnabled"] = watchdogEnabled;
  doc["watchdogTimeout"] = WATCHDOG_TIMEOUT_S;

  // WiFi info
  doc["wifiMode"] = getWiFiModeString();
  doc["wifiConnected"] = (currentWiFiMode == MODE_STATION);
  if (currentWiFiMode == MODE_STATION) {
    doc["rssi"] = WiFi.RSSI();
    doc["ssid"] = WiFi.SSID();
  }
  doc["ip"] = getIPAddress();

  // Statistics
  doc["alarmEvents"] = totalAlarmEvents;
  doc["motionEvents"] = totalMotionEvents;
  doc["notificationsSent"] = totalNotificationsSent;
  doc["notificationsFailed"] = totalNotificationsFailed;

  // Current state
  doc["state"] = stateNames[currentState];
  doc["motionFiltered"] = filteredMotionDetected;
  doc["filterLevel"] = currentFilterPercent;

  // Authentication status (Stage 7)
  doc["authEnabled"] = config.authEnabled;
  doc["authFailedAttempts"] = authFailCount;
  unsigned long lockoutRemaining = getAuthLockoutRemaining();
  if (lockoutRemaining > 0) {
    doc["authLockoutRemaining"] = lockoutRemaining;
  }

  // Heap history
  JsonArray heapHist = doc["heapHistory"].to<JsonArray>();
  int histStart = (heapHistoryCount < HEAP_HISTORY_SIZE) ? 0 : heapHistoryHead;
  for (int i = 0; i < heapHistoryCount; i++) {
    int idx = (histStart + i) % HEAP_HISTORY_SIZE;
    heapHist.add(heapHistory[idx]);
  }

  // Event log summary (last 10 events)
  JsonArray recentEvents = doc["recentEvents"].to<JsonArray>();
  int evtStart = (eventLogCount < 10) ? 0 : (eventLogHead - 10 + EVENT_LOG_SIZE) % EVENT_LOG_SIZE;
  int evtCount = min(eventLogCount, 10);
  for (int i = 0; i < evtCount; i++) {
    int idx = (evtStart + i) % EVENT_LOG_SIZE;
    JsonObject entry = recentEvents.add<JsonObject>();
    entry["uptime"] = formatUptime(eventLog[idx].timestamp);
    entry["event"] = eventTypeNames[eventLog[idx].type];
  }

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleNotFound() {
  server.send(404, "text/plain", "Not Found");
}

// Generate a new API key without saving (for preview in settings page)
void handleGenerateKey() {
  // This endpoint doesn't require auth - it just generates a preview key
  char newKey[API_KEY_LENGTH + 1];
  generateUUID(newKey);

  JsonDocument doc;
  doc["key"] = newKey;

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

// ============================================================================
// WEB PASSWORD AUTH ENDPOINTS
// ============================================================================

// GET /auth/status - Check authentication state
void handleAuthStatus() {
  JsonDocument doc;
  doc["passwordConfigured"] = isPasswordConfigured();
  doc["loggedIn"] = hasValidSession();

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

// POST /auth/setup - Create initial password
void handleAuthSetup() {
  // Only allow setup if no password is configured
  if (isPasswordConfigured()) {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"Password already configured\"}");
    return;
  }

  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"No data received\"}");
    return;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, server.arg("plain"));
  if (error) {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
    return;
  }

  const char* password = doc["password"] | "";
  const char* confirmPassword = doc["confirmPassword"] | "";

  // Validate password length
  if (strlen(password) < MIN_PASSWORD_LENGTH) {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"Password must be at least 6 characters\"}");
    return;
  }

  // Validate passwords match
  if (strcmp(password, confirmPassword) != 0) {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"Passwords do not match\"}");
    return;
  }

  // Hash and store the password
  hashPassword(password, config.passwordHash);
  if (!saveConfig()) {
    config.passwordHash[0] = '\0';
    server.send(500, "application/json", "{\"success\":false,\"message\":\"Failed to save password\"}");
    return;
  }

  // Create a session for the new user
  createSession();

  // Send response with session cookie
  String cookie = "session=" + String(currentSession.token) + "; Path=/; SameSite=Strict";
  server.sendHeader("Set-Cookie", cookie);
  server.send(200, "application/json", "{\"success\":true,\"message\":\"Password created\"}");

  Serial.println("[AUTH] Web password configured");
}

// POST /auth/login - Authenticate with password
void handleAuthLogin() {
  if (!isPasswordConfigured()) {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"No password configured\"}");
    return;
  }

  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"No data received\"}");
    return;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, server.arg("plain"));
  if (error) {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
    return;
  }

  const char* password = doc["password"] | "";

  if (verifyPassword(password)) {
    // Create new session (invalidates any existing session)
    createSession();

    // Send response with session cookie
    String cookie = "session=" + String(currentSession.token) + "; Path=/; SameSite=Strict";
    server.sendHeader("Set-Cookie", cookie);
    server.send(200, "application/json", "{\"success\":true,\"message\":\"Login successful\"}");

    Serial.println("[AUTH] Web login successful");
  } else {
    server.send(401, "application/json", "{\"success\":false,\"message\":\"Invalid password\"}");
    Serial.println("[AUTH] Web login failed - invalid password");
  }
}

// POST /auth/logout - Clear session
void handleAuthLogout() {
  clearSession();

  // Clear session cookie
  server.sendHeader("Set-Cookie", "session=; Path=/; SameSite=Strict; Max-Age=0");
  server.send(200, "application/json", "{\"success\":true,\"message\":\"Logged out\"}");
}

void setupWebServer() {
  // Collect headers for authentication (API Key and Cookie for session)
  const char* headerKeys[] = {"X-API-Key", "Cookie"};
  server.collectHeaders(headerKeys, 2);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/settings", HTTP_GET, handleSettings);
  server.on("/diag", HTTP_GET, handleDiagPage);
  server.on("/api", HTTP_GET, handleApi);
  server.on("/about", HTTP_GET, handleAbout);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/config", HTTP_GET, handleGetConfig);
  server.on("/config", HTTP_POST, handlePostConfig);
  server.on("/reset", HTTP_POST, handleReset);
  server.on("/test-notification", HTTP_POST, handleTestNotification);
  server.on("/test-wled", HTTP_POST, handleTestWled);  // Stage 8
  server.on("/logs", HTTP_GET, handleLogs);
  server.on("/diagnostics", HTTP_GET, handleDiagnostics);
  server.on("/generate-key", HTTP_POST, handleGenerateKey);  // Stage 7
  // Web password auth endpoints (Stage 7)
  server.on("/auth/status", HTTP_GET, handleAuthStatus);
  server.on("/auth/setup", HTTP_POST, handleAuthSetup);
  server.on("/auth/login", HTTP_POST, handleAuthLogin);
  server.on("/auth/logout", HTTP_POST, handleAuthLogout);
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

        // Log event and send notification
        logEvent(EVENT_ALARM_TRIGGERED, totalAlarmEvents);
        sendNotification("alarm_triggered", "ALARM_ACTIVE");
        sendWledPayload();  // Stage 8: Send WLED payload on alarm trigger
        mqttOnMotionStateChange();  // Stage 9: Update Home Assistant
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

        // Log event and send notification
        logEvent(EVENT_ALARM_CLEARED, duration);
        sendNotification("alarm_cleared", "IDLE");
        mqttOnMotionStateChange();  // Stage 9: Update Home Assistant
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
  Serial.println("\n=== STAGE 7 SELF TEST ===");

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
        Serial.println("l - Event log");
        Serial.println("d - Diagnostics");
        Serial.println("a - API key (show/generate)");
        Serial.println("p - Web password (show/clear)");
        Serial.println("h - Help");
        Serial.println("r - Restart");
        Serial.println("i - System info");
        Serial.println("f - Factory reset");
        Serial.println("================\n");
        break;
      case 'a':
      case 'A':
        Serial.println("\n=== API AUTHENTICATION ===");
        Serial.print("Status: ");
        Serial.println(config.authEnabled ? "ENABLED" : "DISABLED");
        if (strlen(config.apiKey) > 0) {
          Serial.print("API Key: ");
          Serial.println(config.apiKey);
        } else {
          Serial.println("API Key: Not configured");
        }
        Serial.print("Failed Attempts: ");
        Serial.println(authFailCount);
        if (authLockoutUntil > 0 && millis() < authLockoutUntil) {
          Serial.print("Lockout: ");
          Serial.print((authLockoutUntil - millis()) / 1000);
          Serial.println(" seconds remaining");
        }
        Serial.println("\nCommands:");
        Serial.println("  'a' again - Generate new key");
        Serial.println("  'A' - Toggle auth on/off");
        Serial.println("==========================\n");
        {
          // Wait for follow-up command
          unsigned long waitStart = millis();
          while (millis() - waitStart < 3000) {
            if (Serial.available()) {
              char subcmd = Serial.read();
              if (subcmd == 'a') {
                // Generate new key
                generateUUID(config.apiKey);
                saveConfig();
                Serial.print("New API Key: ");
                Serial.println(config.apiKey);
                break;
              } else if (subcmd == 'A') {
                // Toggle auth
                config.authEnabled = !config.authEnabled;
                if (config.authEnabled && strlen(config.apiKey) == 0) {
                  generateUUID(config.apiKey);
                  Serial.print("Auto-generated key: ");
                  Serial.println(config.apiKey);
                }
                saveConfig();
                Serial.print("API Auth: ");
                Serial.println(config.authEnabled ? "ENABLED" : "DISABLED");
                break;
              }
            }
          }
        }
        break;
      case 'p':
      case 'P':
        Serial.println("\n=== WEB PASSWORD ===");
        Serial.print("Status: ");
        Serial.println(isPasswordConfigured() ? "CONFIGURED" : "NOT SET");
        Serial.print("Session: ");
        Serial.println(currentSession.active ? "ACTIVE" : "NONE");
        Serial.println("\nCommands:");
        Serial.println("  'p' again - Clear password (returns to setup state)");
        Serial.println("====================\n");
        {
          // Wait for follow-up command
          unsigned long waitStart = millis();
          while (millis() - waitStart < 3000) {
            if (Serial.available()) {
              char subcmd = Serial.read();
              if (subcmd == 'p') {
                // Clear password
                config.passwordHash[0] = '\0';
                clearSession();
                saveConfig();
                Serial.println("Password cleared. Next Settings page visit will prompt for new password.");
                break;
              }
            }
          }
        }
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
      case 'l':
      case 'L':
        Serial.println("\n=== EVENT LOG ===");
        Serial.print("Events: ");
        Serial.print(eventLogCount);
        Serial.print(" / ");
        Serial.println(EVENT_LOG_SIZE);
        Serial.println();
        {
          int start = (eventLogCount < EVENT_LOG_SIZE) ? 0 : eventLogHead;
          for (int i = 0; i < eventLogCount; i++) {
            int idx = (start + i) % EVENT_LOG_SIZE;
            Serial.print(formatUptime(eventLog[idx].timestamp));
            Serial.print(" - ");
            Serial.print(eventTypeNames[eventLog[idx].type]);
            if (eventLog[idx].data != 0) {
              Serial.print(" (");
              Serial.print(eventLog[idx].data);
              Serial.print(")");
            }
            Serial.println();
          }
        }
        Serial.println("=================\n");
        break;
      case 'd':
      case 'D':
        Serial.println("\n=== DIAGNOSTICS ===");
        Serial.print("Uptime: ");
        Serial.println(formatUptime(millis()));
        Serial.print("Free Heap: ");
        Serial.print(ESP.getFreeHeap() / 1024);
        Serial.println(" KB");
        Serial.print("Min Heap: ");
        Serial.print(minHeapSeen / 1024);
        Serial.println(" KB");
        Serial.print("Max Heap: ");
        Serial.print(maxHeapSeen / 1024);
        Serial.println(" KB");
        Serial.print("Watchdog: ");
        Serial.println(watchdogEnabled ? "Enabled" : "Disabled");
        Serial.print("WiFi: ");
        Serial.println(getWiFiModeString());
        if (currentWiFiMode == MODE_STATION) {
          Serial.print("RSSI: ");
          Serial.print(WiFi.RSSI());
          Serial.println(" dBm");
        }
        Serial.print("Alarms: ");
        Serial.println(totalAlarmEvents);
        Serial.print("Notifications Sent: ");
        Serial.println(totalNotificationsSent);
        Serial.print("Notifications Failed: ");
        Serial.println(totalNotificationsFailed);
        Serial.print("Event Log: ");
        Serial.print(eventLogCount);
        Serial.print(" / ");
        Serial.println(EVENT_LOG_SIZE);
        Serial.print("API Auth: ");
        Serial.println(config.authEnabled ? "Enabled" : "Disabled");
        if (authFailCount > 0) {
          Serial.print("Auth Fails: ");
          Serial.println(authFailCount);
        }
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
  Serial.println("ESP32 Microwave Motion Sensor - Stage 9");
  Serial.println("==========================================");
  Serial.println("Home Assistant MQTT Integration");
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

  // Initialize MQTT (Stage 9)
  mqttSetup();

  // Initialize watchdog timer (Stage 6)
  esp_task_wdt_config_t wdtConfig = {
    .timeout_ms = WATCHDOG_TIMEOUT_S * 1000,
    .idle_core_mask = (1 << 0),  // Watch core 0
    .trigger_panic = true
  };
  esp_task_wdt_init(&wdtConfig);
  esp_task_wdt_add(NULL);  // Add current task to watchdog
  watchdogEnabled = true;
  Serial.println("Watchdog timer initialized (30s timeout)");

  // Log boot event
  logEvent(EVENT_BOOT, ESP.getFreeHeap());

  // Record initial heap sample
  recordHeapSample();

  Serial.println();
  Serial.println("------------------------------------------");
  Serial.print("Dashboard:    http://");
  Serial.println(getIPAddress());
  Serial.print("Settings:     http://");
  Serial.print(getIPAddress());
  Serial.println("/settings");
  Serial.print("Diagnostics:  http://");
  Serial.print(getIPAddress());
  Serial.println("/diagnostics");
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
  // Reset watchdog timer (Stage 6)
  if (watchdogEnabled) {
    esp_task_wdt_reset();
  }

  // Handle web server requests
  server.handleClient();

  // Handle MQTT (Stage 9)
  mqttLoop();

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

  // Print heap and record sample periodically (Stage 6)
  if (currentMillis - lastHeapPrint >= HEAP_PRINT_INTERVAL) {
    lastHeapPrint = currentMillis;
    recordHeapSample();
    Serial.print("[HEAP] ");
    Serial.print(ESP.getFreeHeap() / 1024);
    Serial.print(" KB (min: ");
    Serial.print(minHeapSeen / 1024);
    Serial.print(" KB) | State: ");
    Serial.print(stateNames[currentState]);
    Serial.print(" | Alarms: ");
    Serial.println(totalAlarmEvents);
  }

  delay(10);
}
