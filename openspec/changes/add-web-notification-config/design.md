# Technical Design: Web-Based Configuration and Notification System

## Context

This change implements a complete ESP32-based presence detection system using the DFRobot C4001 24GHz mmWave sensor. The system must be production-ready, user-configurable, and integrate with external automation systems.

**Constraints:**
- ESP32-WROOM-32 limited resources (520KB SRAM, 4MB Flash)
- Arduino IDE development environment (not PlatformIO)
- I2C-only sensor communication (UART mode not used)
- Single-threaded Arduino loop model with FreeRTOS underneath
- No external database or cloud services
- Local network deployment only (no internet required)

**Stakeholders:**
- End users: Smart home enthusiasts, building automation users
- Integration targets: Home Assistant, Node-RED, custom automation systems

## Goals / Non-Goals

### Goals
- Incremental development with testable demos at each stage
- Zero hardcoded configuration in production code
- Persistent configuration across power cycles
- Non-blocking operation (async web server, fire-and-forget notifications)
- Comprehensive diagnostics for troubleshooting
- Simple, maintainable code suitable for Arduino IDE users

### Non-Goals
- Cloud connectivity or MQTT (can be added later)
- Mobile app (web interface only)
- Authentication/security (local network deployment assumed)
- OTA firmware updates (can be added later)
- Multiple sensor support (single C4001 sensor)
- Database for historical data (memory-constrained)

## Decisions

### 1. Incremental Build Stages (Feature-by-Feature)

**Decision:** Implement six distinct stages, each building on the previous and resulting in a working, compilable demo.

**Rationale:**
- Arduino users often learn by incremental experimentation
- Each stage can be tested independently before moving forward
- Easier debugging when issues arise
- Clear progression from simple (sensor reading) to complex (full system)
- Allows early validation of sensor integration before adding complexity

**Alternatives Considered:**
- Vertical slices (minimal end-to-end first): Rejected because it would create incomplete features at each stage, harder to test
- Layer-by-layer: Rejected because it delays integration testing until late stages

**Implementation:**
Each stage lives in a separate `.ino` file (e.g., `stage1_sensor_basic.ino`) allowing users to test each independently.

---

### 2. Configuration Storage: NVS (Non-Volatile Storage)

**Decision:** Use ESP32's built-in NVS via the Preferences library.

**Rationale:**
- Built into ESP32 Arduino core, no external dependencies
- Simple key-value API (`preferences.putString()`, `preferences.getString()`)
- Automatic wear leveling
- Survives firmware updates (separate partition)
- Sufficient for configuration data (<100 variables)

**Alternatives Considered:**
- **SPIFFS file system**: More complex, overkill for configuration, requires partition management
- **EEPROM**: Legacy API, manual wear leveling required, less reliable
- **External EEPROM (I2C)**: Extra hardware cost and complexity

**Implementation:**
```cpp
Preferences preferences;
preferences.begin("radar-config", false); // RW mode
String ssid = preferences.getString("wifi_ssid", "");
preferences.putString("wifi_ssid", newSsid);
preferences.end();
```

---

### 3. Web Server: ESPAsyncWebServer

**Decision:** Use ESPAsyncWebServer library for HTTP interface.

**Rationale:**
- Async operation prevents blocking the main loop during HTTP requests
- Handles multiple concurrent connections
- Built-in WebSocket support for real-time updates
- Widely adopted in ESP32 community with good documentation
- Efficient memory usage compared to synchronous ESP32WebServer

**Alternatives Considered:**
- **ESP32WebServer (synchronous)**: Blocks during requests, poor performance with sensor polling
- **Custom TCP server**: Too much effort, reinventing the wheel

**Implementation:**
- Static HTML/CSS/JS embedded in code as raw strings (no SPIFFS needed)
- REST API endpoints: `/status`, `/config`, `/diagnostics`, `/events`
- WebSocket for real-time sensor data updates (optional optimization)

---

### 4. WiFi AP Fallback Mode

**Decision:** Implement Access Point (AP) fallback mode when WiFi connection fails.

**Rationale:**
- Solves the chicken-and-egg problem: can't configure WiFi without WiFi
- Standard practice for IoT devices (ESP8266/ESP32 projects)
- User can always access device at 192.168.4.1 even without pre-configured credentials
- Enables initial setup without hardcoded WiFi credentials
- Prevents device becoming inaccessible if WiFi network changes

