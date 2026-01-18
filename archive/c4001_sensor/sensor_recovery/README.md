# C4001 Sensor Recovery Tool

Factory reset tool for the DFRobot C4001 mmWave sensor when it gets into a bad state.

## When to Use This Tool

Run this recovery tool if you observe any of these symptoms:

- **White LED doesn't light up** (normally indicates presence detection)
- **Green LED blinks** with I2C commands but sensor doesn't respond correctly
- **Configuration values read back as 0** or unexpected values
- **Sensor worked before** but stopped after changing sensitivity or other settings
- **Self-test shows "Config FAIL"** even though I2C communication passes
- **Sensor detected on I2C bus** but `begin()` fails or returns wrong status

## What Causes Sensor Issues

The C4001 sensor stores configuration in internal flash memory. Certain invalid configurations (especially extreme sensitivity values or timing parameters outside valid ranges) can put the sensor into an unresponsive state.

This is a known issue discussed on the [DFRobot forums](https://www.dfrobot.com/forum/topic/335591).

## How to Run

```bash
# Compile, upload, and monitor
arduino-cli compile --fqbn esp32:esp32:esp32 /home/miswired/Multiverse/esp32-radar/sensor_recovery && \
arduino-cli upload --fqbn esp32:esp32:esp32 -p /dev/ttyUSB0 /home/miswired/Multiverse/esp32-radar/sensor_recovery && \
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

## What the Tool Does

1. **Scans I2C bus** to find the sensor at 0x2A or 0x2B
2. **Reads current status** before recovery
3. **Stops the sensor** - sends `eStopSen` (0x33)
4. **Factory resets** - sends `eRecoverSen` (0xAA)
5. **System resets** - sends `eResetSen` (0xCC)
6. **Reinitializes** and reads back configuration
7. **Starts the sensor** - sends `eStartSen` (0x55)
8. **Reports final status**

## Recovery Commands (Library Reference)

| Command | Value | Description |
|---------|-------|-------------|
| `eStartSen` | 0x55 | Start sensor collection |
| `eStopSen` | 0x33 | Stop sensor collection |
| `eResetSen` | 0xCC | System reset |
| `eRecoverSen` | 0xAA | **Factory data reset** |
| `eSaveParams` | 0x5C | Save config to flash |

## Interactive Commands

While the recovery tool is running, you can type:

- **s** - Scan I2C bus again
- **r** - Retry the recovery process
- **p** - Show power cycle instructions

## After Recovery

1. **Power cycle the sensor** - unplug VCC wire, wait 5 seconds, reconnect
2. **Run Stage 1 sketch** - verify all self-tests pass
3. **Check configuration** - type 'c' in Stage 1 to see current config

## If Recovery Fails

If the sensor still doesn't work after recovery:

1. Try **multiple recovery attempts** with power cycles between each
2. Switch to **UART mode** (DIP switch) and try serial commands
3. Check all **wiring and pull-up resistors**
4. The sensor may be **hardware damaged** and need replacement

## Technical Details

The recovery uses the DFRobot_C4001 library's `setSensor()` function with the `eRecoverSen` parameter, which internally sends the "resetCfg" command to restore factory defaults.

## References

- [DFRobot Forum - SEN0609 major issues](https://www.dfrobot.com/forum/topic/335591)
- [DFRobot C4001 Library](https://github.com/cdjq/DFRobot_C4001)
- [DFRobot Wiki - SEN0610](https://wiki.dfrobot.com/SKU_SEN0610_Gravity_C4001_mmWave_Presence_Sensor_12m_I2C_UART)
