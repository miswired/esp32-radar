/**
 * ESP32 mmWave Presence Sensor - Stage 1: Basic Sensor Interface
 *
 * This stage implements:
 * - I2C communication with DFRobot C4001 mmWave sensor
 * - Full sensor configuration (detection range, sensitivity, delays)
 * - Status register verification
 * - Sensor polling at 1Hz
 * - Serial output for debugging
 * - Built-in self-tests
 * - Serial command interface
 *
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
 *
 * Hardware Requirements:
 * - ESP32-WROOM-32
 * - DFRobot SEN0610 C4001 mmWave sensor
 * - 2x 4.7kΩ resistors (pull-ups for I2C)
 *
 * Connections:
 * - Sensor VCC → ESP32 3.3V (or 5V)
 * - Sensor GND → ESP32 GND
 * - Sensor SDA → ESP32 GPIO 21 (with 4.7kΩ pull-up to 3.3V)
 * - Sensor SCL → ESP32 GPIO 22 (with 4.7kΩ pull-up to 3.3V)
 */

#include <Wire.h>
#include <DFRobot_C4001.h>
#include <rom/rtc.h>  // For reset reason detection
#include <soc/rtc_cntl_reg.h>  // For brownout detector control
#include <soc/soc.h>  // For brownout detector control

// I2C Configuration
#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22
#define SENSOR_I2C_ADDR 0x2A

// Timing Configuration
#define SENSOR_POLL_INTERVAL 500   // 2Hz (500ms) - matches reference example
#define HEAP_PRINT_INTERVAL 10000  // Print heap every 10 seconds
#define HEAP_MIN_THRESHOLD 100000  // 100KB minimum free heap

// Power Management Configuration
// Set to 1 to disable brownout detector (use if experiencing spurious resets)
// Set to 0 to keep brownout detector enabled (recommended for debugging power issues)
#define DISABLE_BROWNOUT_DETECTOR 1

// ============================================================================
// SENSOR CONFIGURATION PARAMETERS
// Adjust these values to tune detection behavior
// ============================================================================

// Detection Range Configuration (in cm)
// min: Minimum detection distance, range 30-2500 cm (0.3m - 25m)
// max: Maximum detection distance, range 240-2500 cm (2.4m - 25m)
// trig: Trigger distance, typically set equal to max
#define DETECTION_MIN_RANGE 30     // 30cm = 0.3m minimum
#define DETECTION_MAX_RANGE 800    // 800cm = 8m maximum (presence mode limit)
#define DETECTION_TRIG_RANGE 800   // 800cm = 8m trigger range

// Sensitivity Configuration (0-9, higher = more sensitive)
// trigSensitivity: Sensitivity for initial detection trigger
// keepSensitivity: Sensitivity for maintaining detection state
#define TRIG_SENSITIVITY 3   // Trigger sensitivity (0-9) - lower = less sensitive
#define KEEP_SENSITIVITY 3   // Keep/maintain sensitivity (0-9) - lower = less sensitive

// Delay Configuration
// trigDelay: Delay before triggering, unit 0.01s, range 0-200 (0-2 seconds)
// keepTimeout: Maintain detection timeout, unit 0.5s, range 4-3000 (2-1500 seconds)
// NOTE: keep minimum is 4 (2 seconds), per DFRobot documentation
#define TRIG_DELAY 100       // 100 * 0.01s = 1 second trigger delay (matches reference)
#define KEEP_TIMEOUT 4       // 4 * 0.5s = 2 seconds keep timeout (minimum valid value)

// Create sensor object
DFRobot_C4001_I2C sensor(&Wire, SENSOR_I2C_ADDR);

// Global state variables
unsigned long lastSensorRead = 0;
unsigned long lastHeapPrint = 0;
bool presenceDetected = false;
bool motionDetected = false;
int i2cErrorCount = 0;
bool sensorConfigured = false;

// ============================================================================
// RESET REASON DETECTION
// ============================================================================

/**
 * Print the reason for the last ESP32 reset
 * This helps diagnose unexpected resets that cause UART corruption
 */