**Alternatives Considered:**
- **Hardcoded default credentials**: Insecure, requires code modification for different networks
- **SmartConfig/WPS**: Complex, poor user experience, not universally supported
- **Serial configuration**: Requires USB connection, not user-friendly

**Implementation:**
```cpp
// Stage 2: WiFi with AP fallback
WiFi.begin(ssid, password);
if (WiFi.waitForConnectResult(30000) != WL_CONNECTED) {
  WiFi.mode(WIFI_AP);
  WiFi.softAP("ESP32-Radar-Setup", "12345678");
  WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
  Serial.println("AP Mode: Connect to 'ESP32-Radar-Setup' at 192.168.4.1");
}
```

**User Experience:**
1. Flash firmware to ESP32
2. ESP32 attempts to connect to stored WiFi (or defaults)
3. If connection fails (first boot or wrong credentials), ESP32 becomes AP
4. User connects phone/laptop to "ESP32-Radar-Setup" network
5. User navigates to 192.168.4.1 in browser
6. User configures WiFi credentials via web interface
7. ESP32 saves credentials and reboots
8. ESP32 connects to configured WiFi network

---

### 5. I2C Pull-up Resistors (Hardware Requirement)

**Decision:** Require 4.7kΩ pull-up resistors on SDA and SCL lines.

**Rationale:**
- I2C specification requires pull-ups for open-drain bus
- ESP32 internal pull-ups (~45kΩ) too weak for reliable I2C communication
- Common beginner mistake: "I2C device not found" due to missing pull-ups
- 4.7kΩ is standard value for I2C at 100kHz
- External pull-ups improve signal integrity and reduce errors

**Alternatives Considered:**
- **ESP32 internal pull-ups**: Too weak, unreliable, causes intermittent failures
- **10kΩ resistors**: Slower rise time, not ideal but might work
- **2.2kΩ resistors**: Better for high-speed I2C, but standard is 4.7kΩ

**Implementation:**
- Hardware: Connect 4.7kΩ resistor from GPIO 21 (SDA) to 3.3V
- Hardware: Connect 4.7kΩ resistor from GPIO 22 (SCL) to 3.3V
- Document prominently in tasks.md and README.md
- Include troubleshooting: "I2C device not found → Check pull-up resistors"

---

### 6. ESP32 Partition Scheme

**Decision:** Use "Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)" partition scheme.

**Rationale:**
- NVS partition is included in this scheme (essential for configuration storage)
- 1.2MB for application code is sufficient for this project
- 1.5MB SPIFFS partition available for future expansion (not used initially)
- Default Arduino IDE option, easy to select
- Prevents NVS storage failures due to missing partition

**Alternatives Considered:**
- **"Minimal SPIFFS"**: Smaller SPIFFS, more app space, but still includes NVS
- **"No OTA"**: Maximum app space, but no NVS partition
- **Custom partition table**: Overkill, requires manual CSV file creation

**Implementation:**
- Arduino IDE: Tools > Partition Scheme > "Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)"
- Document in Stage 3 tasks (first stage using NVS)
- Include in "Stage Transition Strategy" section of tasks.md

---

### 7. Watchdog Timer for Stability

**Decision:** Enable ESP32 hardware watchdog timer with 8-second timeout.

**Rationale:**
- Automatic recovery from system hangs or infinite loops
- Essential for long-running stability (24+ hours requirement)
- Hardware watchdog resets ESP32 if software hangs
- 8-second timeout allows for legitimate long operations (WiFi reconnect, etc.)
- Standard practice for production embedded systems

**Alternatives Considered:**
- **Software watchdog only**: Can be disabled by hung code, less reliable
- **No watchdog**: System becomes unrecoverable if code hangs
- **Shorter timeout (e.g., 2 seconds)**: Too aggressive, might trigger on legitimate delays

**Implementation:**
```cpp
#include <esp_task_wdt.h>

void setup() {
  // Enable watchdog with 8-second timeout
  esp_task_wdt_init(8, true);
  esp_task_wdt_add(NULL); // Add current task
}

void loop() {
  esp_task_wdt_reset(); // Feed watchdog every iteration
  // ... rest of loop code
}
```

**Monitoring:**
- Add watchdog status to diagnostics display
- Log watchdog resets to serial on next boot
- Prevent false alarms: ensure loop() runs at least every 7 seconds

