# I2C Communication Troubleshooting Guide for C4001 Sensor

## CRITICAL: Check DIP Switches First!

The **#1 cause of I2C failures** with the C4001 sensor is incorrect DIP switch configuration.

### Location
The sensor has **2 DIP switches on the back** of the board.

### Switch Configuration

1. **Communication Mode Switch (I2C/UART)**
   - **MUST be set to I2C position**
   - If in UART position, the sensor will NOT respond on I2C bus
   - Look for a switch labeled "I2C/UART" or similar

2. **Address Select Switch**
   - Selects between address 0x2A or 0x2B
   - Default is usually 0x2A
   - Our code uses 0x2A by default (`SENSOR_I2C_ADDR 0x2A`)

**ACTION**: Physically inspect the sensor and verify both switches are correctly set!

## Step-by-Step Diagnostic Process

### Step 1: Run I2C Scanner

Before debugging the main code, verify the sensor appears on the I2C bus.

```bash
cd /home/miswired/Multiverse/esp32-radar/i2c_scanner
arduino-cli upload --fqbn esp32:esp32:esp32 -p /dev/ttyUSB0 i2c_scanner.ino
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

**Expected output if sensor is working**:
```
I2C device found at address 0x2A  <- DFRobot C4001 Sensor!
Found 1 device(s)
```

**If no devices found**:
- ❌ DIP switch is in UART mode (most common!)
- ❌ Missing or incorrect pull-up resistors
- ❌ Wiring error
- ❌ Sensor not powered
- ❌ Faulty sensor

### Step 2: Verify Hardware Connections

#### Power Connections
```bash
# Measure voltage at sensor VCC pin
# Should read 3.3V or 5V (depending on your connection)
```

**Check**:
- [ ] VCC connected to ESP32 3.3V (or 5V)
- [ ] GND connected to ESP32 GND
- [ ] LED on sensor lights up (if present)

#### I2C Connections

**Wiring**:
```
ESP32 GPIO 21 (SDA) → Sensor SDA
ESP32 GPIO 22 (SCL) → Sensor SCL
```

**Measure voltage on I2C lines** (multimeter in DC voltage mode):
```bash
# With pull-up resistors installed:
# SDA line should read: ~3.3V (idle state)
# SCL line should read: ~3.3V (idle state)
```

If reading 0V on SDA/SCL:
- Missing pull-up resistors
- Short circuit to ground
- Faulty wiring

#### Pull-up Resistors

**CRITICAL**: 4.7kΩ resistors required on BOTH lines!

```
3.3V ─┬─── 4.7kΩ ───┬─── SDA (GPIO 21) ─── Sensor SDA
      │             │
      └─── 4.7kΩ ───┴─── SCL (GPIO 22) ─── Sensor SCL