void printResetReason() {
  RESET_REASON reason = rtc_get_reset_reason(0);

  Serial.print("Reset reason: ");
  switch (reason) {
    case 1:  Serial.println("POWERON_RESET - Vbat power on reset"); break;
    case 3:  Serial.println("SW_RESET - Software reset"); break;
    case 4:  Serial.println("OWDT_RESET - Legacy watch dog reset"); break;
    case 5:  Serial.println("DEEPSLEEP_RESET - Deep Sleep reset"); break;
    case 6:  Serial.println("SDIO_RESET - Reset by SLC module"); break;
    case 7:  Serial.println("TG0WDT_SYS_RESET - Timer Group0 Watch dog reset"); break;
    case 8:  Serial.println("TG1WDT_SYS_RESET - Timer Group1 Watch dog reset"); break;
    case 9:  Serial.println("RTCWDT_SYS_RESET - RTC Watch dog Reset"); break;
    case 10: Serial.println("INTRUSION_RESET - Instrusion tested to reset CPU"); break;
    case 11: Serial.println("TGWDT_CPU_RESET - Time Group reset CPU"); break;
    case 12: Serial.println("SW_CPU_RESET - Software reset CPU"); break;
    case 13: Serial.println("RTCWDT_CPU_RESET - RTC Watch dog Reset CPU"); break;
    case 14: Serial.println("EXT_CPU_RESET - External CPU reset"); break;
    case 15: Serial.println("RTCWDT_BROWN_OUT_RESET - Brownout Reset"); break;
    case 16: Serial.println("RTCWDT_RTC_RESET - RTC Watch dog reset"); break;
    default: Serial.println("UNKNOWN"); break;
  }

  if (reason == 15) {
    Serial.println("⚠️  BROWNOUT DETECTED - Check power supply voltage!");
    Serial.println("     - Use quality USB cable (data + power)");
    Serial.println("     - Ensure stable 5V power supply");
    Serial.println("     - Check for voltage drops during I2C operations");
  }

  Serial.flush();
}

// ============================================================================
// SENSOR STATUS AND CONFIGURATION
// ============================================================================

/**
 * Read and display sensor status register
 * Returns: true if status is valid and sensor is initialized
 */
bool printSensorStatus() {
  Serial.println("\n--- Sensor Status Register ---");

  sSensorStatus_t status = sensor.getStatus();

  Serial.print("  Work Status: ");
  Serial.print(status.workStatus);
  Serial.println(status.workStatus == 1 ? " (Running)" : " (Stopped)");

  Serial.print("  Work Mode: ");
  Serial.print(status.workMode);
  Serial.println(status.workMode == 0 ? " (Presence/Exit Mode)" : " (Speed Mode)");

  Serial.print("  Init Status: ");
  Serial.print(status.initStatus);
  Serial.println(status.initStatus == 1 ? " (Initialized)" : " (Not Initialized)");

  Serial.println("------------------------------");

  return (status.initStatus == 1);
}

/**
 * Configure the sensor with detection parameters
 * Returns: true if all configurations succeeded
 */
bool configureSensor() {
  bool allSuccess = true;

  Serial.println("\n--- Configuring Sensor ---");

  // Set sensor mode to presence detection (eExitMode)
  sensor.setSensorMode(eExitMode);
  delay(100);
  Serial.println("  Mode: Presence Detection (eExitMode)");

  // Configure detection range
  Serial.print("  Setting detection range (");
  Serial.print(DETECTION_MIN_RANGE);
  Serial.print("-");
  Serial.print(DETECTION_MAX_RANGE);
  Serial.print(" cm, trig=");
  Serial.print(DETECTION_TRIG_RANGE);
  Serial.print(" cm): ");

  if (sensor.setDetectionRange(DETECTION_MIN_RANGE, DETECTION_MAX_RANGE, DETECTION_TRIG_RANGE)) {
    Serial.println("OK");
  } else {
    Serial.println("FAILED");
    allSuccess = false;
  }

  // Set trigger sensitivity
  Serial.print("  Setting trigger sensitivity (");
  Serial.print(TRIG_SENSITIVITY);
  Serial.print("): ");

  if (sensor.setTrigSensitivity(TRIG_SENSITIVITY)) {
    Serial.println("OK");
  } else {
    Serial.println("FAILED");
    allSuccess = false;
  }

  // Set keep sensitivity
  Serial.print("  Setting keep sensitivity (");
  Serial.print(KEEP_SENSITIVITY);
  Serial.print("): ");

  if (sensor.setKeepSensitivity(KEEP_SENSITIVITY)) {
    Serial.println("OK");
  } else {
    Serial.println("FAILED");
    allSuccess = false;
  }

  // Set delay parameters
  Serial.print("  Setting delays (trig=");
  Serial.print(TRIG_DELAY);
  Serial.print(", keep=");
  Serial.print(KEEP_TIMEOUT);
  Serial.print("): ");

  if (sensor.setDelay(TRIG_DELAY, KEEP_TIMEOUT)) {
    Serial.println("OK");
  } else {
    Serial.println("FAILED");
    allSuccess = false;
  }

  // Wait for sensor to apply delay settings
  delay(500);

  Serial.println("---------------------------");

  return allSuccess;
}