---

### 8. Serial Debugging Levels

**Decision:** Implement three debug levels (0=minimal, 1=normal, 2=verbose).

**Rationale:**
- Reduces serial spam in production (level 0)
- Enables detailed debugging during development (level 2)
- Controlled via single constant, easy to change
- Doesn't bloat code size (compiler optimizes out unused Serial.print())

**Alternatives Considered:**
- **Always verbose**: Too much serial spam, hard to find important messages
- **Compile-time #ifdef**: More complex, requires recompilation to change level
- **Runtime configuration**: Overkill, adds memory and complexity

**Implementation:**
```cpp
#define DEBUG_LEVEL 2  // 0=minimal, 1=normal, 2=verbose

#if DEBUG_LEVEL >= 1
  Serial.println("WiFi connected");
#endif

#if DEBUG_LEVEL >= 2
  Serial.print("Free heap: ");
  Serial.println(ESP.getFreeHeap());
#endif
```

**Levels:**
- **0 (Minimal)**: Errors only, production use
- **1 (Normal)**: Important events (WiFi connect, config save, notifications)
- **2 (Verbose)**: All debug info (sensor readings, heap stats, I2C operations)

---

### 9. Notification System: Fire-and-Forget

**Decision:** Implement fire-and-forget notifications with no retry logic.

**Rationale:**
- Prevents blocking the main loop if target server is down
- Simpler implementation (no state management for retries)
- Presence detection is high-frequency; missing one notification is acceptable
- User explicitly requested fire-and-forget behavior

**Alternatives Considered:**
- **Retry with timeout**: Adds complexity, can block loop, requires queue management
- **Queue-based notifications**: Requires FreeRTOS task, adds memory overhead

**Implementation:**
```cpp
// HTTP GET example
HTTPClient http;
http.setTimeout(1000); // 1-second timeout
http.begin("http://192.168.1.100:8080/notify?presence=1");
int code = http.GET();
http.end(); // Don't wait for response
```

---

### 10. Notification Payload Format: JSON

**Decision:** Use JSON for HTTP POST payloads; query parameters for GET.

**Rationale:**
- JSON is standard for REST APIs and automation platforms
- Easy parsing in Node-RED, Home Assistant, Python scripts
- Structured data prevents ambiguity
- ArduinoJson library widely available and efficient

**Alternatives Considered:**
- **Plain text**: Harder to parse, no structure
- **XML**: Verbose, higher memory usage
- **Custom binary**: Incompatible with standard tools

**Implementation:**
```json
{
  "presence": true,
  "motion": false,
  "timestamp": 1673891234
}
```

---

### 11. Detection Parameters: Debounce and Timeout

**Decision:** Implement software debounce (ignore rapid state changes) and presence timeout (delay before declaring "no presence").

**Rationale:**
- mmWave sensors can have noisy edges (brief false negatives)
- Debounce prevents notification spam on state flicker
- Presence timeout provides hysteresis (person temporarily hidden by object)
- User requested advanced parameters for tuning

**Defaults:**
- Debounce: 500ms (ignore state changes faster than this)
- Presence timeout: 10 seconds (wait 10s of no detection before declaring absent)

**Implementation:**
```cpp
unsigned long lastChangeTime = 0;
bool stablePresence = false;

if (currentPresence != stablePresence) {
  if (millis() - lastChangeTime > debounceTime) {
    stablePresence = currentPresence;
    lastChangeTime = millis();
    sendNotification(); // Only on stable change
  }
}
```

---

### 12. I2C Communication: Polling at 1Hz

**Decision:** Poll C4001 sensor at 1Hz (once per second) in main loop.

**Rationale:**
- Presence detection doesn't require high-frequency updates
- Reduces I2C bus traffic and power consumption
- Leaves CPU time for web server and notifications
- 1-second resolution acceptable for human presence (not tracking fast-moving objects)

**Alternatives Considered:**
- **10Hz polling**: Overkill for presence detection, wastes CPU
- **Interrupt-based**: C4001 doesn't expose interrupt pin
- **FreeRTOS task**: Adds complexity for minimal benefit in this use case

---

### 13. Diagnostics Event Log: Circular Buffer

**Decision:** Store last 50 events in a circular buffer (RAM only, not persisted).

