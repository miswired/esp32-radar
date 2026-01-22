# Bill of Materials

## Required Components

| Component | Quantity | Description | Notes |
|-----------|----------|-------------|-------|
| ESP32-WROOM-32 | 1 | ESP32 development board | Any ESP32 DevKit will work |
| RCWL-0516 | 1 | Microwave radar motion sensor | 3.3V-20V input, digital output |
| USB Cable | 1 | Micro-USB or USB-C (depends on board) | For power and programming |
| Jumper Wires | 3 | Female-to-female or as needed | VIN, GND, OUT connections |

## Optional Components

| Component | Quantity | Description | Notes |
|-----------|----------|-------------|-------|
| Enclosure | 1 | Project box or 3D printed case | Protect electronics |
| 5V Power Supply | 1 | USB power adapter | For permanent installation |
| WLED Controller | 1 | ESP8266/ESP32 with WLED firmware | For visual alerts |

## Component Details

### ESP32-WROOM-32

The ESP32 is the main microcontroller. Any ESP32 development board should work.

**Tested boards:**
- ESP32 DevKit V1
- ESP32-WROOM-32D
- NodeMCU-32S

**Requirements:**
- USB port for programming
- 3.3V and 5V power rails
- Available GPIO pins

### RCWL-0516

Microwave Doppler radar motion sensor module.

**Specifications:**
- Operating voltage: 4-28V DC (typically 5V)
- Operating frequency: 3.18 GHz
- Detection range: 5-7 meters (adjustable via potentiometer if present)
- Output: Digital HIGH (3.3V) when motion detected
- Trigger time: ~2 seconds (adjustable on some boards)

**Notes:**
- Detects motion through walls and plastic enclosures
- Sensitive to electrical interference
- May require shielding in noisy environments

## Where to Buy

Components are available from:
- Amazon
- AliExpress
- Adafruit
- SparkFun
- DigiKey
- Mouser

Search for the exact component names listed above.

## Cost Estimate

| Component | Approximate Cost (USD) |
|-----------|----------------------|
| ESP32 DevKit | $5-15 |
| RCWL-0516 | $1-3 |
| Jumper wires | $2-5 (pack) |
| **Total** | **$8-23** |

Prices vary by region and supplier. Buying in bulk significantly reduces costs.