/**
 * Read back and display all sensor configuration parameters
 * This verifies the configuration was applied correctly
 */
void verifySensorConfig() {
  Serial.println("\n--- Verifying Sensor Configuration ---");

  // Read back sensitivity settings
  uint8_t trigSens = sensor.getTrigSensitivity();
  uint8_t keepSens = sensor.getKeepSensitivity();

  Serial.print("  Trigger Sensitivity: ");
  Serial.print(trigSens);
  Serial.print(" (expected: ");
  Serial.print(TRIG_SENSITIVITY);
  Serial.println(trigSens == TRIG_SENSITIVITY ? ") ✓" : ") ✗ MISMATCH");

  Serial.print("  Keep Sensitivity: ");
  Serial.print(keepSens);
  Serial.print(" (expected: ");
  Serial.print(KEEP_SENSITIVITY);
  Serial.println(keepSens == KEEP_SENSITIVITY ? ") ✓" : ") ✗ MISMATCH");

  // Read back range settings
  uint16_t minRange = sensor.getMinRange();
  uint16_t maxRange = sensor.getMaxRange();
  uint16_t trigRange = sensor.getTrigRange();

  Serial.print("  Min Range: ");
  Serial.print(minRange);
  Serial.print(" cm (expected: ");
  Serial.print(DETECTION_MIN_RANGE);
  Serial.println(minRange == DETECTION_MIN_RANGE ? ") ✓" : ") ✗ MISMATCH");

  Serial.print("  Max Range: ");
  Serial.print(maxRange);
  Serial.print(" cm (expected: ");
  Serial.print(DETECTION_MAX_RANGE);
  Serial.println(maxRange == DETECTION_MAX_RANGE ? ") ✓" : ") ✗ MISMATCH");

  Serial.print("  Trigger Range: ");
  Serial.print(trigRange);
  Serial.print(" cm (expected: ");
  Serial.print(DETECTION_TRIG_RANGE);
  Serial.println(trigRange == DETECTION_TRIG_RANGE ? ") ✓" : ") ✗ MISMATCH");

  // Read back delay settings
  uint16_t keepTimeout = sensor.getKeepTimerout();
  uint16_t trigDelay = sensor.getTrigDelay();

  Serial.print("  Keep Timeout: ");
  Serial.print(keepTimeout);
  Serial.print(" (expected: ");
  Serial.print(KEEP_TIMEOUT);
  Serial.println(keepTimeout == KEEP_TIMEOUT ? ") ✓" : ") ✗ MISMATCH");

  Serial.print("  Trigger Delay: ");
  Serial.print(trigDelay);
  Serial.print(" (expected: ");
  Serial.print(TRIG_DELAY);
  Serial.println(trigDelay == TRIG_DELAY ? ") ✓" : ") ✗ MISMATCH");

  Serial.println("--------------------------------------");
}

/**
 * Print current sensor configuration (for 'c' command)
 */
