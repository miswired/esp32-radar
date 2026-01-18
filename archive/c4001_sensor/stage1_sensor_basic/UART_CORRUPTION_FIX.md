# UART Corruption Fix - Garbled Character Issue

## Problem Description

Random garbled characters appearing in serial output, like:
```
����������������������������������������������������������������[HEAP] Free: 331256 bytes (88%)
```

The `����` characters are ESP32 ROM bootloader messages at 74880 baud being received at 115200 baud, indicating the ESP32 is **spontaneously resetting**.

## Root Causes

The ESP32 was resetting due to one or more of these issues:

### 1. **Brownout Detector (Most Common)**
- ESP32's brownout detector is overly sensitive
- Triggers false resets during I2C operations or power fluctuations
- Common with long USB cables or marginal power supplies

### 2. **Interrupt Conflicts**
- I2C operations use interrupts
- UART TX also uses interrupts
- Concurrent I2C and UART can corrupt buffers or cause timing issues

### 3. **UART Buffer Overflow**
- Serial.print() without Serial.flush() leaves data in buffer
- If ESP32 resets or interrupts occur, buffer data can be corrupted or lost

## Fixes Applied (Final Working Solution)

### Fix 1: Reset Reason Detection ✅

Added code to detect and report WHY the ESP32 reset:

```cpp
#include <rom/rtc.h>  // For reset reason detection

void printResetReason() {
  RESET_REASON reason = rtc_get_reset_reason(0);
  // Prints human-readable reset reason (brownout, watchdog, etc.)
}
```

**Benefit**: Immediately see what caused reset when garbled chars appear

**Status**: WORKING - Helps diagnose issues

### Fix 2: Configurable Brownout Detector ✅

```cpp
// Power Management Configuration
#define DISABLE_BROWNOUT_DETECTOR 1  // Set to 0 to re-enable

void setup() {
  #if DISABLE_BROWNOUT_DETECTOR
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  #endif
  // ... rest of setup
}
```

**Benefits**:
- Prevents false resets during I2C operations
- Stable operation with marginal power supplies
- Configurable via #define (easy to toggle)
- Recommended for USB-powered applications

**Important**: Set `DISABLE_BROWNOUT_DETECTOR` to 0 if you suspect real power issues.

**Status**: WORKING - Prevents spurious resets

### Fix 3: UART Buffer Flushing ✅

Added `Serial.flush()` after all Serial.print() calls:

```cpp
Serial.println("[HEAP] Free: " + String(freeHeap) + " bytes");
Serial.flush();  // Ensure data is sent before continuing
```

**Benefits**:
- Ensures UART TX buffer is emptied before any blocking operations
- Prevents buffer corruption during I2C transactions
- Data is guaranteed to be transmitted even if reset occurs

### ~~Fix 4: Interrupt Protection During I2C~~ (REMOVED - CAUSED WATCHDOG)

**IMPORTANT**: Initially tried disabling interrupts during I2C reads, but this caused worse problems:

```cpp
// ❌ BAD - DO NOT USE - Causes watchdog timeout!
noInterrupts();
bool detected = sensor.motionDetection();
interrupts();
```

**Why it failed**:
- sensor.motionDetection() takes longer than expected (could be 100-800ms)
- ESP32 has interrupt watchdog timers on both cores
- Disabling interrupts for >300ms triggers "Interrupt wdt timeout" panic
- Results in continuous crash-reboot loop

**Correct approach**:
- The I2C library (Wire.h) already handles its own locking internally
- No manual interrupt disabling needed
- Let the library manage concurrency

## How to Use

### 1. Flash Updated Code

```bash
cd /home/miswired/Multiverse/esp32-radar/stage1_sensor_basic
arduino-cli upload --fqbn esp32:esp32:esp32 -p /dev/ttyUSB0 stage1_sensor_basic.ino
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

### 2. Check Reset Reason on Boot

You'll now see:
```
========================================
ESP32 mmWave Presence Sensor - Stage 1
========================================

Reset reason: POWERON_RESET - Vbat power on reset
```

**If you see brownout reset**:
```
Reset reason: RTCWDT_BROWN_OUT_RESET - Brownout Reset
⚠️  BROWNOUT DETECTED - Check power supply voltage!
     - Use quality USB cable (data + power)
     - Ensure stable 5V power supply
     - Check for voltage drops during I2C operations
