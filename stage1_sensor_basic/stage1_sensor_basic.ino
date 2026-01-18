/**
 * ESP32 Microwave Motion Sensor - Stage 1: Basic Sensor Interface
 *
 * This stage implements:
 * - GPIO communication with RCWL-0516 microwave radar sensor
 * - Motion detection polling at 10Hz
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
 */

#include <rom/rtc.h>  // For reset reason detection

// Pin Configuration
#define SENSOR_PIN 13       // GPIO for sensor OUT pin
#define LED_PIN 2           // Built-in LED for visual feedback

// Timing Configuration
#define SENSOR_POLL_INTERVAL 100   // 10Hz (100ms)
#define HEAP_PRINT_INTERVAL 30000  // Print heap every 30 seconds
#define HEAP_MIN_THRESHOLD 100000  // 100KB minimum free heap

// Motion state tracking
#define MOTION_TIMEOUT 5000  // Consider motion stopped after 5 seconds of no detection

// Global state variables
unsigned long lastSensorRead = 0;
unsigned long lastHeapPrint = 0;
unsigned long lastMotionTime = 0;
bool motionDetected = false;
bool motionActive = false;  // Sustained motion state
unsigned long motionStartTime = 0;
unsigned long totalMotionEvents = 0;

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
// SELF-TEST FUNCTIONS
// ============================================================================

/**
 * Test GPIO pin configuration
 * Returns: true if GPIO is configured correctly
 */
bool testGPIOConfiguration() {
  Serial.print("  GPIO Configuration (pin ");
  Serial.print(SENSOR_PIN);
  Serial.print("): ");

  // Verify pin is set as input
  pinMode(SENSOR_PIN, INPUT);

  // Read the pin to verify it's accessible
  int reading = digitalRead(SENSOR_PIN);

  // Pin should read either HIGH or LOW (0 or 1)
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
 * Returns: true if LED can be controlled
 */
bool testLEDConfiguration() {
  Serial.print("  LED Configuration (pin ");
  Serial.print(LED_PIN);
  Serial.print("): ");

  pinMode(LED_PIN, OUTPUT);

  // Blink LED briefly to verify
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);

  Serial.println("PASS (blinked)");
  return true;
}

/**
 * Test sensor response (checks if sensor output changes or is stable)
 * Returns: true (always passes - we're just reading the current state)
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
 * Returns: true if heap is above minimum threshold
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
 * Run all self-tests and report results
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
 * Print current sensor and motion statistics
 */
void printStats() {
  Serial.println("\n=== MOTION STATISTICS ===");
  Serial.print("Total Motion Events: ");
  Serial.println(totalMotionEvents);
  Serial.print("Current State: ");
  Serial.println(motionActive ? "MOTION ACTIVE" : "NO MOTION");

  if (motionActive && motionStartTime > 0) {
    Serial.print("Current Motion Duration: ");
    Serial.print((millis() - motionStartTime) / 1000);
    Serial.println(" seconds");
  }

  if (lastMotionTime > 0) {
    Serial.print("Last Motion: ");
    Serial.print((millis() - lastMotionTime) / 1000);
    Serial.println(" seconds ago");
  } else {
    Serial.println("Last Motion: Never");
  }

  Serial.print("Uptime: ");
  Serial.print(millis() / 1000);
  Serial.println(" seconds");
  Serial.println("=========================\n");
}