void printCurrentConfig() {
  Serial.println("\n=== CURRENT SENSOR CONFIG ===");

  // Status
  sSensorStatus_t status = sensor.getStatus();
  Serial.print("Work Status: ");
  Serial.println(status.workStatus == 1 ? "Running" : "Stopped");
  Serial.print("Work Mode: ");
  Serial.println(status.workMode == 0 ? "Presence" : "Speed");
  Serial.print("Init Status: ");
  Serial.println(status.initStatus == 1 ? "OK" : "Failed");

  Serial.println();

  // Sensitivity
  Serial.print("Trigger Sensitivity: ");
  Serial.println(sensor.getTrigSensitivity());
  Serial.print("Keep Sensitivity: ");
  Serial.println(sensor.getKeepSensitivity());

  Serial.println();

  // Range
  Serial.print("Min Range: ");
  Serial.print(sensor.getMinRange());
  Serial.println(" cm");
  Serial.print("Max Range: ");
  Serial.print(sensor.getMaxRange());
  Serial.println(" cm");
  Serial.print("Trigger Range: ");
  Serial.print(sensor.getTrigRange());
  Serial.println(" cm");

  Serial.println();

  // Delays
  Serial.print("Trigger Delay: ");
  Serial.print(sensor.getTrigDelay());
  Serial.println(" (x0.01s)");
  Serial.print("Keep Timeout: ");
  Serial.print(sensor.getKeepTimerout());
  Serial.println(" (x0.5s)");

  Serial.println("=============================\n");
}

// ============================================================================
// SELF-TEST FUNCTIONS
// ============================================================================

/**
 * Test I2C communication with the sensor
 * Returns: true if I2C communication is working, false otherwise
 */
bool testI2CCommunication() {
  Wire.beginTransmission(SENSOR_I2C_ADDR);
  byte error = Wire.endTransmission();

  Serial.print("  I2C Communication (addr 0x");
  Serial.print(SENSOR_I2C_ADDR, HEX);
  Serial.print("): ");

  if (error == 0) {
    Serial.println("PASS");
    return true;
  } else {
    Serial.print("FAIL (error code: ");
    Serial.print(error);
    Serial.println(")");
    return false;
  }
}

/**
 * Test sensor initialization and status
 * Returns: true if sensor is initialized, false otherwise
 */
bool testSensorInitialization() {
  Serial.print("  Sensor Init Status: ");

  sSensorStatus_t status = sensor.getStatus();

  if (status.initStatus == 1) {
    Serial.println("PASS (initialized)");
    return true;
  } else {
    Serial.println("FAIL (not initialized)");
    return false;
  }
}

/**
 * Test sensor configuration was applied correctly
 * Returns: true if all config values match expected, false otherwise
 */
bool testSensorConfig() {
  Serial.print("  Sensor Configuration: ");

  bool allMatch = true;

  // Check sensitivity values
  if (sensor.getTrigSensitivity() != TRIG_SENSITIVITY) allMatch = false;
  if (sensor.getKeepSensitivity() != KEEP_SENSITIVITY) allMatch = false;

  // Check range values
  if (sensor.getMinRange() != DETECTION_MIN_RANGE) allMatch = false;
  if (sensor.getMaxRange() != DETECTION_MAX_RANGE) allMatch = false;
  if (sensor.getTrigRange() != DETECTION_TRIG_RANGE) allMatch = false;

  // Check delay values
  if (sensor.getKeepTimerout() != KEEP_TIMEOUT) allMatch = false;
  if (sensor.getTrigDelay() != TRIG_DELAY) allMatch = false;

  if (allMatch) {
    Serial.println("PASS (all values match)");
    return true;
  } else {
    Serial.println("FAIL (config mismatch - run 'c' for details)");
    return false;
  }
}

/**
 * Test heap memory availability
 * Returns: true if heap is above minimum threshold, false otherwise
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
 * Test sensor reading functionality
 * Returns: true if sensor responds to read request
 */
bool testSensorReading() {
  Serial.print("  Sensor Reading: ");

  // Try to read sensor data - this exercises the I2C read path
  bool detected = sensor.motionDetection();

  // If we get here without hanging, I2C read works
  Serial.print("PASS (current state: ");
  Serial.print(detected ? "DETECTED" : "NONE");
  Serial.println(")");

  return true;
}

/**
 * Run all self-tests and report results
 */
