# Stage 1: Basic Sensor Interface

This stage implements basic GPIO communication with the RCWL-0516 microwave radar motion sensor, polling at 10Hz, state change detection, and serial output for debugging.

## Sensor: RCWL-0516 Microwave Radar

The RCWL-0516 is a Doppler radar motion sensor that operates at ~3.2 GHz. It provides a simple digital output:
- **HIGH (3.3V)**: Motion detected
- **LOW (0V)**: No motion

### Key Specifications

| Parameter | Value |
|-----------|-------|
| Operating Voltage | 4-28V (use 5V) |
| Operating Frequency | ~3.18 GHz |
| Detection Range | 5-7 meters |
| Detection Angle | 360° omnidirectional |
| Current Draw | ~2.7 mA |
| Output Voltage | 3.3V TTL |
| Trigger Duration | ~2-3 seconds |

### Advantages

- **Simple interface**: Just a digital GPIO read - no protocols, no libraries needed
- **Reliable**: No configuration to corrupt, no recovery tools needed
- **Versatile**: Detects through walls, plastic, wood (non-metallic materials)
- **Temperature independent**: Works in any environment (unlike PIR)
- **Low cost**: ~$1-2 per sensor

## Hardware Requirements

- ESP32-WROOM-32 development board
- RCWL-0516 Microwave Radar Motion Sensor
- USB cable for programming

**No pull-up resistors or external components needed!**

## Wiring Connections

```
RCWL-0516          ESP32
---------          -----
VIN            →   VIN (5V from USB)
GND            →   GND
OUT            →   GPIO 13
3V3            →   DO NOT CONNECT (this is an output!)
CDS            →   DO NOT CONNECT (optional LDR feature)
```

### Important Notes

1. **Power**: Connect VIN to ESP32's VIN pin (5V), NOT to 3.3V. The sensor needs 4-28V.
2. **3V3 Pin**: This is a regulated 3.3V OUTPUT from the sensor, not an input. Do not connect it.
3. **Orientation**: The component side of the sensor should face the detection area. Keep the back clear of metal objects.

## Software Setup

### 1. Install Arduino IDE or CLI

```bash
# Verify installation
arduino-cli version

# Update core index
arduino-cli core update-index

# Install ESP32 board support (if not already installed)
arduino-cli core install esp32:esp32
```

### 2. No Additional Libraries Required

The RCWL-0516 uses standard Arduino GPIO functions - no external libraries needed!

## Compilation and Upload

### Using Arduino CLI

```bash
# Navigate to project directory
cd /home/miswired/Multiverse/esp32-radar

# Compile the sketch
arduino-cli compile --fqbn esp32:esp32:esp32 stage1_sensor_basic

# Upload to ESP32 (adjust port if needed)
arduino-cli upload --fqbn esp32:esp32:esp32 -p /dev/ttyUSB0 stage1_sensor_basic

# Monitor serial output
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

### All-in-One Command

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 stage1_sensor_basic && \
arduino-cli upload --fqbn esp32:esp32:esp32 -p /dev/ttyUSB0 stage1_sensor_basic && \
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

## Expected Output

### Boot Sequence

```
==========================================
ESP32 Microwave Motion Sensor - Stage 1
==========================================
Sensor: RCWL-0516 Microwave Radar

Reset reason: POWERON_RESET - Power on reset

Sensor pin configured (GPIO 13)
LED pin configured (GPIO 2)

Sensor specifications:
  - Detection range: 5-7 meters
  - Detection angle: 360 degrees
  - Trigger duration: ~2-3 seconds
  - Output: HIGH when motion detected

=== STAGE 1 SELF TEST ===
  GPIO Configuration (pin 13): PASS (current: LOW)
  LED Configuration (pin 2): PASS (blinked)
  Sensor Reading: PASS (state: NO MOTION)
  Free Heap (305456 bytes): PASS

Overall: ALL TESTS PASSED
=========================

Type 'h' for help menu
Type 's' for motion statistics

