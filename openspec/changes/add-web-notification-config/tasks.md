# Implementation Tasks

## Stage Transition Strategy

Each stage is implemented as a separate `.ino` file to preserve working versions as you progress.

**To transition between stages:**
1. Copy the previous stage file to a new stage file
   - Example: `cp stage1_sensor_basic.ino stage2_wifi_web.ino`
2. Implement the new stage tasks in the copied file
3. Test the new stage thoroughly against its acceptance criteria
4. Keep the previous stage file as backup/reference
5. **Recommended**: Commit to git after each successful stage

**Arduino IDE Configuration:**
- Board: "ESP32 Dev Module" or "ESP32-WROOM-DA Module"
- Upload Speed: 921600
- Flash Frequency: 80MHz
- Partition Scheme: "Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)"
- Serial Monitor Baud: 115200

**Library Versions:**
- ESP32 Arduino Core: 2.0.x (tested on 2.0.14)
- DFRobot_C4001: Latest from Library Manager
- ESPAsyncWebServer: Latest from GitHub (me-no-dev/ESPAsyncWebServer)
- AsyncTCP: Latest from GitHub (me-no-dev/AsyncTCP)
- ArduinoJson: 6.x (tested on 6.21.3)

---

## Stage 1: Basic Sensor Interface (Serial Output)
**Goal**: Read C4001 sensor via I2C and output data to serial console

### Hardware Setup
- [ ] 1.1 **⚠️ CRITICAL: Configure DIP switches on C4001 sensor (BACK of board):**
  - [ ] 1.1.1 **Set Communication Mode switch to "I2C" position (NOT "UART")**
  - [ ] 1.1.2 **Set Address Select switch to "0x2A" position (default)**
  - [ ] 1.1.3 Verify both switches are correctly positioned before proceeding
  - [ ] ⚠️ **COMMON ERROR**: Leaving switch in UART mode causes "NO Devices!" error
- [ ] 1.2 Install I2C pull-up resistors:
  - [ ] 1.2.1 Connect 4.7kΩ resistor between GPIO 21 (SDA) and 3.3V
  - [ ] 1.2.2 Connect 4.7kΩ resistor between GPIO 22 (SCL) and 3.3V
  - [ ] 1.2.3 Verify resistor connections with multimeter (~4.7kΩ to 3.3V rail)
- [ ] 1.3 Connect C4001 sensor to ESP32:
  - [ ] 1.3.1 VCC → 3.3V (or 5V)
  - [ ] 1.3.2 GND → GND
  - [ ] 1.3.3 SDA → GPIO 21
  - [ ] 1.3.4 SCL → GPIO 22

### Library Installation
- [ ] 1.4 Open Arduino IDE Library Manager (Tools > Manage Libraries)
- [ ] 1.5 Install "DFRobot_C4001" library
- [ ] 1.6 Verify library installation by checking Examples menu

### Software Implementation
- [ ] 1.7 Create `stage1_sensor_basic.ino` sketch with DIP switch warning in header comments
- [ ] 1.8 Add includes: Wire.h, DFRobot_C4001.h
- [ ] 1.9 Initialize I2C in `setup()` with Wire.begin(21, 22)
- [ ] 1.10 Initialize C4001 sensor with I2C address 0x2A
- [ ] 1.11 Implement sensor polling loop in `loop()` (1Hz read rate using millis())
- [ ] 1.12 Read presence and motion status from sensor
- [ ] 1.13 Output sensor data to serial console (115200 baud)
- [ ] 1.14 Add error handling for I2C communication failures
- [ ] 1.15 Add free heap memory monitoring (print ESP.getFreeHeap() every 10 seconds)

### Self-Test Implementation
- [ ] 1.16 Implement `testI2CCommunication()` function:
  - [ ] 1.16.1 Call Wire.beginTransmission(0x2A)
  - [ ] 1.16.2 Check Wire.endTransmission() returns 0
  - [ ] 1.16.3 Print "PASS" or "FAIL" to serial
  - [ ] 1.16.4 Return boolean result
- [ ] 1.17 Implement `testSensorInitialization()` function:
  - [ ] 1.17.1 Call sensor.begin() and check return value
  - [ ] 1.17.2 Print "PASS" or "FAIL" to serial
  - [ ] 1.17.3 Return boolean result
- [ ] 1.18 Implement `testHeapMemory()` function:
  - [ ] 1.18.1 Get ESP.getFreeHeap()
  - [ ] 1.18.2 Check if > 100KB (100000 bytes)
  - [ ] 1.18.3 Print heap size and "PASS" or "FAIL"
  - [ ] 1.18.4 Return boolean result
- [ ] 1.19 Implement `testSensorReading()` function:
  - [ ] 1.19.1 Read presence and motion from sensor
  - [ ] 1.19.2 Check values are valid (not error codes)
  - [ ] 1.19.3 Print "PASS" or "FAIL" to serial
  - [ ] 1.19.4 Return boolean result
- [ ] 1.20 Implement `runSelfTest()` function:
  - [ ] 1.20.1 Print "=== STAGE 1 SELF TEST ===" header
  - [ ] 1.20.2 Call all test functions and track pass/fail
  - [ ] 1.20.3 Print overall result (ALL TESTS PASSED or SOME TESTS FAILED)
  - [ ] 1.20.4 Print separator line