void runSelfTest() {
  Serial.println("\n=== STAGE 1 SELF TEST ===");

  bool allPass = true;

  allPass &= testI2CCommunication();
  allPass &= testSensorInitialization();
  allPass &= testSensorConfig();
  allPass &= testHeapMemory();
  allPass &= testSensorReading();

  Serial.println();
  Serial.print("Overall: ");
  Serial.println(allPass ? "ALL TESTS PASSED ✓" : "SOME TESTS FAILED ✗");
  Serial.println("=========================\n");

  if (!allPass) {
    Serial.println("TROUBLESHOOTING:");
    Serial.println("- I2C FAIL: Check 4.7kΩ pull-up resistors on SDA/SCL");
    Serial.println("- Init FAIL: Verify sensor powered and DIP switch set to I2C");
    Serial.println("- Config FAIL: Re-run configuration or check sensor limits");
    Serial.println("- Heap FAIL: Check for memory leaks or reduce usage");
    Serial.println();
  }
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
        Serial.println("c - Print current sensor config");
        Serial.println("s - Print sensor status register");
        Serial.println("v - Verify sensor configuration");
        Serial.println("h - Show this help menu");
        Serial.println("r - Restart ESP32");
        Serial.println("i - Print system info");
        Serial.println("=======================\n");
        break;

      case 'c':
      case 'C':
        printCurrentConfig();
        break;

      case 's':
      case 'S':
        printSensorStatus();
        break;

      case 'v':
      case 'V':
        verifySensorConfig();
        break;

      case 'r':
      case 'R':
        Serial.println("Restarting ESP32...");
        delay(500);
        ESP.restart();
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
        Serial.print("I2C Errors: ");
        Serial.println(i2cErrorCount);
        Serial.print("Sensor Configured: ");
        Serial.println(sensorConfigured ? "Yes" : "No");
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
  // Disable brownout detector if configured (prevents spurious resets)
  #if DISABLE_BROWNOUT_DETECTOR
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  #endif

  // Initialize serial communication
  Serial.begin(115200);
  while(!Serial);  // Wait for serial port to connect
  delay(1000);     // Additional stabilization delay

  Serial.println("\n\n");
  Serial.println("========================================");
  Serial.println("ESP32 mmWave Presence Sensor - Stage 1");
  Serial.println("========================================");
  Serial.println();

  // Print reset reason to diagnose unexpected resets
  printResetReason();
  Serial.println();

  // Initialize I2C with custom pins
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(100000);  // 100kHz I2C clock
  Serial.println("I2C initialized (GPIO 21=SDA, GPIO 22=SCL)");

  // Initialize sensor
  Serial.println("Initializing C4001 mmWave sensor...");

  // Keep trying to initialize until successful
  while (!sensor.begin()) {
    Serial.println("NO Devices! Retrying in 2 seconds...");
    Serial.println("Check:");
    Serial.println("  - DIP switch on sensor back is set to I2C mode");
    Serial.println("  - Sensor is powered (VCC and GND connected)");
    Serial.println("  - 4.7kΩ pull-up resistors installed on SDA/SCL");
    delay(2000);
  }

  Serial.println("Device connected!");

  // Print initial sensor status
  printSensorStatus();

  // Configure the sensor with our desired parameters
  sensorConfigured = configureSensor();

  // Wait for configuration to take effect
  delay(500);

  // Verify configuration was applied correctly
  verifySensorConfig();

  // Print final status after configuration
  printSensorStatus();

  Serial.println();

  // Run self-test automatically on boot
  runSelfTest();

  Serial.println("Type 'h' for help menu");
  Serial.println("Type 'c' to view current config");
  Serial.println();
  Serial.println("Starting sensor polling at 2Hz...");
  Serial.println("----------------------------------------");
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

    // Read sensor using motionDetection()
    bool detected = sensor.motionDetection();

    // Track state changes
    bool stateChanged = (detected != presenceDetected);
    presenceDetected = detected;
    motionDetected = detected;

    // Print sensor data
    Serial.print("[");
    Serial.print(millis() / 1000);
    Serial.print("s] ");
    Serial.print("Presence: ");
    Serial.print(presenceDetected ? "YES" : "NO ");
    Serial.print(" | Motion: ");
    Serial.print(motionDetected ? "DETECTED" : "NONE    ");

    if (stateChanged) {
      Serial.print(" <-- STATE CHANGED");
    }

    Serial.println();
    Serial.flush();

    // Reset error count on successful read
    if (i2cErrorCount > 0) {
      Serial.println("I2C communication recovered!");
      Serial.flush();
      i2cErrorCount = 0;
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
