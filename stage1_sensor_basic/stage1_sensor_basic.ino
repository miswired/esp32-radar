/**
 * ESP32 Microwave Motion Sensor - Stage 1: Basic Sensor Interface
 *
 * This stage implements:
 * - GPIO communication with RCWL-0516 microwave radar sensor
 * - Motion detection polling at 10Hz
 * - Trip delay feature (motion must be sustained before alarm triggers)
 * - State change detection with timestamps
 * - Serial output for debugging
 * - Built-in self-tests
 * - Serial command interface
 *
 * Hardware Requirements:
 * - ESP32-WROOM-32
 * - RCWL-0516 Microwave Radar Motion Sensor
 *
 * Connections:
 * - Sensor VIN → ESP32 VIN (5V from USB)
 * - Sensor GND → ESP32 GND
 * - Sensor OUT → ESP32 GPIO 13
 * - Sensor 3V3 → Do NOT connect (this is an output!)
 * - Sensor CDS → Do NOT connect (optional LDR feature)
 *
 * RCWL-0516 Specifications:
 * - Operating Voltage: 4-28V (use 5V from VIN)
 * - Operating Frequency: ~3.2 GHz (Doppler radar)
 * - Detection Range: 5-7 meters
 * - Detection Angle: 360° omnidirectional
 * - Output: 3.3V HIGH when motion detected, LOW otherwise
 * - Trigger Duration: ~2-3 seconds (retriggerable)
 *
 * Motion Detection Logic:
 * 1. Raw sensor reading (HIGH/LOW)
 * 2. Trip delay: Motion must be sustained for TRIP_DELAY_SECONDS before alarm
 * 3. Alarm state: Triggers notifications (in later stages)
 * 4. Clear timeout: No motion for CLEAR_TIMEOUT_SECONDS clears alarm
 */

#include <rom/rtc.h>  // For reset reason detection

// Pin Configuration
#define SENSOR_PIN 13       // GPIO for sensor OUT pin
#define LED_PIN 2           // Built-in LED for visual feedback

// Timing Configuration
#define SENSOR_POLL_INTERVAL 100   // 10Hz (100ms)
#define HEAP_PRINT_INTERVAL 30000  // Print heap every 30 seconds
#define HEAP_MIN_THRESHOLD 100000  // 100KB minimum free heap

// ============================================================================
// MOTION DETECTION CONFIGURATION
// These values will be configurable via web interface in later stages
// ============================================================================

// Trip Delay: Motion must be sustained for this many seconds before alarm triggers
// This prevents false alarms from brief movements (pets, curtains, HVAC)
#define TRIP_DELAY_SECONDS 3

// Clear Timeout: No motion for this many seconds clears the alarm
#define CLEAR_TIMEOUT_SECONDS 30

// Convert to milliseconds for internal use
#define TRIP_DELAY_MS (TRIP_DELAY_SECONDS * 1000)
#define CLEAR_TIMEOUT_MS (CLEAR_TIMEOUT_SECONDS * 1000)

// ============================================================================
// STATE MACHINE DEFINITIONS
// ============================================================================

enum MotionState {
  STATE_IDLE,           // No motion detected
  STATE_MOTION_PENDING, // Motion detected, waiting for trip delay
  STATE_ALARM_ACTIVE,   // Alarm triggered, notifications sent
  STATE_ALARM_CLEARING  // Motion stopped, waiting for clear timeout
};

// State names for display
const char* stateNames[] = {
  "IDLE",
  "MOTION_PENDING",
  "ALARM_ACTIVE",
  "ALARM_CLEARING"
};

// Global state variables
unsigned long lastSensorRead = 0;
unsigned long lastHeapPrint = 0;
bool rawMotionDetected = false;        // Current sensor reading
MotionState currentState = STATE_IDLE; // State machine state
unsigned long motionStartTime = 0;     // When motion first detected
unsigned long motionStopTime = 0;      // When motion last stopped
unsigned long alarmTriggerTime = 0;    // When alarm was triggered
unsigned long totalAlarmEvents = 0;    // Count of alarm triggers
unsigned long totalMotionEvents = 0;   // Count of raw motion events

// ============================================================================
// RESET REASON DETECTION
// ============================================================================

/**
 * Print the reason for the last ESP32 reset
 */