- [ ] 1.21 Call `runSelfTest()` at end of `setup()` (runs automatically on boot)
- [ ] 1.22 Add serial command 't' in loop() to re-run self-test on demand

### Testing
- [ ] 1.23 **Verify DIP switches are correctly configured (I2C mode, 0x2A address)**
- [ ] 1.24 Verify sketch compiles with 0 errors and 0 warnings
- [ ] 1.25 Check sketch size is under 80% of program memory (shown in Arduino IDE after compile)
- [ ] 1.26 Upload to ESP32-WROOM-32
- [ ] 1.27 Open serial monitor (115200 baud)
- [ ] 1.28 Verify self-test runs automatically and shows results
- [ ] 1.29 Type 't' in serial monitor to re-run self-test
- [ ] 1.30 Verify all self-tests PASS (if FAIL, check DIP switches first!)
- [ ] 1.31 Verify serial output shows presence/motion detection
- [ ] 1.32 Test presence detection by moving near sensor
- [ ] 1.33 Verify free heap is above 100KB

### Acceptance Criteria
- [ ] **DIP switches configured correctly (I2C mode, address 0x2A)**
- [ ] Sketch compiles with 0 errors, 0 warnings
- [ ] Upload successful to ESP32
- [ ] Self-test runs on boot and all tests PASS
- [ ] Serial command 't' re-runs tests successfully
- [ ] Serial monitor displays timestamped sensor readings at 1Hz
- [ ] Presence detection changes when person enters/leaves range
- [ ] Motion detection responds to movement
- [ ] Free heap memory > 100KB
- [ ] No I2C errors in serial output
- [ ] Sketch size < 80% of program memory

**Demo**: Serial monitor displays real-time presence and motion status

---

## Stage 2: WiFi and Basic Web Interface
**Goal**: Add WiFi connectivity with AP fallback and basic web dashboard

### Library Installation
- [ ] 2.1 Install ESPAsyncWebServer library:
  - [ ] 2.1.1 Download from https://github.com/me-no-dev/ESPAsyncWebServer
  - [ ] 2.1.2 Add .zip library via Arduino IDE (Sketch > Include Library > Add .ZIP Library)
- [ ] 2.2 Install AsyncTCP library:
  - [ ] 2.2.1 Download from https://github.com/me-no-dev/AsyncTCP
  - [ ] 2.2.2 Add .zip library via Arduino IDE
- [ ] 2.3 Install ArduinoJson library via Library Manager (version 6.x)

### Software Implementation
- [ ] 2.4 Create `stage2_wifi_web.ino` (copy stage1 + add WiFi)
- [ ] 2.5 Add includes: WiFi.h, ESPAsyncWebServer.h, ArduinoJson.h
- [ ] 2.6 Add WiFi credentials constants (SSID, password) with default placeholders
- [ ] 2.7 Implement WiFi connection with timeout (30 seconds max)
- [ ] 2.8 Implement AP fallback mode:
  - [ ] 2.8.1 If WiFi connection fails, start AP mode with SSID "ESP32-Radar-Setup"
  - [ ] 2.8.2 Set AP password to "12345678" (or open if preferred)
  - [ ] 2.8.3 Configure AP IP address to 192.168.4.1
  - [ ] 2.8.4 Print AP SSID and IP to serial
  - [ ] 2.8.5 Add status variable to track WiFi mode (STA vs AP)
- [ ] 2.9 Initialize ESPAsyncWebServer on port 80
- [ ] 2.10 Create HTML dashboard (embedded PROGMEM string) with:
  - [ ] 2.10.1 Current presence status (Yes/No)
  - [ ] 2.10.2 Current motion status (Detected/None)
  - [ ] 2.10.3 ESP32 IP address and WiFi mode display
  - [ ] 2.10.4 Last update timestamp
  - [ ] 2.10.5 Basic CSS styling for readability
  - [ ] 2.10.6 WiFi configuration form (for AP mode initial setup)
  - [ ] 2.10.7 Auto-refresh every 5 seconds (HTML meta refresh or JavaScript)
- [ ] 2.11 Create `/status` JSON API endpoint using ArduinoJson
- [ ] 2.12 Create `/setwifi` POST endpoint to accept new WiFi credentials (for AP mode)
- [ ] 2.13 Add WiFi reconnection logic in main loop (check if disconnected, retry)

### Self-Test Implementation
- [ ] 2.14 Copy all test functions from Stage 1 (testI2C, testSensor, testHeap)
- [ ] 2.15 Implement `testWiFiConnection()` function:
  - [ ] 2.15.1 Check WiFi.status() == WL_CONNECTED
  - [ ] 2.15.2 Print connection status and SSID
  - [ ] 2.15.3 Print "PASS" or "FAIL" to serial
  - [ ] 2.15.4 Return boolean result
- [ ] 2.16 Implement `testAPFallback()` function:
  - [ ] 2.16.1 Check if in AP mode when expected
  - [ ] 2.16.2 Verify AP IP is 192.168.4.1
  - [ ] 2.16.3 Print "PASS" or "FAIL" to serial
  - [ ] 2.16.4 Return boolean result
- [ ] 2.17 Implement `testWebServer()` function:
  - [ ] 2.17.1 Check if ESPAsyncWebServer is running
  - [ ] 2.17.2 Try to build test response (without sending)
  - [ ] 2.17.3 Print "PASS" or "FAIL" to serial
  - [ ] 2.17.4 Return boolean result
