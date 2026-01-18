# C4001 Sensor Archive

**Archived:** January 2026
**Reason:** Hardware reliability issues

## Summary

This folder contains the original Stage 1 implementation using the DFRobot SEN0610 C4001 24GHz mmWave FMCW radar sensor. The implementation was fully functional but the sensor hardware proved unreliable and was replaced with a simpler alternative.

## Contents

- `stage1_sensor_basic/` - Complete Stage 1 implementation with I2C communication
- `sensor_recovery/` - Factory reset tool for recovering bricked sensors
- `i2c_scanner/` - I2C bus scanner utility
- `TROUBLESHOOTING.md` - Comprehensive troubleshooting guide

## Why We Switched Sensors

### The C4001 Sensor Issues

1. **Sensor Bricking**: During sensitivity configuration, the sensor would occasionally enter an unrecoverable state where:
   - The white presence LED stopped functioning
   - The green I2C activity LED would blink but sensor wouldn't respond correctly
   - Configuration values read back as 0 or unexpected values
   - Required factory reset via `eRecoverSen` command, and sometimes even that failed

2. **Configuration Fragility**: The sensor stores configuration in internal flash memory. Certain parameter combinations (particularly sensitivity values) could put the sensor into a bad state that required recovery or replacement.

3. **Complex I2C Protocol**: Required:
   - External 4.7kΩ pull-up resistors
   - DIP switch configuration (I2C vs UART mode)
   - Multiple configuration calls (range, sensitivity, delays)
   - Status register verification
   - Known library issues with certain commands

4. **Inconsistent Behavior**: Even with identical code and configuration, sensors would behave differently or stop working after firmware updates.

5. **Recovery Tool Required**: We had to develop a dedicated recovery sketch (`sensor_recovery.ino`) to factory reset bricked sensors using the `eRecoverSen` (0xAA) command.

### DFRobot Forum Reports

These issues are not unique to our project. The [DFRobot forums](https://www.dfrobot.com/forum/topic/335591) document similar problems with users unable to reset sensor configuration after accidentally changing UART baud rates or other settings.

## What Worked

Despite the issues, the following was successfully implemented:

- ✅ I2C communication at 100kHz
- ✅ Sensor mode configuration (eExitMode for presence detection)
- ✅ Detection range configuration (30-800cm)
- ✅ Sensitivity configuration (when it worked)
- ✅ Status register reading
- ✅ Presence/motion detection polling
- ✅ Serial debug interface
- ✅ Self-test framework
- ✅ Factory reset recovery tool

## Replacement Sensor

We replaced the C4001 with the **RCWL-0516 Microwave Radar Motion Sensor**:

| Feature | C4001 | RCWL-0516 |
|---------|-------|-----------|
| Interface | I2C (complex) | Simple GPIO (HIGH/LOW) |
| Configuration | Multiple parameters | None required |
| Pull-up resistors | Required (4.7kΩ) | Not needed |
| DIP switches | Required | None |
| Recovery tool | Required | Not needed |
| Reliability | Problematic | Proven reliable |
| Cost | ~$20-30 | ~$1-2 |
| Range | 1.2-12m configurable | 5-7m fixed |
| Sensitivity | Configurable (0-9) | Fixed |

The RCWL-0516 trades configurability for simplicity and reliability. For presence detection applications, the fixed behavior is acceptable.

## Lessons Learned

1. **Simpler is often better** - The C4001's configurability introduced complexity and failure modes
2. **Test hardware reliability early** - We should have stress-tested configuration changes before building on top
3. **Have a recovery plan** - The recovery tool was essential but shouldn't have been necessary
4. **Check community feedback** - Forum reports indicated known issues we could have discovered earlier

## Reusing This Code

If you want to try the C4001 sensor:

1. The code in `stage1_sensor_basic/` is fully functional
2. Use `sensor_recovery/` if the sensor becomes unresponsive
3. Follow `TROUBLESHOOTING.md` for common issues
4. Be cautious with sensitivity settings (stay in 1-5 range)
5. Always power cycle after configuration changes
6. Have a spare sensor ready

## References

- [DFRobot C4001 Wiki](https://wiki.dfrobot.com/SKU_SEN0610_Gravity_C4001_mmWave_Presence_Sensor_12m_I2C_UART)
- [DFRobot Forum - Known Issues](https://www.dfrobot.com/forum/topic/335591)
- [DFRobot_C4001 Library](https://github.com/cdjq/DFRobot_C4001)