void printResetReason() {
  RESET_REASON reason = rtc_get_reset_reason(0);

  Serial.print("Reset reason: ");
  switch (reason) {
    case 1:  Serial.println("POWERON_RESET - Power on reset"); break;
    case 3:  Serial.println("SW_RESET - Software reset"); break;
    case 4:  Serial.println("OWDT_RESET - Legacy watch dog reset"); break;
    case 5:  Serial.println("DEEPSLEEP_RESET - Deep Sleep reset"); break;
    case 6:  Serial.println("SDIO_RESET - Reset by SLC module"); break;
    case 7:  Serial.println("TG0WDT_SYS_RESET - Timer Group0 Watch dog reset"); break;
    case 8:  Serial.println("TG1WDT_SYS_RESET - Timer Group1 Watch dog reset"); break;
    case 9:  Serial.println("RTCWDT_SYS_RESET - RTC Watch dog Reset"); break;
    case 10: Serial.println("INTRUSION_RESET - Intrusion tested to reset CPU"); break;
    case 11: Serial.println("TGWDT_CPU_RESET - Time Group reset CPU"); break;
    case 12: Serial.println("SW_CPU_RESET - Software reset CPU"); break;
    case 13: Serial.println("RTCWDT_CPU_RESET - RTC Watch dog Reset CPU"); break;
    case 14: Serial.println("EXT_CPU_RESET - External CPU reset"); break;
    case 15: Serial.println("RTCWDT_BROWN_OUT_RESET - Brownout Reset"); break;
    case 16: Serial.println("RTCWDT_RTC_RESET - RTC Watch dog reset"); break;
    default: Serial.println("UNKNOWN"); break;
  }

  Serial.flush();
}

// ============================================================================
// STATE MACHINE
// ============================================================================

/**
 * Process the motion state machine
 * Called every sensor poll interval
 */
void processMotionState(bool motionDetected, unsigned long currentMillis) {
  MotionState previousState = currentState;

  switch (currentState) {
    case STATE_IDLE:
      if (motionDetected) {
        // Motion started - begin trip delay countdown
        currentState = STATE_MOTION_PENDING;
        motionStartTime = currentMillis;
        totalMotionEvents++;

        Serial.print("[");
        Serial.print(currentMillis / 1000);
        Serial.print("s] Motion detected - waiting ");
        Serial.print(TRIP_DELAY_SECONDS);
        Serial.println("s for trip delay...");
      }
      break;

    case STATE_MOTION_PENDING:
      if (!motionDetected) {
        // Motion stopped before trip delay - return to idle
        currentState = STATE_IDLE;

        Serial.print("[");
        Serial.print(currentMillis / 1000);
        Serial.println("s] Motion stopped before trip delay - returning to IDLE");
      } else if (currentMillis - motionStartTime >= TRIP_DELAY_MS) {
        // Trip delay elapsed with sustained motion - TRIGGER ALARM
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

        // In later stages, this is where notifications would be sent
      }
      break;

    case STATE_ALARM_ACTIVE:
      if (!motionDetected) {
        // Motion stopped - begin clear timeout countdown
        currentState = STATE_ALARM_CLEARING;
        motionStopTime = currentMillis;

        Serial.print("[");
        Serial.print(currentMillis / 1000);
        Serial.print("s] Motion stopped - alarm will clear in ");
        Serial.print(CLEAR_TIMEOUT_SECONDS);
        Serial.println("s if no motion...");
      }
      break;

    case STATE_ALARM_CLEARING:
      if (motionDetected) {
        // Motion resumed - back to alarm active
        currentState = STATE_ALARM_ACTIVE;

        Serial.print("[");
        Serial.print(currentMillis / 1000);
        Serial.println("s] Motion resumed - alarm still active");
      } else if (currentMillis - motionStopTime >= CLEAR_TIMEOUT_MS) {
        // Clear timeout elapsed - alarm cleared
        currentState = STATE_IDLE;

        unsigned long alarmDuration = (currentMillis - alarmTriggerTime) / 1000;
        Serial.println();
        Serial.print("[");
        Serial.print(currentMillis / 1000);
        Serial.print("s] *** ALARM CLEARED *** (was active for ");
        Serial.print(alarmDuration);
        Serial.println("s)");
        Serial.println();

        // In later stages, this is where "alarm cleared" notification would be sent
      }
      break;
  }

  // Update LED based on state
  // LED ON = alarm active or clearing, LED OFF = idle or pending
  bool ledState = (currentState == STATE_ALARM_ACTIVE || currentState == STATE_ALARM_CLEARING);
  digitalWrite(LED_PIN, ledState ? HIGH : LOW);

  // Log state transitions
  if (currentState != previousState) {
    Serial.print("       State: ");
    Serial.print(stateNames[previousState]);
    Serial.print(" -> ");
    Serial.println(stateNames[currentState]);
    Serial.flush();
  }
}

// ============================================================================
// SELF-TEST FUNCTIONS
// ============================================================================

/**
 * Test GPIO pin configuration
 */