- [ ] 2.18 Implement `testStatusEndpoint()` function:
  - [ ] 2.18.1 Build /status JSON response
  - [ ] 2.18.2 Verify JSON is valid and contains required fields
  - [ ] 2.18.3 Print "PASS" or "FAIL" to serial
  - [ ] 2.18.4 Return boolean result
- [ ] 2.19 Update `runSelfTest()` to include new tests (change header to "STAGE 2")
- [ ] 2.20 Implement `handleSerialCommands()` function:
  - [ ] 2.20.1 Check Serial.available()
  - [ ] 2.20.2 Add 't' command to run self-test
  - [ ] 2.20.3 Add 'h' command to print help menu
  - [ ] 2.20.4 Add 'r' command to restart ESP32
  - [ ] 2.20.5 Add 'i' command to print system info (IP, heap, etc.)
- [ ] 2.21 Implement `printHelpMenu()` function listing all commands
- [ ] 2.22 Call `handleSerialCommands()` in main loop
- [ ] 2.23 Create `/run-tests` endpoint:
  - [ ] 2.23.1 Create JSON document with test results
  - [ ] 2.23.2 Run all test functions (without printing to serial)
  - [ ] 2.23.3 Add results to JSON (true/false for each test)
  - [ ] 2.23.4 Respond with 200 and JSON payload

### Testing
- [ ] 2.14 Verify sketch compiles with 0 errors and 0 warnings
- [ ] 2.15 Check sketch size < 80% program memory
- [ ] 2.16 Test WiFi connection with correct credentials (STA mode)
- [ ] 2.17 Verify web interface accessible at ESP32 IP address
- [ ] 2.18 Test AP fallback mode with incorrect/missing credentials
- [ ] 2.19 Connect to "ESP32-Radar-Setup" AP and access 192.168.4.1
- [ ] 2.20 Configure WiFi credentials via AP mode web interface
- [ ] 2.21 Verify ESP32 reconnects to configured WiFi after reboot
- [ ] 2.22 Test `/status` endpoint returns valid JSON
- [ ] 2.23 Verify dashboard shows live sensor data
- [ ] 2.24 Check free heap > 100KB with web server running

### Acceptance Criteria
- [ ] Sketch compiles with 0 errors, 0 warnings
- [ ] Upload successful to ESP32
- [ ] WiFi connects successfully in STA mode with valid credentials
- [ ] AP fallback activates when WiFi connection fails
- [ ] Web interface accessible at ESP32 IP (STA) or 192.168.4.1 (AP)
- [ ] Dashboard displays real-time sensor data
- [ ] `/status` endpoint returns valid JSON with presence and motion fields
- [ ] WiFi configuration can be changed via web interface in AP mode
- [ ] Free heap memory > 100KB
- [ ] Sketch size < 80% program memory
- [ ] Sensor readings continue without interruption during web requests

**Demo**: Access web interface from browser, see live presence/motion status. Test AP fallback by providing invalid WiFi credentials.

---

## Stage 3: Persistent Configuration Storage
**Goal**: Add NVS storage for WiFi credentials and basic settings

### Arduino IDE Configuration
- [ ] 3.1 Configure partition scheme:
  - [ ] 3.1.1 Select Tools > Partition Scheme > "Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)"
  - [ ] 3.1.2 Verify selection before compiling

### Software Implementation
- [ ] 3.2 Create `stage3_persistence.ino` (copy stage2 + add NVS)
- [ ] 3.3 Add include: Preferences.h
- [ ] 3.4 Create configuration structure (struct) with fields:
  - [ ] 3.4.1 WiFi SSID (String)
  - [ ] 3.4.2 WiFi password (String)
  - [ ] 3.4.3 Device name (String)
  - [ ] 3.4.4 Detection enabled flag (bool)
- [ ] 3.5 Implement `loadConfig()` function:
  - [ ] 3.5.1 Open Preferences in read-only mode
  - [ ] 3.5.2 Read each config field with default values
  - [ ] 3.5.3 Close Preferences
  - [ ] 3.5.4 Print loaded config to serial
- [ ] 3.6 Implement `saveConfig()` function:
  - [ ] 3.6.1 Open Preferences in read-write mode
  - [ ] 3.6.2 Write each config field
  - [ ] 3.6.3 Close Preferences
  - [ ] 3.6.4 Print "Config saved" to serial
- [ ] 3.7 Call `loadConfig()` early in `setup()` before WiFi initialization
- [ ] 3.8 Use loaded WiFi credentials instead of hardcoded values
- [ ] 3.9 Add web UI form for configuration:
  - [ ] 3.9.1 WiFi SSID input field
  - [ ] 3.9.2 WiFi password input field (type="password")
  - [ ] 3.9.3 Device name input field
  - [ ] 3.9.4 Detection enable/disable checkbox
  - [ ] 3.9.5 Save button
  - [ ] 3.9.6 Factory reset button with JavaScript confirmation prompt
- [ ] 3.10 Create `/config` POST endpoint to save settings:
  - [ ] 3.10.1 Parse JSON body
  - [ ] 3.10.2 Validate input fields
  - [ ] 3.10.3 Call `saveConfig()`
  - [ ] 3.10.4 Respond with success/error message
  - [ ] 3.10.5 Optionally trigger WiFi reconnection if credentials changed
