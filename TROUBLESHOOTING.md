# Troubleshooting Guide for RCWL-0516 Sensor

This guide covers common issues with the RCWL-0516 microwave radar motion sensor and ESP32.

## Quick Diagnostic

Run the self-test by typing `t` in the serial monitor:

```
=== STAGE 1 SELF TEST ===
  GPIO Configuration (pin 13): PASS (current: LOW)
  LED Configuration (pin 2): PASS (blinked)
  Sensor Reading: PASS (state: NO MOTION)
  Free Heap (305456 bytes): PASS

Overall: ALL TESTS PASSED
=========================
```

If any test fails, see the relevant section below.

---

## Common Issues and Solutions

### Issue: Sensor Always Reads LOW (No Motion Detected)

**Symptoms:**
- Self-test shows "state: NO MOTION" even when moving
- LED never turns on
- No "MOTION DETECTED" messages in serial output

**Most Likely Causes:**

1. **Wiring Error** - Check connections:
   ```
   RCWL-0516    ESP32
   ---------    -----
   VIN      →   VIN (5V)  ← NOT 3.3V!
   GND      →   GND
   OUT      →   GPIO 13
   ```

2. **Power Issue** - Sensor needs 4-28V:
   - Must use ESP32 VIN pin (5V from USB)
   - 3.3V is NOT enough to power the sensor
   - Check USB cable is providing power (data+power cable, not charge-only)

3. **Wrong GPIO Pin** - Verify code matches wiring:
   ```cpp
   #define SENSOR_PIN 13  // Must match your wiring
   ```

**Verification Steps:**
1. Use a multimeter to check 5V at sensor VIN pin
2. Check continuity from sensor OUT to GPIO 13
3. Try a different GPIO pin and update the code

---

### Issue: Sensor Always Reads HIGH (Constant Motion)

**Symptoms:**
- Self-test shows "state: MOTION" even when still
- LED always on
- Constant "MOTION DETECTED" messages

**Most Likely Causes:**

1. **Metal Objects Too Close** - The sensor's back side is sensitive:
   - Keep metal objects >1cm from back of sensor
   - Metal PCB standoffs, enclosures, or surfaces can cause reflections

2. **Vibration** - The sensor is very sensitive:
   - Mount rigidly to prevent vibration-induced triggers
   - Fans, motors, or unstable surfaces can cause false triggers

3. **Electrical Noise** - Poor power quality:
   - Add 100nF ceramic capacitor between VIN and GND at sensor
   - Use shorter wires
   - Use a quality USB power supply

4. **Sensor Orientation** - Component side should face detection area:
   - The side with ICs and components is the "front"
   - The side with solder pads (C-TM, R-GN, R-CDS) is the "back"

---

### Issue: Erratic/Inconsistent Readings

**Symptoms:**
- Random motion detection events
- Detection sometimes works, sometimes doesn't
- Readings seem delayed or laggy

**Most Likely Causes:**

1. **Long Wires** - Keep wires short:
   - Use wires <20cm for reliable operation
   - Long wires can act as antennas and pick up noise

2. **Poor Connections** - Check all connections:
   - Breadboard connections can be unreliable
   - Solder connections for permanent installation

3. **Insufficient Power** - ESP32 + sensor need adequate current:
   - Use a good USB cable and power source
   - Cheap USB cables may have high resistance

---

### Issue: Detection Range Seems Short

**Expected Range:** 5-7 meters in typical indoor environment

**If Range is Shorter:**

1. **Obstructions** - Check line of sight:
   - Sensor detects through plastic, wood, drywall
   - Metal blocks or significantly attenuates the signal
   - Thick walls reduce range

2. **Metal Behind Sensor** - Keep back clear:
   - Metal mounting surfaces reduce performance
   - Maintain >1cm gap from metal

3. **Environmental Factors:**
   - Large metal objects in room can create dead zones
   - Water (pipes, aquariums) can attenuate signal

