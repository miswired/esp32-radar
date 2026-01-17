# Stage 1: Basic Sensor Interface

This stage implements basic I2C communication with the DFRobot C4001 mmWave sensor, sensor polling at 1Hz, serial output for debugging, and built-in self-tests.

## ⚠️ CRITICAL: DIP SWITCH CONFIGURATION REQUIRED!

**Before connecting anything, configure the DIP switches on the sensor!**

The C4001 sensor has **2 DIP switches on the BACK** of the board that MUST be configured correctly:

### Switch 1: Communication Mode (I2C/UART)
- **MUST be set to "I2C" position**
- This is the **#1 cause of "NO Devices!" errors**
- If in UART position, the sensor will NOT respond on I2C bus

### Switch 2: Address Select (0x2A / 0x2B)
- **Set to 0x2A position** (default)
- Our code uses `SENSOR_I2C_ADDR = 0x2A`
- Only change if using multiple sensors on same bus

**❌ Common mistake**: Leaving switch in UART mode → Sensor won't be detected
**✅ Correct setup**: Both switches set to I2C mode at address 0x2A

## Hardware Requirements

- ESP32-WROOM-32 development board
- DFRobot SEN0610 C4001 mmWave sensor
- 2x 4.7kΩ resistors (for I2C pull-ups)
- Breadboard and jumper wires
- USB cable for programming

## Wiring Connections

**⚠️ STEP 0: Configure DIP switches on sensor BEFORE wiring!**
- Communication Mode switch → I2C position
- Address Select switch → 0x2A position

**CRITICAL**: The I2C pull-up resistors are REQUIRED for reliable communication.

```
DFRobot C4001 Sensor → ESP32-WROOM-32
────────────────────────────────────────
VCC                  → 3.3V (or 5V)
GND                  → GND
SDA                  → GPIO 21 (with 4.7kΩ pull-up to 3.3V)
SCL                  → GPIO 22 (with 4.7kΩ pull-up to 3.3V)
```

**DIP Switches (on back of sensor):**
- Switch 1: I2C (NOT UART)
- Switch 2: 0x2A (default address)

### Pull-up Resistor Installation

Each I2C line (SDA and SCL) needs a 4.7kΩ resistor between the signal line and 3.3V:

```
3.3V ─┬─── 4.7kΩ ───┬─── SDA (GPIO 21) ─── Sensor SDA
      │             │
      └─── 4.7kΩ ───┴─── SCL (GPIO 22) ─── Sensor SCL
```

## Software Setup

### 1. Install Arduino IDE or CLI

If using Arduino CLI (recommended for command-line builds):
```bash
# Verify installation
arduino-cli version

# Update core index
arduino-cli core update-index

# Install ESP32 board support (if not already installed)
arduino-cli core install esp32:esp32
```

### 2. Install Required Libraries

```bash
# Install DFRobot C4001 library
arduino-cli lib install DFRobot_C4001
```

### 3. Verify Board Configuration

In Arduino IDE:
- **Board**: "ESP32 Dev Module" (or your specific ESP32 board)
- **Upload Speed**: 115200
- **CPU Frequency**: 240MHz
- **Flash Frequency**: 80MHz
- **Flash Mode**: QIO
- **Flash Size**: 4MB
- **Partition Scheme**: Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)
- **Port**: Select your ESP32's serial port

## Compilation

### Using Arduino CLI

```bash
# Navigate to the stage1_sensor_basic directory
cd stage1_sensor_basic

# Compile the sketch
arduino-cli compile --fqbn esp32:esp32:esp32 stage1_sensor_basic.ino

# Expected output:
# Sketch uses 316471 bytes (24%) of program storage space.
# Global variables use 22224 bytes (6%) of dynamic memory.
```

### Using Arduino IDE

1. Open `stage1_sensor_basic.ino` in Arduino IDE
2. Select the correct board and port (Tools menu)
3. Click "Verify" button (checkmark icon)
4. Ensure compilation completes with 0 errors and 0 warnings

## Flashing to ESP32

### Using Arduino CLI

```bash
# Flash the compiled sketch (replace PORT with your ESP32's port)
arduino-cli upload --fqbn esp32:esp32:esp32 -p /dev/ttyUSB0 stage1_sensor_basic.ino

# On Linux, common ports are: /dev/ttyUSB0, /dev/ttyUSB1, /dev/ttyACM0
# On macOS: /dev/cu.usbserial-*
# On Windows: COM3, COM4, etc.
```

### Using Arduino IDE

1. Select the correct port (Tools → Port)
2. Click "Upload" button (right arrow icon)
3. Wait for "Done uploading" message

## Testing and Verification

### 1. Open Serial Monitor

```bash
# Using Arduino CLI
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200

# Or use screen
screen /dev/ttyUSB0 115200

# Or using Arduino IDE: Tools → Serial Monitor (set to 115200 baud)
```

### 2. Expected Boot Output

After flashing, you should see:

```
========================================
ESP32 mmWave Presence Sensor - Stage 1
========================================

I2C initialized (GPIO 21=SDA, GPIO 22=SCL)
Initializing C4001 mmWave sensor...
Sensor configured (Presence Detection Mode)

=== STAGE 1 SELF TEST ===
  I2C Communication (addr 0x2A): PASS
  Sensor Initialization: PASS
  Free Heap (305456 bytes): PASS
  Sensor Reading: PASS

Overall: ALL TESTS PASSED ✓
=========================

Type 'h' for help menu
Type 't' to re-run self-test

Starting sensor polling at 1Hz...
----------------------------------------
[1s] Presence: NO  | Motion: NONE
[2s] Presence: NO  | Motion: NONE
[3s] Presence: YES | Motion: DETECTED
[4s] Presence: YES | Motion: DETECTED
```