- [ ] 3.11 Create `/config` GET endpoint to retrieve settings:
  - [ ] 3.11.1 Return current config as JSON
  - [ ] 3.11.2 Redact WiFi password (return empty string or asterisks)
- [ ] 3.12 Implement factory reset function:
  - [ ] 3.12.1 Clear all Preferences data
  - [ ] 3.12.2 Respond with confirmation
  - [ ] 3.12.3 Restart ESP32 after 2-second delay
- [ ] 3.13 Create `/reset` POST endpoint for factory reset

### Self-Test Implementation
- [ ] 3.14 Copy all test functions from Stage 2
- [ ] 3.15 Add `testNVSWrite()`: Write test config to NVS, check for errors
- [ ] 3.16 Add `testNVSRead()`: Read config from NVS, verify matches written values
- [ ] 3.17 Add `testConfigPersistence()`: Save config, restart flag test (manual verify after reboot)
- [ ] 3.18 Update `runSelfTest()` to include NVS tests (change header to "STAGE 3")
- [ ] 3.19 Update `/run-tests` endpoint to include NVS test results

### Testing
- [ ] 3.14 Verify sketch compiles with 0 errors and 0 warnings
- [ ] 3.15 Check sketch size < 80% program memory
- [ ] 3.16 Upload and verify default config loaded on first boot
- [ ] 3.17 Change WiFi settings via web UI
- [ ] 3.18 Verify config saved to NVS (check serial output)
- [ ] 3.19 Reboot ESP32 (power cycle)
- [ ] 3.20 Verify ESP32 connects with new WiFi credentials
- [ ] 3.21 Test `/config` GET endpoint returns saved settings
- [ ] 3.22 Test factory reset clears all settings
- [ ] 3.23 Check free heap > 100KB

### Acceptance Criteria
- [ ] Sketch compiles with 0 errors, 0 warnings
- [ ] Upload successful to ESP32
- [ ] Configuration loads from NVS on boot
- [ ] Configuration can be updated via web UI
- [ ] Configuration persists across reboots
- [ ] Configuration survives power cycle
- [ ] Factory reset clears all stored data and restarts ESP32
- [ ] WiFi credentials work after being changed and saved
- [ ] Free heap memory > 100KB
- [ ] Sketch size < 80% program memory
- [ ] All features from Stage 2 still work

**Demo**: Change WiFi settings via web, reboot ESP32, verify it connects with new settings. Test factory reset.

---

## Stage 4: Notification System
**Goal**: Implement HTTP GET/POST and raw socket notifications

### Library Note
- [ ] 4.1 Verify HTTPClient.h is available (built-in with ESP32 core, no install needed)
- [ ] 4.2 Verify WiFiClient.h is available (built-in, for socket notifications)

### Software Implementation
- [ ] 4.3 Create `stage4_notifications.ino` (copy stage3 + add notifications)
- [ ] 4.4 Add includes: HTTPClient.h, WiFiClient.h
- [ ] 4.5 Extend configuration structure with:
  - [ ] 4.5.1 Notification enabled flag (bool)
  - [ ] 4.5.2 Notification type enum/int (GET=0, POST=1, SOCKET=2)
  - [ ] 4.5.3 Target IP address (String)
  - [ ] 4.5.4 Target port (int, default 8080)
  - [ ] 4.5.5 URL path (String, default "/notify")
- [ ] 4.6 Update `loadConfig()` and `saveConfig()` for new fields
- [ ] 4.7 Implement `sendNotificationHTTPGet()` function:
  - [ ] 4.7.1 Build URL with query parameters (presence, motion)
  - [ ] 4.7.2 Create HTTPClient instance
  - [ ] 4.7.3 Set 1-second timeout
  - [ ] 4.7.4 Perform GET request
  - [ ] 4.7.5 Don't wait for full response (fire-and-forget)
  - [ ] 4.7.6 Close connection immediately
  - [ ] 4.7.7 Return success/failure status
- [ ] 4.8 Implement `sendNotificationHTTPPost()` function:
  - [ ] 4.8.1 Create JSON payload with ArduinoJson (presence, motion, timestamp)
  - [ ] 4.8.2 Set Content-Type: application/json
  - [ ] 4.8.3 Set 1-second timeout
  - [ ] 4.8.4 Perform POST request
  - [ ] 4.8.5 Don't wait for full response (fire-and-forget)
  - [ ] 4.8.6 Close connection immediately
  - [ ] 4.8.7 Return success/failure status
- [ ] 4.9 Implement `sendNotificationSocket()` function:
  - [ ] 4.9.1 Create WiFiClient instance
  - [ ] 4.9.2 Set 1-second connection timeout
  - [ ] 4.9.3 Connect to target IP and port
  - [ ] 4.9.4 Send plain text message: "presence=X,motion=Y\n"
  - [ ] 4.9.5 Close connection immediately
  - [ ] 4.9.6 Return success/failure status
- [ ] 4.10 Implement state change detection:
  - [ ] 4.10.1 Track previous presence state
  - [ ] 4.10.2 Detect state transitions (false→true, true→false)
  - [ ] 4.10.3 Trigger notification only on state change
- [ ] 4.11 Create `sendNotification()` wrapper function:
  - [ ] 4.11.1 Check if notifications enabled
  - [ ] 4.11.2 Call appropriate notification function based on type
  - [ ] 4.11.3 Log result to serial