**Rationale:**
- Sufficient for debugging recent issues
- Fixed memory footprint (no heap fragmentation)
- Fast access for web UI display
- Events don't need to survive reboots

**Alternatives Considered:**
- **SPIFFS log file**: Too slow, wears flash, requires file management
- **Unlimited queue**: Risk of memory overflow on ESP32

**Implementation:**
```cpp
String eventLog[50];
int eventIndex = 0;

void logEvent(String event) {
  eventLog[eventIndex] = event;
  eventIndex = (eventIndex + 1) % 50; // Circular wrap
}
```

---

### 14. HTML/CSS/JS Embedding: Raw Strings in Code

**Decision:** Embed web interface as C++ raw strings directly in .ino file.

**Rationale:**
- No SPIFFS partition configuration required (simpler for Arduino IDE users)
- Faster serving (no file I/O)
- Single-file deployment (easier to share and install)
- Total HTML+CSS+JS size <50KB, fits comfortably in program memory

**Alternatives Considered:**
- **SPIFFS**: Requires partition table configuration, complicates Arduino IDE workflow
- **External SD card**: Extra hardware, unnecessary for static content

**Implementation:**
```cpp
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head><title>ESP32 Presence Sensor</title></head>
<body>...</body>
</html>
)rawliteral";
```

---

### 15. Built-in Self-Test Functions

**Decision:** Implement automated self-test functions at each stage with serial and web interfaces.

**Rationale:**
- Provides immediate verification after flashing each stage
- Human-in-the-loop testing without manual procedures
- No external test framework required (works in Arduino IDE)
- Tests run on actual hardware, not simulation
- Catches configuration errors early (pull-ups, WiFi, NVS)
- Reduces debugging time by identifying failures immediately

**Alternatives Considered:**
- **Manual testing only**: Time-consuming, error-prone, inconsistent
- **PlatformIO + Unity**: Requires different IDE, complex setup for Arduino users
- **ArduinoUnit library**: Good option but adds dependency (offered as alternative)
- **External test harness**: Requires additional hardware/software setup

**Implementation:**

**Stage 1 - I2C and Sensor Tests:**
```cpp
bool testI2CCommunication() {
  Wire.beginTransmission(0x2A);
  byte error = Wire.endTransmission();
  Serial.print("  I2C Communication: ");
  Serial.println(error == 0 ? "PASS" : "FAIL");
  return (error == 0);
}

bool testSensorInitialization() {
  // Check if sensor responds with valid data
  bool success = sensor.begin();
  Serial.print("  Sensor Initialization: ");
  Serial.println(success ? "PASS" : "FAIL");
  return success;
}

bool testHeapMemory() {
  uint32_t freeHeap = ESP.getFreeHeap();
  bool pass = freeHeap > 100000; // > 100KB
  Serial.print("  Free Heap (");
  Serial.print(freeHeap);
  Serial.print(" bytes): ");
  Serial.println(pass ? "PASS" : "FAIL");
  return pass;
}

void runSelfTest() {
  Serial.println("\n=== STAGE 1 SELF TEST ===");
  bool allPass = true;
  allPass &= testI2CCommunication();
  allPass &= testSensorInitialization();
  allPass &= testHeapMemory();
  Serial.print("\nOverall: ");
  Serial.println(allPass ? "ALL TESTS PASSED" : "SOME TESTS FAILED");
  Serial.println("=========================\n");
}
```

**Stage 2+ - Serial Command Interface:**
```cpp
void handleSerialCommands() {
  if (Serial.available()) {
    char cmd = Serial.read();
    switch(cmd) {
      case 't': runSelfTest(); break;
      case 'h': printHelpMenu(); break;
      case 'r': ESP.restart(); break;
      case 'i': printSystemInfo(); break;
    }
  }
}

void printHelpMenu() {
  Serial.println("\n=== SERIAL COMMANDS ===");
  Serial.println("t - Run self test");
  Serial.println("h - Show this help menu");
  Serial.println("r - Restart ESP32");
  Serial.println("i - Print system info");
  Serial.println("=======================\n");
}
```

**Stage 2+ - Web Test Endpoint:**
```cpp
server.on("/run-tests", HTTP_GET, [](AsyncWebServerRequest *request){
  JsonDocument doc;
  doc["i2c"] = testI2CCommunication();
  doc["sensor"] = testSensorInitialization();
  doc["wifi"] = (WiFi.status() == WL_CONNECTED);
  doc["heap"] = testHeapMemory();
  doc["webserver"] = true; // If we're responding, server works

  String output;
  serializeJson(doc, output);
  request->send(200, "application/json", output);
});
```

