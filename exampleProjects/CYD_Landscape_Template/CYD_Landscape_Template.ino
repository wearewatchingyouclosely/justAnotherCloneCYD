/*
 * CYD 2.4R USBC - Landscape Mode Template
 * Board: DIYTZT ESP32 2.4" TFT 240x320 with Touch
 * 
 * This is a clean template for landscape orientation (320x240)
 * Touch coordinates are calibrated for this specific board.
 */

#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>


// Display object
TFT_eSPI tft = TFT_eSPI();

// Touchscreen pins (directly on board)
#define XPT2046_IRQ   36
#define XPT2046_MOSI  13
#define XPT2046_MISO  12
#define XPT2046_CLK   14
#define XPT2046_CS    33

// Touchscreen SPI (uses HSPI)
SPIClass touchscreenSPI = SPIClass(HSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

// Landscape mode settings
#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240
#define ROTATION      1

// Touch coordinates
int touchX, touchY, touchZ;

void setup() {
  Serial.begin(115200);
  
  // Disable brownout detector (prevents random resets on power dips)
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  // Initialize touchscreen on HSPI
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);

  // Initialize display
  tft.init();
  tft.setRotation(ROTATION);
  tft.invertDisplay(true);  // Required for CYD 2.4R!
  
  // Clear screen
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  
  // Welcome message
  int centerX = SCREEN_WIDTH / 2;
  int centerY = SCREEN_HEIGHT / 2;
  tft.drawCentreString("CYD 2.4R Landscape", centerX, centerY - 20, 2);
  tft.drawCentreString("Touch anywhere", centerX, centerY, 2);
}

void loop() {
  if (touchscreen.tirqTouched() && touchscreen.touched()) {
    TS_Point p = touchscreen.getPoint();
    
    // Landscape mode touch calibration (X and Y swapped)
    touchX = map(p.y, 320, 3800, 1, 320);
    touchY = map(p.x, 240, 3700, 1, 240);
    touchZ = p.z;

    // Print to serial
    Serial.printf("X=%d Y=%d Z=%d\n", touchX, touchY, touchZ);
    
    // Show on screen
    tft.fillScreen(TFT_GREEN);
    tft.setTextColor(TFT_BLACK, TFT_WHITE);
    
    int centerX = SCREEN_WIDTH / 2;
    int centerY = SCREEN_HEIGHT / 2;
    
    tft.drawCentreString("X = " + String(touchX), centerX, centerY - 20, 2);
    tft.drawCentreString("Y = " + String(touchY), centerX, centerY, 2);
    
    // Draw touch point
    tft.fillRect(touchX - 5, touchY - 5, 10, 10, TFT_BLACK);

    delay(100);
  }
}
