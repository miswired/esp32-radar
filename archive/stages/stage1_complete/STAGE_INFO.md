# Stage 1: Basic Sensor Interface - Complete

**Archived:** January 2026
**Git Commit:** c98e3c6

## Features Implemented

- RCWL-0516 GPIO communication (10Hz polling)
- Trip delay state machine (3 seconds default)
- Clear timeout (30 seconds default)
- 4 states: IDLE → MOTION_PENDING → ALARM_ACTIVE → ALARM_CLEARING
- LED feedback (ON when alarm active)
- Serial commands: t/s/c/h/i/r/l
- Self-tests (GPIO, LED, sensor, heap)

## Configuration

| Parameter | Value |
|-----------|-------|
| Sensor Pin | GPIO 13 |
| LED Pin | GPIO 2 |
| Trip Delay | 3 seconds |
| Clear Timeout | 30 seconds |
| Poll Interval | 100ms (10Hz) |

## This is a working snapshot

Use this as a reference or rollback point if Stage 2 has issues.