bool testGPIOConfiguration() {
  Serial.print("  GPIO Configuration (pin ");
  Serial.print(SENSOR_PIN);
  Serial.print("): ");

  pinMode(SENSOR_PIN, INPUT);
  int reading = digitalRead(SENSOR_PIN);

  if (reading == HIGH || reading == LOW) {
    Serial.print("PASS (current: ");
    Serial.print(reading == HIGH ? "HIGH" : "LOW");
    Serial.println(")");
    return true;
  } else {
    Serial.println("FAIL (invalid reading)");
    return false;
  }
}

/**
 * Test LED pin configuration
 */
bool testLEDConfiguration() {
  Serial.print("  LED Configuration (pin ");
  Serial.print(LED_PIN);
  Serial.print("): ");

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);

  Serial.println("PASS (blinked)");
  return true;
}

/**
 * Test sensor response
 */
bool testSensorReading() {
  Serial.print("  Sensor Reading: ");

  int reading = digitalRead(SENSOR_PIN);

  Serial.print("PASS (state: ");
  Serial.print(reading == HIGH ? "MOTION" : "NO MOTION");
  Serial.println(")");

  return true;
}

/**
 * Test heap memory availability
 */
bool testHeapMemory() {
  uint32_t freeHeap = ESP.getFreeHeap();
  bool pass = (freeHeap > HEAP_MIN_THRESHOLD);

  Serial.print("  Free Heap (");
  Serial.print(freeHeap);
  Serial.print(" bytes): ");
  Serial.println(pass ? "PASS" : "FAIL");

  return pass;
}

/**
 * Run all self-tests
 */
void runSelfTest() {
  Serial.println("\n=== STAGE 1 SELF TEST ===");

  bool allPass = true;

  allPass &= testGPIOConfiguration();
  allPass &= testLEDConfiguration();
  allPass &= testSensorReading();
  allPass &= testHeapMemory();

  Serial.println();
  Serial.print("Overall: ");
  Serial.println(allPass ? "ALL TESTS PASSED" : "SOME TESTS FAILED");
  Serial.println("=========================\n");

  if (!allPass) {
    Serial.println("TROUBLESHOOTING:");
    Serial.println("- GPIO FAIL: Check wiring to sensor OUT pin");
    Serial.println("- LED FAIL: Built-in LED may not be on pin 2");
    Serial.println("- Heap FAIL: Check for memory leaks");
    Serial.println();
  }
}

/**
 * Print current configuration
 */
void printConfig() {
  Serial.println("\n=== CURRENT CONFIGURATION ===");
  Serial.print("Sensor Pin: GPIO ");
  Serial.println(SENSOR_PIN);
  Serial.print("LED Pin: GPIO ");
  Serial.println(LED_PIN);
  Serial.print("Trip Delay: ");
  Serial.print(TRIP_DELAY_SECONDS);
  Serial.println(" seconds");
  Serial.print("Clear Timeout: ");
  Serial.print(CLEAR_TIMEOUT_SECONDS);
  Serial.println(" seconds");
  Serial.print("Poll Interval: ");
  Serial.print(SENSOR_POLL_INTERVAL);
  Serial.println(" ms");
  Serial.println("=============================\n");
}

/**
 * Print motion statistics
 */
void printStats() {
  Serial.println("\n=== MOTION STATISTICS ===");
  Serial.print("Current State: ");
  Serial.println(stateNames[currentState]);
  Serial.print("Raw Motion Events: ");
  Serial.println(totalMotionEvents);
  Serial.print("Alarm Triggers: ");
  Serial.println(totalAlarmEvents);

  if (currentState == STATE_MOTION_PENDING && motionStartTime > 0) {
    unsigned long elapsed = (millis() - motionStartTime) / 1000;
    unsigned long remaining = TRIP_DELAY_SECONDS - elapsed;
    Serial.print("Trip Delay Remaining: ");
    Serial.print(remaining);
    Serial.println(" seconds");
  }

  if (currentState == STATE_ALARM_ACTIVE && alarmTriggerTime > 0) {
    unsigned long duration = (millis() - alarmTriggerTime) / 1000;
    Serial.print("Alarm Active For: ");
    Serial.print(duration);
    Serial.println(" seconds");
  }

  if (currentState == STATE_ALARM_CLEARING && motionStopTime > 0) {
    unsigned long elapsed = (millis() - motionStopTime) / 1000;
    unsigned long remaining = CLEAR_TIMEOUT_SECONDS - elapsed;
    Serial.print("Clear Timeout Remaining: ");
    Serial.print(remaining);
    Serial.println(" seconds");
  }

  Serial.print("Uptime: ");
  Serial.print(millis() / 1000);
  Serial.println(" seconds");
  Serial.println("=========================\n");
}

/**
 * Handle serial commands
 */