- [ ] 4.12 Add web UI configuration panel for notifications:
  - [ ] 4.12.1 Enable/disable toggle (checkbox)
  - [ ] 4.12.2 Type selector dropdown (GET/POST/SOCKET)
  - [ ] 4.12.3 IP address input (text, with validation pattern)
  - [ ] 4.12.4 Port input (number, 1-65535)
  - [ ] 4.12.5 URL path input (text, for HTTP methods)
  - [ ] 4.12.6 "Test Notification" button
  - [ ] 4.12.7 Status indicator showing last notification result
- [ ] 4.13 Update `/config` endpoints to include notification settings
- [ ] 4.14 Create `/test-notification` POST endpoint:
  - [ ] 4.14.1 Send test notification with test flag in payload
  - [ ] 4.14.2 Return success/failure result immediately
- [ ] 4.15 Add notification status display to web dashboard

### Self-Test Implementation
- [ ] 4.16 Copy all test functions from Stage 3
- [ ] 4.17 Add `testHTTPGet()`: Send test GET request to configured endpoint, check return code
- [ ] 4.18 Add `testHTTPPost()`: Send test POST with JSON, verify no errors
- [ ] 4.19 Add `testSocketConnection()`: Test TCP socket connection, send test message
- [ ] 4.20 Add `testStateChangeDetection()`: Simulate state changes, verify notifications triggered only on transitions
- [ ] 4.21 Update `runSelfTest()` to include notification tests (change header to "STAGE 4")
- [ ] 4.22 Update `/run-tests` endpoint to include notification test results
- [ ] 4.23 Note: Network tests require valid endpoint; document that these may FAIL if endpoint not configured

### Testing
- [ ] 4.16 Verify sketch compiles with 0 errors and 0 warnings
- [ ] 4.17 Check sketch size < 80% program memory
- [ ] 4.18 Set up test HTTP server to receive notifications (e.g., nc -l 8080 or Python SimpleHTTPServer)
- [ ] 4.19 Configure notification endpoint via web UI
- [ ] 4.20 Test HTTP GET notifications:
  - [ ] 4.20.1 Trigger presence detection
  - [ ] 4.20.2 Verify GET request received at test server
  - [ ] 4.20.3 Verify query parameters correct
- [ ] 4.21 Test HTTP POST notifications:
  - [ ] 4.21.1 Trigger presence detection
  - [ ] 4.21.2 Verify POST request received
  - [ ] 4.21.3 Verify JSON payload is valid and correct
- [ ] 4.22 Test raw socket notifications:
  - [ ] 4.22.1 Set up netcat listener: nc -l 8080
  - [ ] 4.22.2 Trigger presence detection
  - [ ] 4.22.3 Verify message received
- [ ] 4.23 Test "Test Notification" button
- [ ] 4.24 Test notification failure handling (invalid IP/port)
- [ ] 4.25 Verify sensor operation continues if notifications fail
- [ ] 4.26 Check free heap > 100KB

### Acceptance Criteria
- [ ] Sketch compiles with 0 errors, 0 warnings
- [ ] Upload successful to ESP32
- [ ] HTTP GET notifications send successfully
- [ ] HTTP POST notifications send with valid JSON
- [ ] Raw socket notifications send successfully
- [ ] Notifications trigger only on presence state change
- [ ] Test notification button works
- [ ] Notification failures don't crash system
- [ ] Notification configuration persists across reboots
- [ ] Free heap memory > 100KB
- [ ] Sketch size < 80% program memory
- [ ] All features from Stage 3 still work

**Demo**: Configure notification endpoint, trigger presence detection, verify notification received at test server.

---

## Stage 5: Advanced Detection Parameters
**Goal**: Add configurable sensitivity, zones, and timeouts

### Software Implementation
- [ ] 5.1 Create `stage5_advanced_params.ino` (copy stage4 + add parameters)
- [ ] 5.2 Extend configuration structure with:
  - [ ] 5.2.1 Sensitivity level (int, if supported by C4001 - check library docs)
  - [ ] 5.2.2 Minimum detection distance in meters (float, default 1.2)
  - [ ] 5.2.3 Maximum detection distance in meters (float, default 8.0)
  - [ ] 5.2.4 Debounce time in milliseconds (int, default 500)
  - [ ] 5.2.5 Presence timeout in seconds (int, default 10)
- [ ] 5.3 Update `loadConfig()` and `saveConfig()` for new fields
- [ ] 5.4 Implement debounce logic:
  - [ ] 5.4.1 Track last state change timestamp
  - [ ] 5.4.2 Only accept state changes after debounce time elapsed
  - [ ] 5.4.3 Filter out rapid state transitions
- [ ] 5.5 Implement presence timeout:
  - [ ] 5.5.1 Track timestamp when presence last detected
  - [ ] 5.5.2 Maintain presence state as TRUE until timeout expires
  - [ ] 5.5.3 Only change to FALSE after continuous no-presence for timeout duration
- [ ] 5.6 Apply distance filtering (if C4001 library provides distance data):
  - [ ] 5.6.1 Read distance from sensor
  - [ ] 5.6.2 Ignore detections outside configured min/max range
  - [ ] 5.6.3 Log filtered detections to serial