### 3. Serial Commands

While the sensor is running, you can use these interactive commands:

- **t** - Run self-test again
- **h** - Display help menu
- **r** - Restart ESP32
- **i** - Display system information

Example of 'i' command output:
```
=== SYSTEM INFO ===
Free Heap: 305456 bytes
Heap Size: 327680 bytes
Chip Model: ESP32-D0WDQ6
CPU Freq: 240 MHz
Flash Size: 4194304 bytes
Uptime: 127 seconds
I2C Errors: 0
===================
```

## Acceptance Criteria

Stage 1 is complete when:

- [x] Sketch compiles with 0 errors and 0 warnings
- [x] Sketch size < 80% of program memory (currently 24%)
- [x] Free heap > 100KB (currently ~305KB)
- [ ] **DIP switches configured correctly (I2C mode, 0x2A address)**
- [x] ESP32 boots and runs self-test automatically
- [ ] All self-tests report PASS
- [ ] Sensor polling displays presence detection at 1Hz
- [ ] Serial commands (t/h/r/i) work correctly
- [ ] I2C error count remains at 0 during normal operation

## Troubleshooting

### Issue: All self-tests fail "I2C Communication: FAIL"

**Cause**: Sensor not detected on I2C bus

**Solutions (check in this order)**:
1. **⚠️ CHECK DIP SWITCHES FIRST!** - Communication mode MUST be set to I2C (not UART)
2. Verify address select switch is set to 0x2A position
3. Verify sensor is powered (VCC connected to 3.3V or 5V)
4. Check GND connection
5. Install 4.7kΩ pull-up resistors on SDA and SCL lines
6. Verify wiring: GPIO 21 → SDA, GPIO 22 → SCL
7. Test I2C bus with i2c_scanner sketch (in parent directory)
8. Ensure sensor I2C address matches code (default 0x2A)

### Issue: Self-test passes but sensor shows "I2C error count: X"

**Cause**: Intermittent I2C communication failures

**Solutions**:
1. Add or replace pull-up resistors (must be 4.7kΩ)
2. Shorten I2C wire length (keep under 20cm if possible)
3. Check for loose connections
4. Ensure 3.3V power supply is stable

### Issue: "Free Heap: FAIL" during self-test

**Cause**: Insufficient memory available

**Solutions**:
1. Restart ESP32 (type 'r' in serial monitor)
2. Check for memory leaks in other code
3. Reduce sketch complexity if modified

### Issue: Upload fails with "No serial data received"

**Cause**: ESP32 not entering bootloader mode

**Solutions**:
1. Hold BOOT button on ESP32 while clicking Upload
2. Verify correct port selected
3. Check USB cable (data + power, not just power)
4. Try different USB port
5. Install CH340/CP2102 drivers if needed

### Issue: Sensor always shows "NO" for presence

**Cause**: Sensor may need configuration or environment factors

**Solutions**:
1. Verify sensor is started (self-test passed)
2. Move within sensor range (1.2m - 8m)
3. Ensure clear line of sight to sensor
4. Wait 5-10 seconds after boot for sensor to stabilize
5. Check sensor is in presence detection mode (printed at boot)

### Issue: Self-test shows "Config FAIL" or sensor in bad state

**Symptoms**:
- White LED doesn't light up (normally indicates presence)
- Green LED blinks with I2C commands but sensor doesn't work
- Configuration values read back as 0 or unexpected values
- Sensor worked before but stopped after changing settings

**Cause**: Sensor flash memory may have invalid configuration

**Solution**: Run the **sensor recovery tool** to factory reset:

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 ../sensor_recovery && \
arduino-cli upload --fqbn esp32:esp32:esp32 -p /dev/ttyUSB0 ../sensor_recovery && \
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

The recovery tool sends `eRecoverSen` (0xAA) which performs a factory data reset. After recovery:
1. Power cycle the sensor (unplug VCC, wait 5 seconds, reconnect)
2. Run Stage 1 sketch again
3. Verify all self-tests pass

See `TROUBLESHOOTING.md` in the project root for detailed recovery instructions.

## Next Steps

Once Stage 1 is working correctly:

1. Verify all acceptance criteria are met
2. Test sensor detection by moving in front of sensor
3. Confirm serial commands work (especially 't' and 'i')
4. Take note of free heap and I2C error count
5. Proceed to Stage 2: WiFi and Web Interface

## Technical Details

### Memory Usage
- Program storage: 316,471 bytes (24% of 1.3MB)
- Global variables: 22,224 bytes (6% of 327KB)
- Free heap: ~305KB (well above 100KB minimum)

### Sensor Configuration
- I2C Address: 0x2A
- I2C Clock: 100kHz
- Mode: Presence Detection (eExitMode)
- Poll Rate: 1Hz (1000ms interval)

### Timing Parameters
- Sensor poll interval: 1000ms (1Hz)
- Heap monitor interval: 10000ms (10 seconds)
- Heap minimum threshold: 100KB