Starting motion detection at 10Hz...
------------------------------------------
```

### Motion Detection Output

```
[5s] MOTION DETECTED (#1)
[15s] Motion active for 10s...
[23s] MOTION STOPPED (duration: 18s)
[45s] MOTION DETECTED (#2)
[52s] MOTION STOPPED (duration: 7s)
```

## Serial Commands

| Command | Description |
|---------|-------------|
| `t` | Run self-test |
| `s` | Print motion statistics |
| `h` | Show help menu |
| `r` | Restart ESP32 |
| `i` | Print system info |
| `l` | Toggle LED manually |

### Example: System Info (`i`)

```
=== SYSTEM INFO ===
Free Heap: 305456 bytes
Heap Size: 327680 bytes
Chip Model: ESP32-D0WDQ6
CPU Freq: 240 MHz
Flash Size: 4194304 bytes
Uptime: 127 seconds
Sensor Pin: GPIO 13
Motion Events: 5
===================
```

### Example: Motion Statistics (`s`)

```
=== MOTION STATISTICS ===
Total Motion Events: 5
Current State: NO MOTION
Last Motion: 23 seconds ago
Uptime: 150 seconds
=========================
```

## Acceptance Criteria

Stage 1 is complete when:

- [x] Sketch compiles with 0 errors and 0 warnings
- [x] ESP32 boots and runs self-test automatically
- [ ] All self-tests report PASS
- [ ] Sensor detects motion when you move in front of it
- [ ] LED lights up when motion is detected
- [ ] Motion start/stop events are logged with timestamps
- [ ] Serial commands (t/s/h/r/i/l) work correctly
- [ ] Heap memory remains stable over time

## Troubleshooting

### Issue: Sensor always reads LOW (no motion detected)

**Possible causes:**
1. Wiring issue - OUT pin not connected to GPIO 13
2. Power issue - Sensor not receiving 5V on VIN
3. Orientation - Component side not facing detection area

**Solutions:**
1. Verify OUT → GPIO 13 connection
2. Verify VIN → ESP32 VIN (not 3.3V)
3. Verify GND → ESP32 GND
4. Check USB cable provides enough current

### Issue: Sensor always reads HIGH (constant motion)

**Possible causes:**
1. Sensor detecting its own reflection from nearby metal
2. Vibration causing false triggers
3. Electrical noise

**Solutions:**
1. Keep metal objects >1cm from back of sensor
2. Mount sensor rigidly to prevent vibration
3. Add 100nF capacitor between VIN and GND if needed

### Issue: Erratic readings

**Possible causes:**
1. Poor power supply
2. Long wires picking up noise
3. WiFi interference (unlikely at 3.2 GHz vs 2.4 GHz)

**Solutions:**
1. Use short wires (<20cm)
2. Use a quality USB cable
3. Add decoupling capacitor on power

### Issue: Detection range seems short

**Possible causes:**
1. Obstructions in detection path
2. Metal objects behind sensor reducing performance

**Solutions:**
1. Ensure clear line of sight
2. Keep back of sensor >1cm from metal surfaces
3. Note: 5-7m range is typical for indoor environments

## Optional Hardware Modifications

The RCWL-0516 has solder pads on the back for modifications:

| Pad | Modification | Effect |
|-----|--------------|--------|
| C-TM | Add capacitor | Extends trigger time (0.2µF=50s, 1µF=250s) |
| R-GN | Add resistor | Reduces range (1MΩ=5m, 270kΩ=1.5m) |
| R-CDS | Add LDR | Disables sensor in bright conditions |

## Technical Details

### Polling Rate
- Sensor is read at 10Hz (every 100ms)
- This provides responsive detection while avoiding excessive CPU usage

### Motion State Machine
- **MOTION_DETECTED**: Sensor output went HIGH
- **MOTION_ACTIVE**: Sustained motion (keeps retriggering)
- **MOTION_STOPPED**: No motion for 5 seconds (configurable via MOTION_TIMEOUT)

### Memory Usage
- Program storage: ~200KB (15% of flash)
- Global variables: ~20KB (6% of RAM)
- Free heap: ~305KB (well above 100KB minimum)

## Next Steps

Once Stage 1 is working correctly:

1. Verify all acceptance criteria are met
2. Test motion detection from various angles and distances
3. Confirm serial commands work
4. Proceed to Stage 2: WiFi and Web Interface

## References

- [Random Nerd Tutorials: ESP32 RCWL-0516](https://randomnerdtutorials.com/esp32-rcwl-0516-arduino/)
- [Instructables: All About RCWL-0516](https://www.instructables.com/All-About-RCWL-0516-Microwave-Radar-Motion-Sensor/)
- [GitHub: RCWL-0516 Technical Info](https://github.com/jdesbonnet/RCWL-0516)
