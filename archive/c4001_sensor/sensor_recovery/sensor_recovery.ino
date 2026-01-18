/**
 * C4001 Sensor Recovery Tool
 *
 * Use this sketch to attempt to factory reset a C4001 sensor that
 * has gotten into a bad state (e.g., wrong sensitivity settings,
 * unresponsive, white LED not working).
 *
 * This sends the eRecoverSen (0xAA) command which performs a
 * "factory data reset" according to the DFRobot library.
 */

#include <Wire.h>
#include <DFRobot_C4001.h>

#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22

// Try both possible I2C addresses
#define SENSOR_ADDR_PRIMARY 0x2A
#define SENSOR_ADDR_SECONDARY 0x2B

DFRobot_C4001_I2C sensor(&Wire, SENSOR_ADDR_PRIMARY);

void scanI2C() {
  Serial.println("\n=== I2C Bus Scan ===");
  byte count = 0;

  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    byte error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("  Found device at 0x");
      if (addr < 16) Serial.print("0");
      Serial.println(addr, HEX);
      count++;
    }
  }

  if (count == 0) {
    Serial.println("  No I2C devices found!");
    Serial.println("  Check: DIP switch set to I2C, wiring, pull-ups");
  } else {
    Serial.print("  Total devices: ");
    Serial.println(count);
  }
  Serial.println("====================\n");
}

bool tryRecovery(uint8_t address) {
  Serial.print("\nAttempting recovery on address 0x");
  Serial.println(address, HEX);

  // Create sensor object with this address
  DFRobot_C4001_I2C testSensor(&Wire, address);

  // Check if device responds
  Wire.beginTransmission(address);
  if (Wire.endTransmission() != 0) {
    Serial.println("  No response at this address");
    return false;
  }

  Serial.println("  Device found, attempting begin()...");

  if (!testSensor.begin()) {
    Serial.println("  begin() failed, trying recovery anyway...");
  } else {
    Serial.println("  begin() succeeded");
  }

  // Try to get status first
  Serial.println("\n  Reading status before recovery...");
  sSensorStatus_t status = testSensor.getStatus();
  Serial.print("    Work Status: ");
  Serial.println(status.workStatus);
  Serial.print("    Work Mode: ");
  Serial.println(status.workMode);
  Serial.print("    Init Status: ");
  Serial.println(status.initStatus);

  // Stop the sensor first
  Serial.println("\n  Step 1: Stopping sensor...");
  testSensor.setSensor(eStopSen);
  delay(500);

  // Factory reset
  Serial.println("  Step 2: Sending factory reset (eRecoverSen)...");
  testSensor.setSensor(eRecoverSen);
  delay(1000);

  // System reset
  Serial.println("  Step 3: Sending system reset (eResetSen)...");
  testSensor.setSensor(eResetSen);
  delay(2000);

  // Try to reinitialize
  Serial.println("  Step 4: Reinitializing sensor...");
  if (!testSensor.begin()) {
    Serial.println("  WARNING: begin() failed after reset");
    Serial.println("  Sensor may need power cycle");
  } else {
    Serial.println("  begin() succeeded after reset!");
  }

  // Read status after recovery
  Serial.println("\n  Reading status after recovery...");
  status = testSensor.getStatus();
  Serial.print("    Work Status: ");
  Serial.println(status.workStatus);
  Serial.print("    Work Mode: ");
  Serial.println(status.workMode);
  Serial.print("    Init Status: ");
  Serial.println(status.initStatus);

  // Try to read current config
  Serial.println("\n  Reading configuration after recovery...");
  Serial.print("    Trig Sensitivity: ");
  Serial.println(testSensor.getTrigSensitivity());
  Serial.print("    Keep Sensitivity: ");
  Serial.println(testSensor.getKeepSensitivity());
  Serial.print("    Min Range: ");
  Serial.println(testSensor.getMinRange());
  Serial.print("    Max Range: ");
  Serial.println(testSensor.getMaxRange());

  // Start the sensor
  Serial.println("\n  Step 5: Starting sensor...");
  testSensor.setSensor(eStartSen);
  delay(500);

  // Final status
  Serial.println("\n  Final status check...");
  status = testSensor.getStatus();
  Serial.print("    Work Status: ");
  Serial.print(status.workStatus);
  Serial.println(status.workStatus == 1 ? " (Running)" : " (Stopped)");
  Serial.print("    Init Status: ");
  Serial.print(status.initStatus);
  Serial.println(status.initStatus == 1 ? " (OK)" : " (Failed)");

  return (status.initStatus == 1);
}

void setup() {
  Serial.begin(115200);
  while(!Serial);
  delay(1000);

  Serial.println("\n\n");
  Serial.println("==========================================");
  Serial.println("  C4001 SENSOR RECOVERY TOOL");
  Serial.println("==========================================");
  Serial.println();
  Serial.println("This tool attempts to factory reset the sensor");
  Serial.println("using the eRecoverSen (0xAA) command.");
  Serial.println();

  // Initialize I2C
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(100000);
  Serial.println("I2C initialized (GPIO 21=SDA, GPIO 22=SCL)");

  // Scan for devices
  scanI2C();

  // Try recovery on primary address
  Serial.println("\n--- Trying Primary Address (0x2A) ---");
  bool success1 = tryRecovery(SENSOR_ADDR_PRIMARY);

  // Try recovery on secondary address
  Serial.println("\n--- Trying Secondary Address (0x2B) ---");
  bool success2 = tryRecovery(SENSOR_ADDR_SECONDARY);

  Serial.println("\n==========================================");
  Serial.println("  RECOVERY COMPLETE");
  Serial.println("==========================================");

  if (success1 || success2) {
    Serial.println("\nRecovery appears successful!");
    Serial.println("Please power cycle the sensor (unplug and replug)");
    Serial.println("then try the Stage 1 sketch again.");
  } else {
    Serial.println("\nRecovery may have failed.");
    Serial.println("Try the following:");
    Serial.println("  1. Power cycle the sensor");
    Serial.println("  2. Check DIP switch is set to I2C mode");
    Serial.println("  3. Verify wiring and pull-up resistors");
    Serial.println("  4. Try running this recovery tool again");
    Serial.println("  5. If still failing, sensor may be damaged");
  }

  Serial.println("\n\nType 's' to scan I2C bus");
  Serial.println("Type 'r' to retry recovery");
  Serial.println("Type 'p' to power cycle reminder");
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();

    switch(cmd) {
      case 's':
      case 'S':
        scanI2C();
        break;

      case 'r':
      case 'R':
        Serial.println("\n--- Retrying Recovery ---");
        tryRecovery(SENSOR_ADDR_PRIMARY);
        tryRecovery(SENSOR_ADDR_SECONDARY);
        break;

      case 'p':
      case 'P':
        Serial.println("\n=== POWER CYCLE INSTRUCTIONS ===");
        Serial.println("1. Disconnect sensor VCC wire");
        Serial.println("2. Wait 5 seconds");
        Serial.println("3. Reconnect sensor VCC wire");
        Serial.println("4. Wait 2 seconds for sensor boot");
        Serial.println("5. Type 's' to scan I2C bus");
        Serial.println("================================\n");
        break;

      case '\n':
      case '\r':
        break;

      default:
        Serial.println("Commands: s=scan, r=retry, p=power cycle info");
        break;
    }
  }

  delay(10);
}