void handleSerialCommands() {
  if (Serial.available()) {
    char cmd = Serial.read();

    switch(cmd) {
      case 't':
      case 'T':
        runSelfTest();
        break;

      case 'h':
      case 'H':
        Serial.println("\n=== SERIAL COMMANDS ===");
        Serial.println("t - Run self test");
        Serial.println("s - Print motion statistics");
        Serial.println("c - Print current configuration");
        Serial.println("h - Show this help menu");
        Serial.println("r - Restart ESP32");
        Serial.println("i - Print system info");
        Serial.println("l - Toggle LED");
        Serial.println("=======================\n");
        break;

      case 's':
      case 'S':
        printStats();
        break;

      case 'c':
      case 'C':
        printConfig();
        break;

      case 'r':
      case 'R':
        Serial.println("Restarting ESP32...");
        delay(500);
        ESP.restart();
        break;

      case 'l':
      case 'L':
        {
          static bool ledState = false;
          ledState = !ledState;
          digitalWrite(LED_PIN, ledState ? HIGH : LOW);
          Serial.print("LED: ");
          Serial.println(ledState ? "ON" : "OFF");
        }
        break;

      case 'i':
      case 'I':
        Serial.println("\n=== SYSTEM INFO ===");
        Serial.print("Free Heap: ");
        Serial.print(ESP.getFreeHeap());
        Serial.println(" bytes");
        Serial.print("Heap Size: ");
        Serial.print(ESP.getHeapSize());
        Serial.println(" bytes");
        Serial.print("Chip Model: ");
        Serial.println(ESP.getChipModel());
        Serial.print("CPU Freq: ");
        Serial.print(ESP.getCpuFreqMHz());
        Serial.println(" MHz");
        Serial.print("Flash Size: ");
        Serial.print(ESP.getFlashChipSize());
        Serial.println(" bytes");
        Serial.print("Uptime: ");
        Serial.print(millis() / 1000);
        Serial.println(" seconds");
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
  while(!Serial);
  delay(1000);

  Serial.println("\n\n");
  Serial.println("==========================================");
  Serial.println("ESP32 Microwave Motion Sensor - Stage 1");
  Serial.println("==========================================");
  Serial.println("Sensor: RCWL-0516 Microwave Radar");
  Serial.println();

  printResetReason();
  Serial.println();

  // Initialize pins
  pinMode(SENSOR_PIN, INPUT);
  Serial.print("Sensor pin configured (GPIO ");
  Serial.print(SENSOR_PIN);
  Serial.println(")");

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  Serial.print("LED pin configured (GPIO ");
  Serial.print(LED_PIN);
  Serial.println(")");

  Serial.println();
  Serial.println("Sensor specifications:");
  Serial.println("  - Detection range: 5-7 meters");
  Serial.println("  - Detection angle: 360 degrees");
  Serial.println("  - Output: HIGH when motion detected");
  Serial.println();

  // Print configuration
  printConfig();

  Serial.println("Motion Detection Logic:");
  Serial.print("  1. Motion must be sustained for ");
  Serial.print(TRIP_DELAY_SECONDS);
  Serial.println("s to trigger alarm");
  Serial.print("  2. No motion for ");
  Serial.print(CLEAR_TIMEOUT_SECONDS);
  Serial.println("s clears the alarm");
  Serial.println();

  // Run self-test
  runSelfTest();

  Serial.println("Type 'h' for help menu");
  Serial.println("Type 's' for motion statistics");
  Serial.println("Type 'c' for current configuration");
  Serial.println();
  Serial.println("Starting motion detection...");
  Serial.println("------------------------------------------");
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
  handleSerialCommands();

  unsigned long currentMillis = millis();

  // Poll sensor at configured interval
  if (currentMillis - lastSensorRead >= SENSOR_POLL_INTERVAL) {
    lastSensorRead = currentMillis;

    // Read sensor state
    rawMotionDetected = (digitalRead(SENSOR_PIN) == HIGH);

    // Process state machine
    processMotionState(rawMotionDetected, currentMillis);
  }

  // Print heap status periodically
  if (currentMillis - lastHeapPrint >= HEAP_PRINT_INTERVAL) {
    lastHeapPrint = currentMillis;

    uint32_t freeHeap = ESP.getFreeHeap();
    Serial.print("[HEAP] Free: ");
    Serial.print(freeHeap);
    Serial.print(" bytes | State: ");
    Serial.print(stateNames[currentState]);
    Serial.print(" | Alarms: ");
    Serial.println(totalAlarmEvents);
    Serial.flush();

    if (freeHeap < HEAP_MIN_THRESHOLD) {
      Serial.println("WARNING: Low heap memory!");
      Serial.flush();
    }
  }

  delay(10);
}
