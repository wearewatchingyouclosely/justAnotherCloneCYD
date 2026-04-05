/*
 * CYD 2.4R USBC - WiFi Connection Demo
 * Board: DIYTZT ESP32 2.4" TFT 240x320 with Touch
 * 
 * Demonstrates WiFi connection with status display.
 * Shows IP address on screen when connected.
 * 
 * SETUP: Create wifi_credentials.tkn file - see README.md
 */

#include <SPI.h>
#include <WiFi.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

// Brownout detector fix
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// WiFi credentials from external file (see README.md)
#include "wifi_credentials.tkn"

const char* ssid     = WIFI_SSID;
const char* password = WIFI_PASSWORD;

TFT_eSPI tft = TFT_eSPI();

// Touchscreen pins
#define XPT2046_IRQ   36
#define XPT2046_MOSI  13
#define XPT2046_MISO  12
#define XPT2046_CLK   14
#define XPT2046_CS    33

SPIClass touchscreenSPI = SPIClass(HSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

// Landscape mode
#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240
#define ROTATION      1

void setup() {
  Serial.begin(115200);
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  // Initialize touchscreen
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);

  // Initialize display
  tft.init();
  tft.setRotation(ROTATION);
  tft.invertDisplay(true);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  int centerX = SCREEN_WIDTH / 2;
  
  // Show connecting status
  tft.drawCentreString("Connecting to WiFi...", centerX, 60, 2);
  tft.drawCentreString(ssid, centerX, 90, 2);

  // Connect to WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    tft.drawString(".", 10 + (attempts * 10), 120, 2);
    attempts++;
  }

  tft.fillScreen(TFT_BLACK);
  
  if (WiFi.status() == WL_CONNECTED) {
    // Connected successfully
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawCentreString("WiFi Connected!", centerX, 60, 2);
    
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawCentreString("IP Address:", centerX, 100, 2);
    tft.drawCentreString(WiFi.localIP().toString(), centerX, 130, 4);
    
    tft.drawCentreString("RSSI: " + String(WiFi.RSSI()) + " dBm", centerX, 180, 2);
    
    Serial.println("\nConnected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    // Connection failed
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawCentreString("Connection Failed!", centerX, 100, 2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawCentreString("Check credentials", centerX, 140, 2);
  }
}

void loop() {
  // Add your connected logic here
  
  // Example: check if still connected
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 5000) {
    lastCheck = millis();
    if (WiFi.status() != WL_CONNECTED) {
      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.drawCentreString("WiFi Disconnected!", SCREEN_WIDTH/2, SCREEN_HEIGHT/2, 2);
    }
  }
}
