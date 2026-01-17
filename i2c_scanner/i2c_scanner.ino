/**
 * I2C Scanner for ESP32
 *
 * This sketch scans the I2C bus to find connected devices.
 * Use this to verify the C4001 sensor appears at address 0x2A or 0x2B.
 *
 * Expected output if sensor is working:
 *   I2C device found at address 0x2A
 */

#include <Wire.h>

#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\n========================================");
  Serial.println("I2C Scanner for ESP32");
  Serial.println("========================================\n");

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(100000);  // 100kHz I2C clock

  Serial.println("I2C initialized (GPIO 21=SDA, GPIO 22=SCL)");
  Serial.println("Scanning I2C bus...\n");
}

void loop() {
  byte error, address;
  int nDevices = 0;

  Serial.println("Scanning...");

  for(address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.print(address, HEX);

      // Identify known devices
      if (address == 0x2A || address == 0x2B) {
        Serial.print("  <- DFRobot C4001 Sensor!");
      }

      Serial.println();
      nDevices++;
    }
    else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address, HEX);
    }
  }

  Serial.println("----------------------------------------");
  if (nDevices == 0) {
    Serial.println("No I2C devices found!");
    Serial.println("\nTROUBLESHOOTING:");
    Serial.println("1. Check DIP switch on sensor back is set to I2C mode");
    Serial.println("2. Verify 4.7kΩ pull-up resistors on SDA and SCL");
    Serial.println("3. Check wiring: SDA->GPIO21, SCL->GPIO22");
    Serial.println("4. Ensure sensor is powered (VCC->3.3V, GND->GND)");
    Serial.println("5. Measure voltage on SDA/SCL pins (should be ~3.3V)");
  }
  else {
    Serial.print("Found ");
    Serial.print(nDevices);
    Serial.println(" device(s)");

    if (nDevices > 0) {
      Serial.println("\nIf C4001 NOT found above:");
      Serial.println("- Check DIP switch is set to I2C (not UART)");
      Serial.println("- Try toggling the address select switch");
      Serial.println("- Power cycle the sensor");
    }
  }
  Serial.println("========================================\n");

  delay(5000);  // Wait 5 seconds before next scan
}
