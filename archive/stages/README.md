# Archived Development Stages

This folder contains the historical development stages of the ESP32 Microwave Motion Sensor project. Each stage represents a milestone in the project's evolution.

**For the latest firmware, use the `firmware/` folder in the project root.**

## Stage Overview

| Stage | Folder | Description |
|-------|--------|-------------|
| 1 | stage1_sensor_basic | Basic motion detection with LED indicator |
| 2 | stage2_wifi_web | WiFi connectivity and web dashboard |
| 3 | stage3_persistence | NVS settings persistence |
| 4 | stage4_notifications | HTTP webhook notifications |
| 5 | *(integrated)* | Adaptive motion filtering |
| 6 | stage6_full_system | Diagnostics page and system integration |
| 7 | stage7_api_auth | API key and web authentication |
| 8 | stage8_wled | WLED integration for visual alerts |
| 9 | stage9_homeassistant | Home Assistant MQTT integration |

## Usage

These stages are preserved for:
- **Learning**: Understanding how the project evolved
- **Debugging**: Testing individual features in isolation
- **Reference**: Seeing how specific features were implemented

To compile a specific stage:

```bash
cd archive/stages/stage<N>_<name>
arduino-cli compile --fqbn esp32:esp32:esp32 .
```

## Note

Stage 5 (Adaptive Filter) was integrated directly into Stage 6 and does not have a separate folder.