- [ ] 5.7 Investigate C4001 sensitivity configuration:
  - [ ] 5.7.1 Check DFRobot_C4001 library for sensitivity methods
  - [ ] 5.7.2 If available, implement sensitivity setting
  - [ ] 5.7.3 If not available via I2C, document limitation
- [ ] 5.8 Create web UI panel for detection parameters:
  - [ ] 5.8.1 Sensitivity slider (if applicable, 1-10 scale)
  - [ ] 5.8.2 Minimum distance input (number, 1.2-12.0)
  - [ ] 5.8.3 Maximum distance input (number, 1.2-12.0)
  - [ ] 5.8.4 Debounce time input (number, 0-5000 ms)
  - [ ] 5.8.5 Presence timeout input (number, 0-300 seconds)
  - [ ] 5.8.6 Real-time parameter preview/description
  - [ ] 5.8.7 Input validation with helpful error messages
- [ ] 5.9 Update `/config` endpoints for new parameters
- [ ] 5.10 Add parameter validation:
  - [ ] 5.10.1 Check min distance < max distance
  - [ ] 5.10.2 Check values within sensor range limits
  - [ ] 5.10.3 Return error message if invalid

### Self-Test Implementation
- [ ] 5.11 Copy all test functions from Stage 4
- [ ] 5.12 Add `testDebounceLogic()`: Simulate rapid state changes, verify filtered
- [ ] 5.13 Add `testPresenceTimeout()`: Simulate presence then absence, verify timeout delay works
- [ ] 5.14 Add `testParameterValidation()`: Try invalid configs (min>max), verify rejection
- [ ] 5.15 Add `testDistanceFiltering()`: If sensor provides distance, test min/max filtering
- [ ] 5.16 Update `runSelfTest()` to include detection parameter tests (change header to "STAGE 5")
- [ ] 5.17 Update `/run-tests` endpoint to include parameter test results

### Testing
- [ ] 5.11 Verify sketch compiles with 0 errors and 0 warnings
- [ ] 5.12 Check sketch size < 80% program memory
- [ ] 5.13 Test debounce logic:
  - [ ] 5.13.1 Set debounce to 2000ms
  - [ ] 5.13.2 Trigger brief presence (< 2 seconds)
  - [ ] 5.13.3 Verify no notification sent (filtered)
  - [ ] 5.13.4 Trigger sustained presence (> 2 seconds)
  - [ ] 5.13.5 Verify notification sent
- [ ] 5.14 Test presence timeout:
  - [ ] 5.14.1 Set timeout to 5 seconds
  - [ ] 5.14.2 Trigger presence then move away
  - [ ] 5.14.3 Verify presence remains TRUE for 5 seconds
  - [ ] 5.14.4 Verify "no presence" notification after timeout
- [ ] 5.15 Test distance filtering (if available)
- [ ] 5.16 Test parameter validation (e.g., min > max distance)
- [ ] 5.17 Verify all parameters persist in NVS
- [ ] 5.18 Check free heap > 100KB

### Acceptance Criteria
- [ ] Sketch compiles with 0 errors, 0 warnings
- [ ] Upload successful to ESP32
- [ ] Debounce logic filters rapid state changes
- [ ] Presence timeout provides hysteresis for "no presence" detection
- [ ] Distance filtering works (if sensor provides distance data)
- [ ] Parameter validation prevents invalid configurations
- [ ] All parameters configurable via web UI
- [ ] Parameters persist across reboots
- [ ] Free heap memory > 100KB
- [ ] Sketch size < 80% program memory
- [ ] All features from Stage 4 still work

**Demo**: Adjust detection parameters via web, verify behavior changes (e.g., longer timeout delays "no presence" notification).

---

## Stage 6: Full Diagnostics and System Integration
**Goal**: Add comprehensive monitoring, event logging, watchdog timer, and final integration

### Software Implementation - Event Logging
- [ ] 6.1 Create `stage6_full_system.ino` (copy stage5 + add diagnostics)
- [ ] 6.2 Add include: esp_task_wdt.h
- [ ] 6.3 Implement event logging system:
  - [ ] 6.3.1 Create circular buffer array: String eventLog[50]
  - [ ] 6.3.2 Create index variable: int eventIndex = 0
  - [ ] 6.3.3 Implement addEvent(String message) function:
    - Add timestamp to message
    - Store in eventLog[eventIndex]
    - Increment eventIndex with wrap-around (% 50)
  - [ ] 6.3.4 Log presence state changes
  - [ ] 6.3.5 Log motion state changes
  - [ ] 6.3.6 Log notification sends (with type and target)
  - [ ] 6.3.7 Log I2C errors
  - [ ] 6.3.8 Log configuration changes
  - [ ] 6.3.9 Log WiFi connection/disconnection events
  - [ ] 6.3.10 Log factory reset events

### Software Implementation - Watchdog Timer
- [ ] 6.4 Enable ESP32 hardware watchdog timer:
  - [ ] 6.4.1 In setup(), call esp_task_wdt_init(8, true) for 8-second timeout
  - [ ] 6.4.2 Add current task to watchdog: esp_task_wdt_add(NULL)
  - [ ] 6.4.3 In loop(), feed watchdog: esp_task_wdt_reset()
  - [ ] 6.4.4 Add comment explaining watchdog timeout will reset ESP32 if hung
- [ ] 6.5 Add watchdog status to diagnostics display