```

**Check**:
- [ ] 4.7kΩ resistor from SDA to 3.3V
- [ ] 4.7kΩ resistor from SCL to 3.3V
- [ ] Resistors are actually 4.7kΩ (measure with multimeter if uncertain)
- [ ] Resistors connected to 3.3V (not 5V, not floating)

**Common mistakes**:
- Using wrong resistor value (4.7kΩ required, not 1kΩ or 10kΩ)
- Connecting pull-ups to wrong voltage rail
- Forgetting one of the two resistors

### Step 3: Check I2C Address

If scanner finds a device but at wrong address:

**Found at 0x2B instead of 0x2A**:
- Toggle the address select DIP switch
- OR change code to use `DEVICE_ADDR_1` (0x2B)

**To use 0x2B address in code**:
```cpp
// Change this line in stage1_sensor_basic.ino:
#define SENSOR_I2C_ADDR 0x2B  // Was 0x2A
```

### Step 4: Verify Code Configuration

If I2C scanner shows device at 0x2A but main code fails:

1. **Check I2C initialization**:
   ```cpp
   Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);  // GPIO 21, 22
   Wire.setClock(100000);  // 100kHz
   ```

2. **Verify sensor object creation**:
   ```cpp
   DFRobot_C4001_I2C sensor(&Wire, SENSOR_I2C_ADDR);
   ```

3. **Check begin() call pattern**:
   ```cpp
   while (!sensor.begin()) {
     Serial.println("NO Devices!");
     delay(2000);
   }
   ```

## Sensor Recovery (Factory Reset)

If your sensor gets into a bad state after configuration changes, you may need to run the **sensor recovery tool** to factory reset it.

### Symptoms That Indicate Recovery Is Needed

- **White LED doesn't light up** (normally indicates presence detection)
- **Green LED blinks** with I2C commands but sensor doesn't respond correctly
- **Configuration values read back as 0** or unexpected values
- **Sensor worked before** but stopped after changing sensitivity or other settings
- **Self-test shows "Config FAIL"** even though I2C communication passes
- **Sensor detected on I2C bus** but `begin()` fails or returns wrong status

### What Causes This

The C4001 sensor stores configuration in internal flash memory. Certain invalid configurations (especially extreme sensitivity values or timing parameters) can put the sensor into an unresponsive state. This is a known issue discussed on the [DFRobot forums](https://www.dfrobot.com/forum/topic/335591).

### Running the Recovery Tool

```bash
# Compile and upload the recovery tool
arduino-cli compile --fqbn esp32:esp32:esp32 /home/miswired/Multiverse/esp32-radar/sensor_recovery && \
arduino-cli upload --fqbn esp32:esp32:esp32 -p /dev/ttyUSB0 /home/miswired/Multiverse/esp32-radar/sensor_recovery && \
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

### What the Recovery Tool Does

1. Scans I2C bus to find the sensor
2. Sends `eStopSen` (0x33) - stops sensor collection
3. Sends `eRecoverSen` (0xAA) - **factory data reset**
4. Sends `eResetSen` (0xCC) - system reset
5. Reinitializes and verifies the sensor
6. Reports status before and after recovery

### Expected Recovery Output

```
==========================================
  C4001 SENSOR RECOVERY TOOL
==========================================

=== I2C Bus Scan ===
  Found device at 0x2A
  Total devices: 1
====================

--- Trying Primary Address (0x2A) ---

Attempting recovery on address 0x2A
  Device found, attempting begin()...
  begin() succeeded

  Reading status before recovery...
    Work Status: 1
    Work Mode: 0
    Init Status: 1

  Step 1: Stopping sensor...
  Step 2: Sending factory reset (eRecoverSen)...
  Step 3: Sending system reset (eResetSen)...
  Step 4: Reinitializing sensor...
  begin() succeeded after reset!

  Reading configuration after recovery...
    Trig Sensitivity: 3
    Keep Sensitivity: 3
    Min Range: 30
    Max Range: 800

  Step 5: Starting sensor...

  Final status check...
    Work Status: 1 (Running)
    Init Status: 1 (OK)

==========================================
  RECOVERY COMPLETE
==========================================

Recovery appears successful!
Please power cycle the sensor (unplug and replug)
then try the Stage 1 sketch again.
```

### After Recovery

1. **Power cycle the sensor** - unplug VCC, wait 5 seconds, reconnect
2. **Run Stage 1 sketch** - verify all self-tests pass
3. **Check configuration** - type 'c' to see current config values

### Recovery Commands

While the recovery tool is running:
- **s** - Scan I2C bus again
- **r** - Retry the recovery process
- **p** - Show power cycle instructions

### If Recovery Fails

If the sensor still doesn't work after recovery:
1. Try switching to **UART mode** (DIP switch) and use serial commands
2. Power cycle multiple times between recovery attempts
3. The sensor may be hardware damaged and need replacement

---

## Common Issues and Solutions

### Issue: "NO Devices!" message repeating

**Most likely cause**: DIP switch in UART mode

**Solutions** (in order of likelihood):
1. ✅ **Check DIP switch** - set to I2C mode
2. ✅ **Install pull-up resistors** - 4.7kΩ on SDA and SCL
3. ✅ **Verify wiring** - GPIO 21→SDA, GPIO 22→SCL
4. ✅ **Check power** - measure VCC at sensor (3.3V)
5. ✅ **Run I2C scanner** - verify device appears at 0x2A