**Test Categories by Stage:**

**Stage 1:**
- I2C communication (Wire.endTransmission() == 0)
- Sensor initialization (sensor.begin() success)
- Heap memory (> 100KB free)
- Sensor data reading (valid presence/motion values)

**Stage 2:**
- All Stage 1 tests
- WiFi connection (STA mode with valid credentials)
- AP fallback (when credentials invalid)
- Web server responding (can serve index)
- `/status` endpoint returns valid JSON

**Stage 3:**
- All Stage 2 tests
- NVS write success (config saves without error)
- NVS read success (config loads correctly)
- Configuration persistence (values match after reboot)

**Stage 4:**
- All Stage 3 tests
- HTTP GET notification (HTTPClient.GET() success)
- HTTP POST notification (with valid JSON payload)
- Socket notification (TCP connection succeeds)
- State change detection (notifications only on transitions)

**Stage 5:**
- All Stage 4 tests
- Debounce logic (rapid changes filtered)
- Presence timeout (hysteresis works correctly)
- Parameter validation (rejects invalid configs)

**Stage 6:**
- All Stage 5 tests
- Event logging (events added to circular buffer)
- Watchdog feeding (esp_task_wdt_reset() doesn't crash)
- Diagnostics endpoint (returns valid JSON)
- Memory leak check (heap stable over time)

**User Experience:**

1. Flash stage firmware
2. Open serial monitor
3. Self-test runs automatically on boot
4. Review PASS/FAIL results
5. Type 't' to re-run tests anytime
6. Access `/run-tests` endpoint in browser (Stage 2+)
7. Proceed to next stage only if all tests pass

**Benefits:**
- **Immediate feedback**: Know within seconds if stage works
- **Reproducible**: Same tests every time
- **Documentable**: Serial output can be saved/shared
- **Debuggable**: Failed tests pinpoint exact issue
- **Confidence**: Proceed knowing previous stages work

---

### 16. C4001 DIP Switch Configuration (Critical Hardware Requirement)

**Decision:** Prominently document and warn about DIP switch configuration requirement in all code, documentation, and troubleshooting guides.

**Rationale:**
- The C4001 sensor has 2 DIP switches on the BACK of the board that MUST be configured correctly
- **Switch 1 (Communication Mode)**: Must be set to "I2C" position (not "UART")
  - This is the **#1 cause of "NO Devices!" errors** in field testing
  - If left in UART mode, sensor will not respond on I2C bus at all
  - Many users miss this because the switches are on the back and not obvious
- **Switch 2 (Address Select)**: Selects between I2C address 0x2A or 0x2B
  - Default is 0x2A (our code uses this)
  - Only needs changing if using multiple sensors on same I2C bus
- **Real-world validation**: This issue was encountered during initial Stage 1 testing and was immediately resolved once switches were correctly configured

**Impact if Misconfigured:**
- I2C scanner will find no devices at any address
- `sensor.begin()` will fail indefinitely
- All self-tests will report FAIL
- Users will waste time checking wiring, pull-up resistors, power supply when the actual issue is a simple switch position

**Implementation:**

**In Code Header Comments:**
```cpp
/**
 * ⚠️  CRITICAL: DIP SWITCH CONFIGURATION REQUIRED! ⚠️
 *
 * The C4001 sensor has 2 DIP switches on the BACK of the board:
 *
 * 1. COMMUNICATION MODE SWITCH: Must be set to "I2C" position (NOT "UART")
 *    - This is the #1 cause of "NO Devices!" errors
 *    - Look for switch labeled "I2C/UART" or similar
 *
 * 2. ADDRESS SELECT SWITCH: Must match code setting (default 0x2A)
 *    - Toggle this switch if using multiple sensors
 *    - Our code uses SENSOR_I2C_ADDR = 0x2A
 *
 * If you see "NO Devices!" error, CHECK THE DIP SWITCHES FIRST!
 */
```

**In Documentation:**
- Add "⚠️ CRITICAL: DIP SWITCH CONFIGURATION REQUIRED!" section at top of README
- Make it the FIRST step in hardware setup checklist
- Add to acceptance criteria for Stage 1
- Include in troubleshooting guide as #1 item to check
- Add photo/diagram showing switch positions (if available)

**In Error Messages:**
```cpp
while (!sensor.begin()) {
  Serial.println("NO Devices! Retrying in 2 seconds...");
  Serial.println("Check:");
  Serial.println("  - DIP switch on sensor back is set to I2C mode");
  Serial.println("  - Sensor is powered (VCC and GND connected)");
  Serial.println("  - 4.7kΩ pull-up resistors installed on SDA/SCL");
  delay(2000);
}
```

**Alternatives Considered:**
- **UART mode instead**: Rejected because I2C is simpler (2 wires vs 2 wires + separate power), already chosen for project
- **Auto-detect mode**: Not possible - ESP32 can't detect if switch is in wrong position, it just sees no device
- **Software-only solution**: Not applicable - this is a hardware configuration issue

**Benefits:**
- Prevents frustration from most common setup mistake
- Reduces debugging time from hours to seconds
- Clear, actionable error messages
- Sets expectation upfront before users start wiring
- Improves first-time success rate for Stage 1

**Trade-offs:**
- None - this is purely about better documentation and error messaging
- Adds a few lines of warning comments to code (acceptable)

---

## Risks / Trade-offs

### Risk: Memory Exhaustion
**Mitigation:**
- Monitor free heap via `ESP.getFreeHeap()` and display in diagnostics
- Use `PROGMEM` for large strings (HTML, constants)
- Limit event log size (50 entries max)
- Test long-running stability (24+ hours)

### Risk: WiFi Disconnection
**Mitigation:**
- Implement WiFi reconnection logic in main loop
- Continue sensor operation even when WiFi is down
- Display WiFi status in diagnostics
- Don't block on WiFi connection during boot (timeout after 30 seconds)

### Risk: I2C Communication Failures
**Mitigation:**
- Check return values from all Wire operations
- Log I2C errors to event log
- Display I2C health indicator in web UI
- Attempt I2C re-initialization if failures persist

### Risk: Notification Endpoint Unreachable
**Mitigation:**
- Use fire-and-forget with short timeout (1 second)
- Log notification failures to event log
- Continue operation even if notifications fail
- Provide test notification button in web UI for troubleshooting

### Trade-off: No Authentication
**Impact:** Web interface is open to anyone on local network
**Justification:** Simplifies initial implementation, local network deployment assumed trusted
**Future:** Can add basic auth in later enhancement if needed

---

## Migration Plan

This is a new project with no existing deployment, so no migration is required.

### First-Time Setup
1. Install Arduino IDE and ESP32 board support
2. Install required libraries (DFRobot_C4001, ESPAsyncWebServer, AsyncTCP)
3. Connect C4001 sensor to ESP32 (VCC, GND, SDA, SCL with pull-ups)
4. Flash Stage 1 firmware to test sensor communication
5. Progress through stages, testing each before moving forward
6. At Stage 3, configure WiFi credentials via serial or hardcoded default
7. From Stage 3 onward, all configuration done via web interface

### Rollback
If issues arise, flash previous stage firmware to revert functionality.

---

## Open Questions

1. **Does the C4001 library expose sensitivity/gain configuration?**
   - Need to check DFRobot_C4001 library API documentation
   - May need to configure via UART initially if not available via I2C
   - Fallback: Omit sensitivity parameter if not supported

2. **What is the realistic memory footprint of ESPAsyncWebServer?**
   - Test in Stage 2 with heap monitoring
   - May need to reduce event log size or simplify HTML if memory tight

3. **Should WebSocket be implemented in Stage 2 or Stage 6?**
   - Recommend Stage 6 (final integration) to keep Stage 2 simple
   - Can do polling-based updates in earlier stages

4. **Should notifications include distance/speed data or just presence/motion flags?**
   - User requested "basic status only" (presence/motion)
   - Can add optional extended payload in future enhancement

---

## Success Criteria

- All six build stages compile and run successfully
- Each stage demonstrates new features as described
- Configuration persists across reboots (Stage 3+)
- Notifications delivered successfully (Stage 4+)
- Web interface accessible and responsive (Stage 2+)
- System runs stably for 24+ hours without crashes or memory leaks
- Documentation sufficient for Arduino users to replicate
