# ESP32 River Levels Display

Real-time USGS river data display for ESP32-3248S035C development board with 480x320 TFT display.

![Hardware](https://img.shields.io/badge/Hardware-ESP32--3248S035C-blue)
![Display](https://img.shields.io/badge/Display-ST7796%20480x320-green)
![License](https://img.shields.io/badge/License-MIT-yellow)

## Table of Contents

- [Overview](#overview)
- [Hardware Requirements](#hardware-requirements)
- [Software Installation from Scratch](#software-installation-from-scratch)
- [Critical TFT_eSPI Configuration](#critical-tft_espi-configuration)
- [WiFi Configuration](#wifi-configuration)
- [Building and Uploading](#building-and-uploading)
- [Troubleshooting](#troubleshooting)
- [Project Structure](#project-structure)
- [Display Layout](#display-layout)
- [API Information](#api-information)

## Overview

This project displays real-time river level data from USGS monitoring stations on a 3.5" TFT display. It shows water levels, flow rates, trends (rising/falling/steady), and weather information for 6 rivers in Alabama.

**Features:**
- 480x320 landscape display with 6 river cards
- Auto-refresh every 5 minutes
- Elapsed time updates every 60 seconds
- Color-coded status indicators (green=optimal, red=low flow)
- WiFi connectivity for API data fetch
- Weather data integration (temperature, wind)
- Quantitative Precipitation Forecast (QPF)

**Monitored Rivers:**
1. Locust Fork (USGS 02455000)
2. Town Creek (USGS 03572900)
3. South Sauty (USGS 03572690)
4. Little River (USGS 02399200)
5. Short Creek (USGS 03574500)
6. Mulberry Fork (USGS 02450000)

## Hardware Requirements

- **Board:** ESP32-3248S035C (Sunton/DIYmall)
  - MCU: ESP32-WROOM-32 (WiFi + Bluetooth)
  - Display: 3.5" TFT LCD, 480x320 pixels
  - Driver: ST7796
  - Interface: SPI
  - Touch: XPT2046 (optional, not used in this project)

- **USB Cable:** USB-C for programming and power
- **Power:** 5V via USB or external power supply

## Software Installation from Scratch

### 1. Install Arduino CLI

```bash
# Create local bin directory
mkdir -p $HOME/local/bin

# Download and install Arduino CLI (Linux x64)
cd $HOME/local/bin
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh

# Add to PATH (add this to ~/.bashrc or ~/.bash_profile)
export PATH="$HOME/local/bin:$PATH"

# Verify installation
arduino-cli version
```

### 2. Configure Arduino CLI for ESP32

```bash
# Initialize Arduino CLI config
arduino-cli config init

# Add ESP32 board manager URL
arduino-cli config add board_manager.additional_urls https://espressif.github.io/arduino-esp32/package_esp32_index.json

# Update core index
arduino-cli core update-index

# Install ESP32 platform
arduino-cli core install esp32:esp32
```

**Note:** ESP32 packages will be installed to `$HOME/.arduino15/packages/esp32`

### 3. Install Required Libraries

```bash
# Update library index
arduino-cli lib update-index

# Install ArduinoJson (for JSON parsing)
arduino-cli lib install ArduinoJson

# Install TFT_eSPI (for display control)
arduino-cli lib install TFT_eSPI

# Verify libraries are installed
arduino-cli lib list
```

**Libraries will be installed to:** `$HOME/Arduino/libraries/`

## Critical TFT_eSPI Configuration

⚠️ **IMPORTANT:** The TFT_eSPI library requires hardware-specific configuration for the ESP32-3248S035C board. **This is the most common source of problems** - if the display doesn't turn on, this configuration is likely incorrect.

### The Problem

By default, TFT_eSPI is configured for ILI9341 displays with generic ESP8266 pins. The ESP32-3248S035C uses:
- **Different driver:** ST7796 (not ILI9341)
- **Different pins:** ESP32-specific GPIO pins (not ESP8266 PIN_Dx)

### The Solution

Edit the file: `$HOME/Arduino/libraries/TFT_eSPI/User_Setup.h`

#### Required Changes:

**1. Enable ST7796 Driver (around line 41)**
```cpp
// Change this:
#define ILI9341_DRIVER       // WRONG - default driver

// To this:
//#define ILI9341_DRIVER      // Comment out
#define ST7796_DRIVER        // CORRECT for ESP32-3248S035C
```

**2. Configure ESP32-3248S035C Pins (around lines 70-79)**
```cpp
// *** ESP32-3248S035C Pin Configuration (Sunton/DIYmall Board) ***
#define TFT_MISO 12   // SPI MISO
#define TFT_MOSI 13   // SPI MOSI
#define TFT_SCLK 14   // SPI Clock
#define TFT_CS   15   // Chip select control pin
#define TFT_DC    2   // Data Command control pin
#define TFT_RST  -1   // Reset pin (connected to EN, use -1)

// Backlight control (PWM capable)
#define TFT_BL   27            // LED back-light control pin
#define TFT_BACKLIGHT_ON HIGH  // Level to turn ON back-light (HIGH or LOW)
```

**3. Set SPI Frequency (around line 126)**
```cpp
#define SPI_FREQUENCY  80000000   // ST7796 can run at 80MHz for fast rendering
```

### Complete User_Setup.h Template

A complete, working `User_Setup.h` file is provided in this repository. You can copy it directly:

```bash
# Backup original configuration
cp $HOME/Arduino/libraries/TFT_eSPI/User_Setup.h $HOME/Arduino/libraries/TFT_eSPI/User_Setup.h.backup

# Option 1: Manually edit the file
nano $HOME/Arduino/libraries/TFT_eSPI/User_Setup.h

# Option 2: Copy from a working configuration (if available)
# cp /path/to/working/User_Setup.h $HOME/Arduino/libraries/TFT_eSPI/User_Setup.h
```

**Key Pin Mapping for ESP32-3248S035C:**

| Function | GPIO Pin | Notes |
|----------|----------|-------|
| TFT_MISO | GPIO 12  | SPI Master In Slave Out |
| TFT_MOSI | GPIO 13  | SPI Master Out Slave In |
| TFT_SCLK | GPIO 14  | SPI Clock |
| TFT_CS   | GPIO 15  | Chip Select |
| TFT_DC   | GPIO 2   | Data/Command |
| TFT_RST  | -1       | Reset (connected to EN) |
| TFT_BL   | GPIO 27  | Backlight (PWM) |
| TOUCH_CS | GPIO 33  | Touch chip select (optional) |

## WiFi Configuration

Create a file named `secrets.h` in the project directory:

```cpp
#ifndef SECRETS_H
#define SECRETS_H

// WiFi Credentials
const char* WIFI_SSID = "Your_WiFi_SSID";
const char* WIFI_PASSWORD = "Your_WiFi_Password";

#endif
```

⚠️ **Security Note:** The `secrets.h` file is excluded from version control (via `.gitignore`). Never commit WiFi credentials to a repository.

## Building and Uploading

### Compile the Sketch

```bash
cd /home/mdchansl/IOT/ESP32-320-480-display-horizontal

# Compile with build directory
arduino-cli compile --build-path ./build --fqbn esp32:esp32:esp32 .

# Or compile without specifying build directory (uses default)
arduino-cli compile --fqbn esp32:esp32:esp32 ESP32-320-480-display-horizontal.ino
```

### Upload to Board

```bash
# Make sure the ESP32 is connected via USB
# Check which port it's connected to:
ls /dev/ttyUSB*
# or
ls /dev/ttyACM*

# Upload the compiled sketch
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32 --input-dir ./build .

# Or without build directory specified:
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32 ESP32-320-480-display-horizontal.ino
```

### Monitor Serial Output

```bash
# Monitor serial output for debugging
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

**Expected Serial Output:**
```
=== ESP32-3248S035C River Levels Display (HORIZONTAL) ===
Connecting to WiFi: Your_SSID
WiFi connected!
IP address: 192.168.1.XXX
Fetching data from API... Success!
Locust Fork: 1234 rising
Town Creek: 567 steady
...
```

## Troubleshooting

### Display Does Not Turn On (Black Screen)

**Symptom:** Sketch uploads successfully, but display remains black/off.

**Root Cause:** Incorrect TFT_eSPI configuration (wrong driver or pins).

**Solution:**
1. Verify `User_Setup.h` has `ST7796_DRIVER` enabled (not ILI9341)
2. Verify pin definitions match ESP32-3248S035C hardware (see [Critical TFT_eSPI Configuration](#critical-tft_espi-configuration))
3. Check backlight pin is defined: `TFT_BL 27`
4. Recompile after making changes to `User_Setup.h`

### WiFi Connection Fails

**Symptom:** Display shows "WiFi Failed - Check credentials"

**Solutions:**
- Verify `secrets.h` exists with correct SSID and password
- Check WiFi network is 2.4GHz (ESP32 does not support 5GHz)
- Ensure WiFi is within range
- Check serial monitor for WiFi status codes

### Display Shows Garbage/Corrupted Graphics

**Possible Causes:**
- SPI frequency too high - try reducing to 40MHz or 27MHz
- Incorrect color order - try adding `#define TFT_RGB_ORDER TFT_BGR`
- Loose connections (if using breadboard/jumper wires)

### API Data Not Showing

**Symptom:** Display turns on, WiFi connects, but river data shows as "---" or zeros.

**Solutions:**
- Check serial monitor for API response
- Verify API endpoint is accessible: `https://docker-blue-sound-1751.fly.dev/api/river-levels`
- Check JSON parsing errors in serial output
- Ensure ArduinoJson library is installed (minimum version 6.x)

### Build Directory Issues

**Different Users, Different Results:**

If the same code works for one user but not another on the same system:
- Check library paths: `$HOME/Arduino/libraries/TFT_eSPI/`
- Verify `User_Setup.h` is correctly configured in each user's library directory
- Libraries are **per-user**, not system-wide
- Each user needs their own TFT_eSPI configuration

**Our Specific Case:**
- User `mdc`: Had correct ST7796 configuration, program worked
- User `mdchansl`: Had default ILI9341 configuration, display stayed black
- **Fix:** Updated `mdchansl`'s `User_Setup.h` to match working configuration

### Permission Issues

If you get permission errors accessing `/dev/ttyUSB0`:

```bash
# Add user to dialout group
sudo usermod -a -G dialout $USER

# Log out and log back in for changes to take effect
```

## Project Structure

```
ESP32-320-480-display-horizontal/
├── ESP32-320-480-display-horizontal.ino  # Main sketch file
├── secrets.h                              # WiFi credentials (not in git)
├── CLAUDE.md                              # AI assistant instructions
├── README.md                              # This file
├── build/                                 # Compiled binaries (generated)
└── build_not_working/                     # Backup of non-working build
```

**Single-File Architecture:**
- Entire application in one `.ino` file
- No separate headers or modules
- All functions defined in main sketch

## Display Layout

**Screen Dimensions:** 480x320 pixels (landscape orientation)

**Layout Structure:**
```
┌─────────────────────────────────────────────────────┐
│ RIVER LEVELS              Updated: 2m ago   (30px) │ ← Header
├─────────────────────────────────────────────────────┤
│ ● Locust Fork      1.2'    1234 cfs   ^rising      │ ← River Card 1 (46px)
├─────────────────────────────────────────────────────┤
│ ● Town Creek       2.5'     567 cfs   -steady      │ ← River Card 2 (46px)
├─────────────────────────────────────────────────────┤
│ ● South Sauty      3.1'     890 cfs   vfalling     │ ← River Card 3 (46px)
├─────────────────────────────────────────────────────┤
│ ● LRC              1.8'     432 cfs   ^rising      │ ← River Card 4 (46px)
├─────────────────────────────────────────────────────┤
│ ● Short Creek      2.2'     678 cfs   -steady      │ ← River Card 5 (46px)
├─────────────────────────────────────────────────────┤
│ ● Mulberry Fork    1.5'     321 cfs   vfalling     │ ← River Card 6 (46px)
└─────────────────────────────────────────────────────┘
```

**Color Coding:**
- 🟢 **Green dot:** River flow is in optimal range
- 🔴 **Red dot:** River flow is below minimum (too low)
- 🟡 **Yellow dot:** Warning condition

**Trend Indicators:**
- `^rising` (cyan) - Water level increasing
- `vfalling` (orange) - Water level decreasing
- `-steady` (white) - Water level stable

## API Information

**Endpoint:** `https://docker-blue-sound-1751.fly.dev/api/river-levels`

**Response Format:**
```json
{
  "sites": [
    {
      "site_id": "02455000",
      "name": "Locust Fork at Cleveland, AL",
      "flow": "1234",
      "trend": "rising",
      "stage_ft": 1.2,
      "in_range": true,
      "timestamp": "2024-11-22T12:00:00Z",
      "qpf": {
        "today": 0.0,
        "tomorrow": 0.15
      },
      "weather": {
        "temp_f": 65.0,
        "wind_mph": 8.5,
        "wind_dir": "NW"
      }
    },
    ...
  ]
}
```

**Update Frequency:**
- API called every 5 minutes
- Display elapsed time updated every 60 seconds
- Non-blocking timing using `millis()`

## Development Notes

### Timer Management

Two independent timers prevent unnecessary full redraws:
- `lastUpdate`: Tracks 5-minute API refresh cycle
- `lastDisplayUpdate`: Tracks 60-second elapsed time updates

Both reset when full refresh occurs to keep them synchronized.

### Memory Considerations

- Uses `DynamicJsonDocument` with 8KB buffer for JSON parsing
- Increasing river count may require larger buffer
- ESP32 has ample RAM for 6 rivers + weather data

### Text Formatting

River card uses fixed-width `sprintf` formatting for column alignment:
- River name: 16 chars left-aligned
- Level: 5 chars right-aligned (includes apostrophe)
- Flow: 9 chars total (5-char number + " cfs")
- Trend: Fixed position at X=270

## License

This project is open source. Feel free to modify for your own river monitoring needs.

## Acknowledgments

- USGS for providing real-time river data
- TFT_eSPI library by Bodmer
- ArduinoJson library by Benoit Blanchon
- ESP32 Arduino Core by Espressif

## Support

For issues specific to:
- **Hardware:** Check ESP32-3248S035C datasheet and Sunton documentation
- **TFT_eSPI:** See https://github.com/Bodmer/TFT_eSPI
- **ESP32 Arduino:** See https://github.com/espressif/arduino-esp32

---

**Last Updated:** November 2024
**Tested With:**
- Arduino CLI 0.35.x
- ESP32 Arduino Core 3.3.3-3.3.4
- TFT_eSPI 2.5.x
- ArduinoJson 6.x