**Range Reduction (Optional):**
If you want to REDUCE range, add a resistor to R-GN pad:
- 1MΩ = ~5m range
- 270kΩ = ~1.5m range

---

### Issue: LED Not Working

**Symptoms:**
- Motion detected in serial output but LED doesn't light
- Self-test shows LED "blinked" but you didn't see it

**Solutions:**

1. **Check LED Pin** - Different ESP32 boards use different pins:
   ```cpp
   #define LED_PIN 2  // Common for ESP32-WROOM-32
   ```
   Other boards may use different pins (e.g., 5, 22)

2. **LED Polarity** - Some boards have inverted LED logic:
   - Try swapping HIGH/LOW in the code

3. **Damaged LED** - The built-in LED may be faulty:
   - Connect an external LED with resistor to verify

---

### Issue: Heap Memory Warning

**Symptoms:**
- "WARNING: Low heap memory!" message
- Free heap below 100KB

**Solutions:**
1. This shouldn't happen with Stage 1 alone
2. If it occurs, restart ESP32 (type `r`)
3. Check for memory leaks if using modified code

---

## Hardware Verification Checklist

Before troubleshooting software, verify hardware:

- [ ] ESP32 powers on (power LED lit)
- [ ] USB cable is data+power (not charge-only)
- [ ] VIN connected to ESP32 VIN (5V), not 3.3V
- [ ] GND connected to ESP32 GND
- [ ] OUT connected to GPIO 13
- [ ] Sensor 3V3 pin is NOT connected
- [ ] Sensor CDS pin is NOT connected
- [ ] No metal within 1cm of sensor back
- [ ] Sensor mounted rigidly (no vibration)
- [ ] Component side facing detection area

---

## Understanding Sensor Behavior

### Normal Operation

The RCWL-0516 uses Doppler radar to detect motion:

1. **Motion Start**: Output goes HIGH (3.3V)
2. **Continued Motion**: Output stays HIGH (retriggerable)
3. **Motion Stop**: Output stays HIGH for ~2-3 seconds
4. **No Motion**: Output goes LOW (0V)

### What It Detects

- Any moving object (people, animals, fans, etc.)
- Movement through non-metallic materials
- Movement at speeds up to 2 m/s

### What It Doesn't Detect

- Stationary objects (even people sitting still)
- Movement behind metal barriers
- Very slow movement (<0.1 m/s)

---

## Serial Commands for Debugging

| Command | Use For |
|---------|---------|
| `t` | Run self-test - checks GPIO, LED, sensor reading |
| `s` | View motion statistics - event count, current state |
| `i` | System info - heap, uptime, chip details |
| `r` | Restart ESP32 - fresh start |
| `l` | Toggle LED - verify LED works independently |

---

## Factory Reset Not Needed

Unlike the C4001 sensor, the RCWL-0516 has no software configuration to corrupt. If you're having issues:

1. Check wiring
2. Check power
3. Check for metal/vibration interference
4. Replace sensor if damaged (~$1-2)

There's no firmware to brick and no recovery tool needed!

---

## Getting Help

If still having issues after following this guide:

1. Run self-test (`t`) and note results
2. Run system info (`i`) and note details
3. Check wiring matches the diagram exactly
4. Measure voltages with multimeter:
   - VIN at sensor should be ~5V
   - OUT at sensor should be 0V (no motion) or 3.3V (motion)

## References

- [Random Nerd Tutorials: ESP32 RCWL-0516](https://randomnerdtutorials.com/esp32-rcwl-0516-arduino/)
- [Instructables: All About RCWL-0516](https://www.instructables.com/All-About-RCWL-0516-Microwave-Radar-Motion-Sensor/)
- [GitHub: RCWL-0516 Technical Info](https://github.com/jdesbonnet/RCWL-0516)
- [Microcontrollers Lab: RCWL-0516 with ESP32](https://microcontrollerslab.com/rcwl-0516-microwave-radar-sensor-esp32/)