### Software Implementation - Diagnostics Data
- [ ] 6.6 Implement system diagnostics data collection:
  - [ ] 6.6.1 Free heap memory (ESP.getFreeHeap())
  - [ ] 6.6.2 Total heap size (ESP.getHeapSize())
  - [ ] 6.6.3 Heap usage percentage calculation
  - [ ] 6.6.4 Uptime calculation from millis() (convert to days:hours:mins:secs)
  - [ ] 6.6.5 WiFi signal strength RSSI (WiFi.RSSI())
  - [ ] 6.6.6 WiFi SSID (WiFi.SSID())
  - [ ] 6.6.7 WiFi IP address (WiFi.localIP())
  - [ ] 6.6.8 I2C communication status (ok/error)
  - [ ] 6.6.9 I2C error counter
  - [ ] 6.6.10 Sensor firmware version (if available from C4001 library)
  - [ ] 6.6.11 ESP32 chip ID (ESP.getChipId())
  - [ ] 6.6.12 Flash size (ESP.getFlashChipSize())

### Software Implementation - Web UI Dashboard
- [ ] 6.7 Create comprehensive web UI dashboard (split into header file if > 10KB):
  - [ ] 6.7.1 Real-time sensor data panel:
    - Presence status with color indicator (green/red)
    - Motion status with color indicator
    - Last detection timestamp
    - Detection distance (if available)
    - Detection speed (if available)
  - [ ] 6.7.2 System status panel:
    - Uptime (formatted)
    - Free heap memory (KB and percentage)
    - WiFi status, SSID, RSSI, IP
    - I2C health indicator (green=ok, red=error)
    - Watchdog status
  - [ ] 6.7.3 Event log viewer:
    - Scrollable div with last 50 events
    - Reverse chronological order (newest first)
    - Auto-refresh every 5 seconds
    - Color-coded event types
  - [ ] 6.7.4 Notification history panel:
    - Last 10 notification attempts
    - Timestamp, type, target, status
    - Success/failure indicators
  - [ ] 6.7.5 Configuration panels (from previous stages)
  - [ ] 6.7.6 Navigation tabs/sections for better organization
- [ ] 6.8 Minimize HTML/CSS/JS size:
  - [ ] 6.8.1 Remove unnecessary whitespace
  - [ ] 6.8.2 Use inline CSS for simplicity
  - [ ] 6.8.3 Keep JavaScript minimal
  - [ ] 6.8.4 Consider splitting into web_interface.h header file if needed
  - [ ] 6.8.5 Verify total HTML size < 50KB

### Software Implementation - API Endpoints
- [ ] 6.9 Create `/diagnostics` JSON API endpoint:
  - [ ] 6.9.1 Return sensor status (presence, motion, timestamp)
  - [ ] 6.9.2 Return system info (uptime, heap, WiFi)
  - [ ] 6.9.3 Return I2C status
  - [ ] 6.9.4 Use ArduinoJson to build response
- [ ] 6.10 Create `/events` JSON API endpoint:
  - [ ] 6.10.1 Return event log as JSON array
  - [ ] 6.10.2 Newest events first
  - [ ] 6.10.3 Include timestamp and message for each event

### Software Implementation - Serial Debugging
- [ ] 6.11 Implement serial debugging levels:
  - [ ] 6.11.1 Add DEBUG_LEVEL constant (0=minimal, 1=normal, 2=verbose)
  - [ ] 6.11.2 Wrap debug Serial.print() statements with level checks
  - [ ] 6.11.3 Keep level 0 for production (errors only)
  - [ ] 6.11.4 Use level 2 for development (all debug info)

### Code Cleanup and Optimization
- [ ] 6.12 Code organization:
  - [ ] 6.12.1 Group related functions together
  - [ ] 6.12.2 Add function header comments
  - [ ] 6.12.3 Extract HTML to separate header file if > 10KB
  - [ ] 6.12.4 Use const char* PROGMEM for all string constants
- [ ] 6.13 Add comprehensive error handling:
  - [ ] 6.13.1 Check all WiFi operations
  - [ ] 6.13.2 Check all I2C operations
  - [ ] 6.13.3 Check all JSON parsing operations
  - [ ] 6.13.4 Handle null pointer dereferences
- [ ] 6.14 Add code comments:
  - [ ] 6.14.1 Explain complex algorithms (debounce, timeout)
  - [ ] 6.14.2 Document API endpoint behavior
  - [ ] 6.14.3 Note hardware pin assignments
  - [ ] 6.14.4 Explain watchdog timer operation
- [ ] 6.15 Memory leak verification:
  - [ ] 6.15.1 Monitor free heap over time
  - [ ] 6.15.2 Check for continuous heap decrease
  - [ ] 6.15.3 Fix any leaks found (HTTPClient.end(), string cleanup, etc.)

### Self-Test Implementation
- [ ] 6.16 Copy all test functions from Stage 5
- [ ] 6.17 Add `testEventLogging()`: Add test event, verify it appears in circular buffer
- [ ] 6.18 Add `testWatchdogFeeding()`: Verify esp_task_wdt_reset() calls succeed
- [ ] 6.19 Add `testDiagnosticsEndpoint()`: Call /diagnostics, verify JSON structure and content
- [ ] 6.20 Add `testEventsEndpoint()`: Call /events, verify event array returned
- [ ] 6.21 Add `testMemoryLeak()`: Run multiple iterations, check heap stays stable
- [ ] 6.22 Update `runSelfTest()` to include all Stage 6 tests (change header to "STAGE 6 FULL SYSTEM")
- [ ] 6.23 Update `/run-tests` endpoint to include all diagnostics test results
- [ ] 6.24 Add test result summary to web dashboard (all tests PASS/FAIL count)