### Issue: I2C scanner finds device but sensor.begin() fails

**Possible causes**:
- Wrong I2C address in code vs DIP switch setting
- Sensor initialization timing issue
- Library version incompatibility

**Solutions**:
1. Verify address matches: code=0x2A, DIP switch=0x2A position
2. Add delay before begin(): `delay(500);` after `Wire.begin()`
3. Check library version: `arduino-cli lib list | grep DFRobot`
4. Power cycle the sensor

### Issue: Intermittent I2C errors during operation

**Symptoms**: Tests pass initially, but "I2C error count" increases

**Solutions**:
1. Shorten wire length (keep under 20cm if possible)
2. Use higher quality jumper wires
3. Verify pull-up resistor connections are solid
4. Check for loose breadboard connections
5. Add 100nF capacitor between VCC and GND at sensor
6. Reduce I2C clock speed: `Wire.setClock(50000);` // 50kHz

### Issue: Tests pass but no presence detection

**This is different from I2C failure** - sensor communicates but doesn't detect:

**Solutions**:
1. Wait 5-10 seconds after boot for sensor calibration
2. Move within detection range (1.2m - 8m from sensor)
3. Ensure clear line of sight (no obstacles)
4. Check sensor mode is eExitMode (presence detection)
5. Adjust sensitivity settings if needed

## Code Updates Applied

Based on official DFRobot examples, the following fixes were applied:

### Fix 1: Removed incorrect setSensor() call
```cpp
// BEFORE (incorrect):
sensor.begin();
sensor.setSensor(eStartSen);  // This method doesn't work as expected

// AFTER (correct):
sensor.begin();
sensor.setSensorMode(eExitMode);  // Set presence detection mode
```

### Fix 2: Added retry loop in setup()
```cpp
// Match official example pattern:
while (!sensor.begin()) {
  Serial.println("NO Devices! Retrying...");
  delay(2000);
}
```

### Fix 3: Updated self-test to match official API
```cpp
bool testSensorInitialization() {
  bool success = sensor.begin();
  if (success) {
    sensor.setSensorMode(eExitMode);  // Correct API call
    return true;
  }
  return false;
}
```

## Hardware Checklist

Before proceeding, verify:

- [ ] Sensor has power (measure VCC = 3.3V or 5V)
- [ ] GND connected
- [ ] DIP switch #1 set to **I2C** (not UART)
- [ ] DIP switch #2 matches code address (default 0x2A)
- [ ] 4.7kΩ pull-up resistor on SDA line
- [ ] 4.7kΩ pull-up resistor on SCL line
- [ ] Pull-ups connected to 3.3V
- [ ] SDA connected to GPIO 21
- [ ] SCL connected to GPIO 22
- [ ] I2C scanner detects device at 0x2A

## Next Steps

1. **Run I2C scanner first** to confirm sensor is visible
2. **Check DIP switches** if scanner finds no devices
3. **Flash updated Stage 1 code** with corrected API calls
4. **Monitor serial output** for specific error messages
5. **Report findings** - which tests pass/fail for further diagnosis

## Reference Links

- [DFRobot C4001 Official Wiki](https://wiki.dfrobot.com/SKU_SEN0610_Gravity_C4001_mmWave_Presence_Sensor_12m_I2C_UART)
- [ESP32 I2C Guide](https://www.espboards.dev/sensors/c4001/)
- [DFRobot Forum - I2C Issues](https://www.dfrobot.com/forum/topic/399073)
- [Official GitHub Examples](https://github.com/dfrobot/DFRobot_C4001)
- [SmartHomeScene Tutorial](https://smarthomescene.com/diy/diy-presence-sensor-with-25m-detection-range-using-dfrobot-c4001-and-esp32/)

## Contact for Help

If still having issues after following this guide:

1. Run I2C scanner and capture output
2. Take photo of DIP switch positions
3. Capture serial monitor output from Stage 1
4. Measure and report:
   - Voltage at sensor VCC pin
   - Voltage at SDA line
   - Voltage at SCL line
5. Report which step in this guide you're stuck at

This will help diagnose the specific issue!