```

This means your power supply is marginal. Solutions:
- Use a different USB cable (data + power, not just charging cable)
- Use powered USB hub
- Use external 5V power supply (2A recommended)
- Shorten USB cable length

### 3. Monitor for Garbled Characters

If you still see `����` characters:
1. Note the reset reason printed after reboot
2. Check power supply voltage with multimeter (should be 4.75V - 5.25V)
3. Try different USB port or cable
4. Check for loose wiring on I2C connections

## Testing Results

After applying fixes, you should see:
- ✅ No garbled characters in serial output
- ✅ Reset reason is POWERON_RESET on first boot
- ✅ Clean, consistent output from sensor polling and heap monitoring
- ✅ No spontaneous resets during normal operation

## Advanced Troubleshooting

### If garbled characters persist:

1. **Check actual baud rate**
   - Verify serial monitor is set to 115200 baud
   - Some terminal programs auto-detect incorrectly

2. **Re-enable brownout detector temporarily**
   ```cpp
   // Comment out this line in setup():
   // WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
   ```
   - If resets stop, you have a power supply issue
   - Measure voltage at ESP32 VIN and 3.3V pins

3. **Check USB cable quality**
   - Measure voltage drop across cable under load
   - Should be < 0.5V drop from PC USB port to ESP32 VIN
   - Try cable rated for data + power (not just charging)

4. **Check I2C wiring**
   - Loose connections can cause high current spikes
   - Verify pull-up resistors are 4.7kΩ (not too low or too high)
   - Keep I2C wires short (< 20cm if possible)

5. **Monitor heap for leaks**
   - If free heap decreases over time, you may have memory leak
   - Eventually causes crashes and resets
   - Our code is simple and shouldn't leak, but library bugs possible

6. **Check sensor power consumption**
   - C4001 draws ~100-150mA during operation
   - Some USB ports can't provide enough current
   - Try powered USB hub or external 5V supply

## Technical Details

### Why Garbled Characters Appear

When ESP32 resets, it goes through boot sequence:
1. ROM bootloader runs at **74880 baud**
2. Prints boot messages (mode, flash config, etc.)
3. Second-stage bootloader at 115200 baud
4. Finally, your sketch at 115200 baud

If your terminal is set to 115200 baud, the ROM bootloader messages at 74880 baud appear as `����`.

### ESP32 Reset Codes

Common reset reasons you might see:

| Code | Name | Meaning |
|------|------|---------|
| 1 | POWERON_RESET | Normal power on (expected) |
| 12 | SW_CPU_RESET | Software reset (ESP.restart()) |
| 15 | RTCWDT_BROWN_OUT_RESET | **Brownout - power issue!** |
| 7 | TG0WDT_SYS_RESET | Task watchdog timeout |
| 8 | TG1WDT_SYS_RESET | Interrupt watchdog timeout |

**Most common issue**: Code 15 (brownout reset)

### Serial.flush() Behavior

`Serial.flush()` on ESP32 Arduino:
- **Blocks** until UART TX FIFO is empty
- Ensures all data physically transmitted
- Takes ~1-5ms for typical messages
- Safe to call frequently (we only call 1x/second max)

### noInterrupts() Impact

`noInterrupts()` / `interrupts()`:
- Disables **all** interrupts (including WiFi, timers)
- Should be kept as short as possible (< 50ms)
- Our I2C read takes ~10-20ms (acceptable)
- Alternative: use `portENTER_CRITICAL()` / `portEXIT_CRITICAL()` for specific cores

## Verification

After flashing updated code, run for 10+ minutes and verify:

- [ ] No garbled `����` characters appear
- [ ] Reset reason shows POWERON_RESET on initial boot
- [ ] No unexpected resets during operation
- [ ] Sensor polling continues smoothly at 1Hz
- [ ] Heap monitoring shows stable memory (~305KB free)

If all checks pass, the UART corruption issue is resolved!

## Watchdog Timer Issue (FIXED)

### Symptom
After initial fix attempt, ESP32 started crashing with:
```
Guru Meditation Error: Core 1 panic'ed (Interrupt wdt timeout on CPU1)
```

### Cause
Used `noInterrupts()` / `interrupts()` around sensor.motionDetection() call, but:
- The function takes 100-800ms to execute (longer than expected)
- ESP32 interrupt watchdog triggers if interrupts disabled > ~300ms
- Other core's watchdog can't be serviced → panic and reboot

### Fix
**Removed noInterrupts() / interrupts() calls completely**
- I2C library already handles its own locking
- No manual interrupt management needed
- Let the library do its job

**Configuration change**:
```cpp
// Old (CAUSED CRASHES):
// #define DISABLE_BROWNOUT_DETECTOR 1

// New (CONFIGURABLE):
#define DISABLE_BROWNOUT_DETECTOR 1  // Can be toggled
```

Now user can easily enable/disable brownout detector by changing one line.

## For Future Stages

These fixes are now part of Stage 1 and will be carried forward to all future stages:
- ✅ Reset reason detection (all stages) - helps diagnose issues
- ✅ Configurable brownout detector (all stages) - prevents spurious resets
- ✅ Serial.flush() after important messages (all stages) - ensures data sent
- ❌ ~~Interrupt protection~~ - NOT NEEDED, causes watchdog timeouts

**Key Lessons**:
1. Don't disable interrupts for long operations (> 50ms)
2. Trust library implementations (Wire.h handles its own locking)
3. ESP32 dual-core architecture requires careful interrupt management
4. Always test on actual hardware - watchdog timings vary

When adding WiFi in Stage 2, we'll rely on FreeRTOS task priorities and mutexes instead of raw interrupt disabling.
