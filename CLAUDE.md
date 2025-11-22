# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ESP32-based river levels display system for the ESP32-3248S035C development board (Sunton/DIYmall). Fetches USGS river data from a custom API and displays real-time water levels, flow rates, and weather data on a 480x320 TFT display in landscape orientation.

**Hardware:** ESP32-3248S035C with ST7796 3.5" TFT display (480x320 pixels, SPI interface)

**Display Layout:** Horizontal/landscape orientation showing 6 rivers in a vertical card-based list with status indicators, levels, flow rates, and trends.

## Development Commands

This is an Arduino sketch project. Build and upload using Arduino IDE or Arduino CLI:

```bash
# Compile with build directory (recommended - keeps artifacts organized)
arduino-cli compile --build-path ./build --fqbn esp32:esp32:esp32 .

# Compile without specifying build directory (uses temp directory)
arduino-cli compile --fqbn esp32:esp32:esp32 ESP32-320-480-display-horizontal.ino

# Upload to board (specify build directory if used during compile)
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32 --input-dir ./build .

# Upload without build directory
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32 ESP32-320-480-display-horizontal.ino

# Monitor serial output (baudrate 115200)
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200

# Find connected device port
ls /dev/ttyUSB* /dev/ttyACM*
```

For Arduino IDE: Open `.ino` file, select board "ESP32 Dev Module", and use standard Upload (Ctrl+U) and Serial Monitor (Ctrl+Shift+M).

**Build Artifacts:** When using `--build-path ./build`, compiled binaries (`.bin`, `.elf`, `.map`) and object files are stored in the `build/` directory. This directory is gitignored.

## Architecture

### Single-File Design
The entire application is contained in `ESP32-320-480-display-horizontal.ino` - a monolithic Arduino sketch with no separate headers or modules.

### Core Flow
1. **Startup:** Initialize TFT display (480x320 landscape), connect to WiFi, fetch initial data
2. **Main Loop:**
   - Full API refresh every 5 minutes (`REFRESH_INTERVAL`)
   - Elapsed time display update every 60 seconds (`DISPLAY_UPDATE_INTERVAL`)
   - Non-blocking timing using `millis()`

### Data Model
`RiverData` struct holds all information for each river site:
- USGS data: flow (cfs), stage/level (ft), trend (rising/falling/steady), in_range status
- Weather: temperature, wind speed/direction
- QPF: precipitation forecast for today/tomorrow
- Metadata: site_id, name, timestamp

Fixed array of 6 rivers mapped by USGS site IDs:
- `02455000` - Locust Fork (index 0)
- `03572900` - Town Creek (index 1)
- `03572690` - South Sauty (index 2)
- `02399200` - Little River (index 3)
- `03574500` - Short Creek (index 4)
- `02450000` - Mulberry Fork (index 5)

### Display System
**Layout Constants:**
- `SCREEN_WIDTH`: 480px, `SCREEN_HEIGHT`: 320px
- `HEADER_HEIGHT`: 30px (title + elapsed time)
- `RIVER_CARD_HEIGHT`: 46px per river (6 cards fit vertically)

**Color Scheme:**
- Green (`COLOR_GOOD`): River in optimal range
- Red (`COLOR_LOW`): Flow below minimum threshold
- Orange (`COLOR_WIND`): Wind alert (not currently used)
- Light blue (`COLOR_COLD`): Cold alert (not currently used)

**Rendering:**
- `displayRivers()`: Full screen redraw (header + all 6 river cards)
- `updateElapsedTimeDisplay()`: Partial update (only elapsed time area in header)
- Each river card shows: status dot, name, level (ft), flow (cfs), trend arrow

### API Integration
**Endpoint:** `https://docker-blue-sound-1751.fly.dev/api/river-levels`

**Response Format:** JSON with `sites` array containing objects with:
- `site_id`, `name`, `flow`, `trend`, `stage_ft`, `in_range`, `timestamp`
- `qpf` object: `today`, `tomorrow` (precipitation)
- `weather` object: `temp_f`, `wind_mph`, `wind_dir`

Uses `ArduinoJson` library with 8KB dynamic buffer for parsing.

### WiFi Configuration
Credentials loaded from `secrets.h` in the project root directory. Create this file from the template:

```bash
cp secrets.h.example secrets.h
# Edit secrets.h with actual WiFi credentials
```

The `secrets.h` file must define:
- `WIFI_SSID` (const char*)
- `WIFI_PASSWORD` (const char*)

Connection timeout: 30 attempts × 500ms = 15 seconds max.

**Note:** The `secrets.h` file is gitignored and should never be committed to version control.

## Display Coordinate System

The TFT uses landscape orientation (`tft.setRotation(1)`):
- Origin (0,0) at top-left
- X-axis: 0-479 (horizontal)
- Y-axis: 0-319 (vertical)

River cards are stacked vertically starting at Y=35 (after header + padding), each 46px tall.

## Key Implementation Details

### Text Formatting & Alignment
River card Line 1 uses fixed-width `sprintf` formatting to align columns:
- River name: 16 chars left-aligned
- Level: 5 chars right-aligned (includes decimal and apostrophe, e.g., " 1.2'")
- Flow: 9 chars total (5-char right-aligned number + " cfs")
- Trend: Fixed position at X=270 (independent of other fields)

### Timer Management
Two independent timers prevent unnecessary full redraws:
- `lastUpdate`: Tracks 5-minute API refresh cycle
- `lastDisplayUpdate`: Tracks 60-second elapsed time updates

Both reset when full refresh occurs to keep them synchronized.

### Library Dependencies
- `WiFi.h`: ESP32 WiFi connectivity
- `HTTPClient.h`: HTTP GET requests
- `ArduinoJson.h`: JSON parsing (requires v6+)
- `TFT_eSPI.h`: ST7796 display driver (requires proper `User_Setup.h` configuration for ESP32-3248S035C)

## Key Functions Reference

Located in `ESP32-320-480-display-horizontal.ino`:

- `setup()`: Initialize display, WiFi, fetch initial data (lines 73-96)
- `loop()`: Check timers for API refresh (5min) or display update (60s) (lines 98-116)
- `fetchRiverData()`: HTTP GET to API, parse JSON, populate rivers array (lines 176-263)
- `displayRivers()`: Full screen redraw - header + all 6 river cards (lines 265-279)
- `updateElapsedTimeDisplay()`: Partial update - only elapsed time in header (lines 320-326)
- `drawRiverCard(int, int)`: Render single river card with status, name, level, flow, trend (lines 328-406)
- `drawHeader()`: Render top bar with title and elapsed time (lines 281-298)
- `connectWiFi()`: WiFi connection with 15-second timeout (lines 131-174)

## Important Constraints

1. **Display Driver Configuration:** TFT_eSPI requires hardware-specific pin mappings in `User_Setup.h` (in Arduino libraries folder at `$HOME/Arduino/libraries/TFT_eSPI/User_Setup.h`). The ESP32-3248S035C uses:
   - Driver: ST7796 (NOT ILI9341 default)
   - Required pin configuration:
     ```cpp
     #define TFT_MISO 12   // SPI MISO
     #define TFT_MOSI 13   // SPI MOSI
     #define TFT_SCLK 14   // SPI Clock
     #define TFT_CS   15   // Chip select
     #define TFT_DC    2   // Data/Command
     #define TFT_RST  -1   // Reset (connected to EN)
     #define TFT_BL   27   // Backlight
     #define SPI_FREQUENCY 80000000  // 80MHz
     ```
   - This is the most common source of "black screen" issues - verify configuration matches hardware
   - See README section "Critical TFT_eSPI Configuration" for complete setup

2. **River Order:** The 6-element array index must match the site_id mapping in `fetchRiverData()` (lines 210-217). Adding/removing rivers requires updating both:
   - Array size declaration: `RiverData rivers[6]` (line 71)
   - Index mapping logic in the site_id conditional chain (lines 212-217)
   - Display name mapping in `drawRiverCard()` (lines 342-348)
   - Display constants if changing count (RIVER_CARD_HEIGHT calculation may need adjustment)

3. **Memory:** Uses `DynamicJsonDocument` with 8KB buffer (line 193). Increasing river count or adding more data fields may require larger buffer to avoid parse errors.

4. **WiFi Credentials:** Project requires `secrets.h` file in project root. Use `secrets.h.example` as template. This file is gitignored for security.