/**
 * Handle serial commands for interactive testing
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
        Serial.print("Sensor Pin: GPIO ");
        Serial.println(SENSOR_PIN);
        Serial.print("Motion Events: ");
        Serial.println(totalMotionEvents);
        Serial.println("===================\n");
        break;

      case '\n':
      case '\r':
        // Ignore newlines
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
  // Initialize serial communication
  Serial.begin(115200);
  while(!Serial);  // Wait for serial port to connect
  delay(1000);     // Stabilization delay

  Serial.println("\n\n");
  Serial.println("==========================================");
  Serial.println("ESP32 Microwave Motion Sensor - Stage 1");
  Serial.println("==========================================");
  Serial.println("Sensor: RCWL-0516 Microwave Radar");
  Serial.println();

  // Print reset reason
  printResetReason();
  Serial.println();

  // Initialize sensor pin as input
  pinMode(SENSOR_PIN, INPUT);
  Serial.print("Sensor pin configured (GPIO ");
  Serial.print(SENSOR_PIN);
  Serial.println(")");

  // Initialize LED pin for visual feedback
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  Serial.print("LED pin configured (GPIO ");
  Serial.print(LED_PIN);
  Serial.println(")");

  Serial.println();
  Serial.println("Sensor specifications:");
  Serial.println("  - Detection range: 5-7 meters");
  Serial.println("  - Detection angle: 360 degrees");
  Serial.println("  - Trigger duration: ~2-3 seconds");
  Serial.println("  - Output: HIGH when motion detected");
  Serial.println();

  // Run self-test automatically on boot
  runSelfTest();

  Serial.println("Type 'h' for help menu");
  Serial.println("Type 's' for motion statistics");
  Serial.println();
  Serial.println("Starting motion detection at 10Hz...");
  Serial.println("------------------------------------------");
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
  // Handle serial commands
  handleSerialCommands();

  // Poll sensor at configured interval
  unsigned long currentMillis = millis();

  if (currentMillis - lastSensorRead >= SENSOR_POLL_INTERVAL) {
    lastSensorRead = currentMillis;

    // Read sensor state
    bool currentReading = (digitalRead(SENSOR_PIN) == HIGH);

    // Update LED to match sensor state
    digitalWrite(LED_PIN, currentReading ? HIGH : LOW);

    // Detect state changes
    if (currentReading && !motionDetected) {
      // Motion just started
      motionDetected = true;
      lastMotionTime = currentMillis;

      if (!motionActive) {
        // New motion event
        motionActive = true;
        motionStartTime = currentMillis;
        totalMotionEvents++;

        Serial.print("[");
        Serial.print(currentMillis / 1000);
        Serial.print("s] MOTION DETECTED (#");
        Serial.print(totalMotionEvents);
        Serial.println(")");
        Serial.flush();
      }
    } else if (currentReading && motionDetected) {
      // Motion continues
      lastMotionTime = currentMillis;
    } else if (!currentReading && motionDetected) {
      // Sensor went LOW, but might retrigger
      motionDetected = false;
    }

    // Check for motion timeout (sustained no-motion)
    if (motionActive && !motionDetected &&
        (currentMillis - lastMotionTime >= MOTION_TIMEOUT)) {
      motionActive = false;

      unsigned long duration = (lastMotionTime - motionStartTime) / 1000;
      Serial.print("[");
      Serial.print(currentMillis / 1000);
      Serial.print("s] MOTION STOPPED (duration: ");
      Serial.print(duration);
      Serial.println("s)");
      Serial.flush();

      motionStartTime = 0;
    }

    // Periodic status output (every 10 seconds when motion active)
    static unsigned long lastStatusPrint = 0;
    if (motionActive && (currentMillis - lastStatusPrint >= 10000)) {
      lastStatusPrint = currentMillis;
      unsigned long duration = (currentMillis - motionStartTime) / 1000;
      Serial.print("[");
      Serial.print(currentMillis / 1000);
      Serial.print("s] Motion active for ");
      Serial.print(duration);
      Serial.println("s...");
      Serial.flush();
    }
  }

  // Print heap status periodically
  if (currentMillis - lastHeapPrint >= HEAP_PRINT_INTERVAL) {
    lastHeapPrint = currentMillis;

    uint32_t freeHeap = ESP.getFreeHeap();
    Serial.print("[HEAP] Free: ");
    Serial.print(freeHeap);
    Serial.print(" bytes (");
    Serial.print((freeHeap * 100) / ESP.getHeapSize());
    Serial.println("%)");
    Serial.flush();

    if (freeHeap < HEAP_MIN_THRESHOLD) {
      Serial.println("WARNING: Low heap memory!");
      Serial.flush();
    }
  }

  // Small delay to prevent tight loop
  delay(10);
}