### Final Integration Testing
- [ ] 6.16 Test all configuration options:
  - [ ] 6.16.1 WiFi credentials
  - [ ] 6.16.2 Notification settings (all three types)
  - [ ] 6.16.3 Detection parameters
  - [ ] 6.16.4 Device name
- [ ] 6.17 Test all notification types end-to-end
- [ ] 6.18 Test persistence across multiple reboots
- [ ] 6.19 Test long-running stability:
  - [ ] 6.19.1 Run for 24+ hours continuously
  - [ ] 6.19.2 Monitor heap memory every hour
  - [ ] 6.19.3 Verify no crashes or hangs
  - [ ] 6.19.4 Check watchdog hasn't triggered
- [ ] 6.20 Test edge cases:
  - [ ] 6.20.1 WiFi disconnect during operation (unplug router)
  - [ ] 6.20.2 WiFi reconnection (plug router back in)
  - [ ] 6.20.3 I2C sensor disconnection (unplug sensor)
  - [ ] 6.20.4 I2C sensor reconnection
  - [ ] 6.20.5 Invalid notification endpoint
  - [ ] 6.20.6 Rapid presence state changes
  - [ ] 6.20.7 Factory reset with active notifications
- [ ] 6.21 Verify sketch compiles with 0 errors and 0 warnings
- [ ] 6.22 Check final sketch size < 80% program memory
- [ ] 6.23 Verify free heap > 100KB during normal operation

### Acceptance Criteria
- [ ] Sketch compiles with 0 errors, 0 warnings
- [ ] Upload successful to ESP32
- [ ] Event log captures all significant events
- [ ] Watchdog timer configured and feeding correctly
- [ ] Full diagnostics display in web UI
- [ ] All system metrics displayed accurately
- [ ] I2C health monitoring works
- [ ] Memory usage stable over 24+ hours
- [ ] No memory leaks detected
- [ ] WiFi reconnection works automatically
- [ ] Sensor reconnection handled gracefully
- [ ] All edge cases handled without crashes
- [ ] Free heap memory > 100KB
- [ ] Sketch size < 80% program memory
- [ ] All features from Stages 1-5 still work

**Demo**: Full-featured presence detection system with web dashboard showing live data, event logs, diagnostics, and configurable notifications. System runs stably for 24+ hours.

---

## Documentation
- [ ] 7.1 Create README.md with:
  - [ ] 7.1.1 Hardware wiring diagram (text or ASCII art)
  - [ ] 7.1.2 Pull-up resistor installation instructions
  - [ ] 7.1.3 Library installation instructions (with version numbers)
  - [ ] 7.1.4 Arduino IDE configuration steps
  - [ ] 7.1.5 First-time setup guide:
    - Flash Stage 1, verify sensor works
    - Progress through stages
    - Initial WiFi configuration via AP mode
  - [ ] 7.1.6 Configuration examples:
    - Home Assistant HTTP POST notification
    - Node-RED webhook
    - Custom Python script
  - [ ] 7.1.7 Troubleshooting section:
    - "I2C device not found" → Check pull-up resistors
    - "Can't connect to WiFi" → Use AP fallback mode
    - "Out of memory" → Check heap in diagnostics
    - "Watchdog reset" → Look for blocking code
- [ ] 7.2 Add inline code comments for API endpoints
- [ ] 7.3 Document JSON API schema:
  - [ ] 7.3.1 /status response format
  - [ ] 7.3.2 /config request/response format
  - [ ] 7.3.3 /diagnostics response format
  - [ ] 7.3.4 /events response format
- [ ] 7.4 Create example notification payloads:
  - [ ] 7.4.1 HTTP GET example URL
  - [ ] 7.4.2 HTTP POST JSON example
  - [ ] 7.4.3 Socket message example
- [ ] 7.5 Document stage transition procedure
- [ ] 7.6 List known limitations (if any)

---

## Testing Checklist (All Stages)
- [ ] 8.1 Verify compilation with no errors at every stage
- [ ] 8.2 Verify upload to ESP32 successful at every stage
- [ ] 8.3 Test serial output at each stage
- [ ] 8.4 Test web interface in multiple browsers (Chrome, Firefox, Safari, mobile)
- [ ] 8.5 Test configuration persistence across reboots
- [ ] 8.6 Test all notification types (GET, POST, Socket)
- [ ] 8.7 Test WiFi reconnection after network outage
- [ ] 8.8 Test behavior with sensor disconnected
- [ ] 8.9 Test factory reset functionality
- [ ] 8.10 Verify memory usage stays within limits (heap > 100KB)
- [ ] 8.11 Verify sketch size < 80% program memory at all stages
- [ ] 8.12 Test AP fallback mode for initial setup
- [ ] 8.13 Test debounce logic
- [ ] 8.14 Test presence timeout
- [ ] 8.15 Verify watchdog timer operation
- [ ] 8.16 Test 24+ hour stability
- [ ] 8.17 Test all edge cases listed in Stage 6
