# Archived Libraries

This folder contains archived copies of the Arduino libraries used by this project. These are the exact versions that have been tested and verified to work.

## Included Libraries

| Library | Version | License | Description |
|---------|---------|---------|-------------|
| ArduinoJson | 7.4.2 | MIT | JSON serialization/deserialization |
| PubSubClient | 2.8 | MIT | MQTT client for Arduino |

## Installation

### Option 1: Install from Arduino Library Manager (Recommended)

```bash
arduino-cli lib install ArduinoJson@7.4.2
arduino-cli lib install PubSubClient@2.8
```

### Option 2: Install from these archived copies

Copy the library folders to your Arduino libraries directory:

**Linux:**
```bash
cp -r ArduinoJson PubSubClient ~/Arduino/libraries/
```

**macOS:**
```bash
cp -r ArduinoJson PubSubClient ~/Documents/Arduino/libraries/
```

**Windows:**
```powershell
Copy-Item -Recurse ArduinoJson, PubSubClient "$env:USERPROFILE\Documents\Arduino\libraries\"
```

## Why Archive Libraries?

- **Reproducibility**: Ensures the exact library versions are available even if newer versions introduce breaking changes
- **Offline builds**: Allows compiling without internet access to download libraries
- **Version control**: Documents which library versions were tested with this firmware

## Library Sources

- ArduinoJson: https://github.com/bblanchon/ArduinoJson
- PubSubClient: https://github.com/knolleary/pubsubclient
