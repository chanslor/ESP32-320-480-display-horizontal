/*
 * ESP32 River Levels Display for ESP32-3248S035C (HORIZONTAL)
 * Fetches USGS river data from API and displays on 3.5" ST7796 TFT
 *
 * Hardware: ESP32-3248S035C (Sunton/DIYmall)
 * Display: 480x320 pixels (landscape), ST7796 driver, SPI interface
 * MCU: ESP32-WROOM-32 (WiFi + Bluetooth)
 *
 * Shows 6 rivers in a vertical list layout (landscape mode)
 * - Locust Fork, Town Creek, South Sauty, Little River, Short Creek, Mulberry Fork
 *
 * Features:
 * - Auto-refresh every 5 minutes
 * - Elapsed time updates every 60 seconds
 * - Color-coded status indicators
 * - Real-time water levels, flow rates, and trends
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include "secrets.h"  // WiFi credentials

// API endpoint
const char* API_URL = "https://docker-blue-sound-1751.fly.dev/api/river-levels";

// Refresh intervals
const unsigned long REFRESH_INTERVAL = 5 * 60 * 1000; // 5 minutes in milliseconds (API call)
const unsigned long DISPLAY_UPDATE_INTERVAL = 60 * 1000; // 60 seconds (elapsed time display)
unsigned long lastUpdate = 0;           // When we last called the API
unsigned long lastDisplayUpdate = 0;    // When we last updated the elapsed time display

// TFT display
TFT_eSPI tft = TFT_eSPI();

// Color definitions
#define COLOR_BG       0x0000  // Black
#define COLOR_HEADER   0xFFFF  // White
#define COLOR_BORDER   0x7BEF  // Light gray
#define COLOR_GOOD     0x07E0  // Green (in range)
#define COLOR_LOW      0xF800  // Red (below minimum)
#define COLOR_WIND     0xFD20  // Orange (wind alert)
#define COLOR_COLD     0x1E9F  // Light blue (cold alert)
#define COLOR_TEXT     0xFFFF  // White

// Display dimensions for ESP32-3248S035C in LANDSCAPE
#define SCREEN_WIDTH  480
#define SCREEN_HEIGHT 320

// Layout constants for horizontal list (6 rivers)
#define HEADER_HEIGHT 30
#define RIVER_CARD_HEIGHT 46  // 320 - 30 header - 5 padding = 285 / 6 = ~47 per river

// River data structure
struct RiverData {
  String name;
  String site_id;
  String flow;
  String trend;
  float stage_ft;
  float qpf_today;
  float qpf_tomorrow;
  float temp_f;
  float wind_mph;
  String wind_dir;
  bool in_range;
  String timestamp;
};

RiverData rivers[6]; // Locust Fork, Town Creek, South Sauty, Little River, Short Creek, Mulberry Fork

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\n=== ESP32-3248S035C River Levels Display (HORIZONTAL) ===");

  // Initialize display
  tft.init();
  tft.setRotation(1); // Landscape mode (480x320)
  tft.fillScreen(COLOR_BG);

  // Show startup screen
  showStartupScreen();

  // Connect to WiFi
  connectWiFi();

  // Initial data fetch
  fetchRiverData();
  displayRivers();

  lastUpdate = millis();
  lastDisplayUpdate = millis();
}

void loop() {
  // Check if it's time to refresh API data (every 5 minutes)
  if (millis() - lastUpdate >= REFRESH_INTERVAL) {
    Serial.println("Refreshing river data...");
    fetchRiverData();
    displayRivers();
    lastUpdate = millis();
    lastDisplayUpdate = millis(); // Reset display timer after full refresh
  }

  // Check if it's time to update the elapsed time display (every 60 seconds)
  else if (millis() - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
    Serial.println("Updating elapsed time display...");
    updateElapsedTimeDisplay();
    lastDisplayUpdate = millis();
  }

  delay(1000); // Check every second
}

void showStartupScreen() {
  tft.fillScreen(COLOR_BG);
  tft.setTextColor(COLOR_HEADER);
  tft.setFreeFont(&FreeSansBold18pt7b);  // Use smooth built-in font
  tft.setCursor(100, 120);
  tft.println("River Levels");

  tft.setFreeFont(&FreeSans12pt7b);
  tft.setCursor(80, 180);
  tft.println("Connecting to WiFi...");
  tft.setTextFont(1);  // Reset to default font
}

void connectWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  Serial.print("Password length: ");
  Serial.println(strlen(WIFI_PASSWORD));

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    Serial.print(WiFi.status());
    Serial.print(" ");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // Show success on display
    tft.fillRect(0, 160, SCREEN_WIDTH, 40, COLOR_BG);
    tft.setTextColor(COLOR_GOOD);
    tft.setFreeFont(&FreeSans12pt7b);
    tft.setCursor(130, 190);
    tft.println("WiFi Connected!");
    tft.setTextFont(1);  // Reset to default
    delay(2000);
  } else {
    Serial.println("\nWiFi connection failed!");
    tft.fillRect(0, 160, SCREEN_WIDTH, 40, COLOR_BG);
    tft.setTextColor(COLOR_LOW);
    tft.setTextFont(1);
    tft.setCursor(80, 180);
    tft.println("WiFi Failed - Check credentials");
    while(1) delay(1000); // Halt
  }
}

void fetchRiverData() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected!");
    return;
  }

  HTTPClient http;
  http.begin(API_URL);

  Serial.print("Fetching data from API... ");
  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();
    Serial.println("Success!");

    // Parse JSON
    DynamicJsonDocument doc(8192); // Allocate enough for all rivers
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
      Serial.print("JSON parse error: ");
      Serial.println(error.c_str());
      http.end();
      return;
    }

    // Extract river data
    JsonArray sites = doc["sites"];
    int riverIndex = 0;

    for (JsonObject site : sites) {
      String site_id = site["site_id"].as<String>();

      // Map site IDs to our river array (6 rivers now!)
      int index = -1;
      if (site_id == "02455000") index = 0; // Locust Fork
      else if (site_id == "03572900") index = 1; // Town Creek
      else if (site_id == "03572690") index = 2; // South Sauty
      else if (site_id == "02399200") index = 3; // Little River
      else if (site_id == "03574500") index = 4; // Short Creek
      else if (site_id == "02450000") index = 5; // Mulberry Fork

      if (index >= 0 && index < 6) {
        rivers[index].name = site["name"].as<String>();
        rivers[index].site_id = site_id;
        rivers[index].flow = site["flow"].as<String>();
        rivers[index].trend = site["trend"].as<String>();
        rivers[index].stage_ft = site["stage_ft"] | 0.0;
        rivers[index].in_range = site["in_range"] | false;
        rivers[index].timestamp = site["timestamp"].as<String>();

        // QPF data
        if (site["qpf"].containsKey("today")) {
          rivers[index].qpf_today = site["qpf"]["today"];
        }
        if (site["qpf"].containsKey("tomorrow")) {
          rivers[index].qpf_tomorrow = site["qpf"]["tomorrow"];
        }

        // Weather data
        if (site["weather"].containsKey("temp_f") && !site["weather"]["temp_f"].isNull()) {
          rivers[index].temp_f = site["weather"]["temp_f"];
        } else {
          rivers[index].temp_f = 0.0; // Use 0 to indicate no data
        }
        if (site["weather"].containsKey("wind_mph")) {
          rivers[index].wind_mph = site["weather"]["wind_mph"];
        }
        if (site["weather"].containsKey("wind_dir")) {
          rivers[index].wind_dir = site["weather"]["wind_dir"].as<String>();
        }

        Serial.print(rivers[index].name);
        Serial.print(": ");
        Serial.print(rivers[index].flow);
        Serial.print(" ");
        Serial.println(rivers[index].trend);
      }
    }

  } else {
    Serial.print("HTTP error: ");
    Serial.println(httpCode);
  }

  http.end();
}

void displayRivers() {
  tft.fillScreen(COLOR_BG);

  // Draw header
  drawHeader();

  // Draw rivers in vertical list (card deck style) - 6 rivers now!
  int yPos = HEADER_HEIGHT + 5;
  int cardHeight = RIVER_CARD_HEIGHT;

  for (int i = 0; i < 6; i++) {
    drawRiverCard(i, yPos);
    yPos += cardHeight;
  }
}

void drawHeader() {
  tft.fillRect(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, COLOR_BG);

  // Draw top border
  tft.drawLine(5, 5, SCREEN_WIDTH - 5, 5, COLOR_BORDER);

  // Title
  tft.setTextColor(TFT_SKYBLUE);
  tft.setTextFont(2);  // Built-in font 2 (smooth)
  tft.setCursor(10, 12);
  tft.print("RIVER LEVELS");

  // Update time (right side)
  drawElapsedTime();

  // Draw bottom border
  tft.drawLine(5, HEADER_HEIGHT - 2, SCREEN_WIDTH - 5, HEADER_HEIGHT - 2, COLOR_BORDER);
}

void drawElapsedTime() {
  // Update time (right side)
  tft.setTextColor(TFT_CYAN);
  tft.setTextFont(2);  // Match header font
  tft.setCursor(330, 12);  // Adjusted for 480 width
  tft.print("Updated: ");

  // Calculate time since last update
  unsigned long secsSinceUpdate = (millis() - lastUpdate) / 1000;
  if (secsSinceUpdate < 60) {
    tft.print(secsSinceUpdate);
    tft.print("s");
  } else {
    unsigned long minsSinceUpdate = secsSinceUpdate / 60;
    tft.print(minsSinceUpdate);
    tft.print("m");
  }
  tft.print(" ago");
}

void updateElapsedTimeDisplay() {
  // Clear just the elapsed time area (right portion of header)
  tft.fillRect(330, 7, SCREEN_WIDTH - 335, 20, COLOR_BG);

  // Redraw elapsed time
  drawElapsedTime();
}

void drawRiverCard(int riverIndex, int y) {
  RiverData &river = rivers[riverIndex];

  // Status color based on in_range
  uint16_t statusColor;
  if (river.in_range) {
    statusColor = COLOR_GOOD;  // Green
  } else if (river.flow.toInt() < 100) {
    statusColor = COLOR_LOW;   // Red (too low)
  } else {
    statusColor = 0xFFE0;      // Yellow (warning)
  }

  // River name (abbreviated)
  String displayName = river.name;
  if (riverIndex == 0) displayName = "Locust Fork";
  else if (riverIndex == 1) displayName = "Town Creek";
  else if (riverIndex == 2) displayName = "South Sauty";
  else if (riverIndex == 3) displayName = "LRC";
  else if (riverIndex == 4) displayName = "Short Creek";
  else if (riverIndex == 5) displayName = "Mulberry Fork";

  // Line 1: Status dot + River name + Lvl + Flow + Trend
  int xPos = 10;

  // Draw status dot
  tft.fillCircle(xPos + 5, y + 8, 5, statusColor);
  xPos += 18;

  // Line 1: River name + Lvl + Flow + Trend (all using sprintf for alignment)
  tft.setTextFont(2);
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(xPos, y + 3);

  char line1[60];
  char lvlStr[10];

  // Format level string - need to ensure apostrophe aligns
  if (river.stage_ft > 0.0) {
    // 4-wide, 1 decimal place, then apostrophe: e.g. " 1.2'"
    sprintf(lvlStr, "%4.1f'", river.stage_ft);
  } else {
    // Keep apostrophe column aligned for missing data
    strcpy(lvlStr, " ---'");
  }

  // Format flow with cfs as a complete unit (support up to 4 digits)
  char flowStr[15];
  sprintf(flowStr, "%5d cfs", river.flow.toInt());  // Right-aligned flow with cfs (5 chars for number)

  // Format entire line with fixed widths (adjusted for 480 width)
  // - river name: left-justified in 16 chars (slightly wider)
  // - level: 5 chars (includes the ')
  // - flow: 9 chars total (right-aligned number + " cfs")
  sprintf(line1, "%-16s %5s  %s",
          displayName.c_str(),   // River name (16 chars, left-aligned)
          lvlStr,                // Level (5 chars total including ')
          flowStr);              // Flow (9 chars: "    X cfs" or "   XX cfs" or "  XXX cfs" or " XXXX cfs")

  tft.print(line1);

  // Trend (fixed position, adjusted for wider screen)
  tft.setCursor(xPos + 270, y + 3);  // Moved right for 480 width
  if (river.trend.indexOf("^") >= 0 || river.trend.indexOf("rising") >= 0) {
    tft.setTextColor(TFT_CYAN);
    tft.print("^rising");
  } else if (river.trend.indexOf("v") >= 0 || river.trend.indexOf("falling") >= 0) {
    tft.setTextColor(TFT_ORANGE);
    tft.print("vfalling");
  } else {
    tft.setTextColor(COLOR_TEXT);
    tft.print("-steady");
  }

  // Draw divider line (but not for the last river)
  if (riverIndex < 5) {  // 6 rivers, so last index is 5
    tft.drawLine(5, y + 38, SCREEN_WIDTH - 5, y + 38, COLOR_BORDER);
  }
}
