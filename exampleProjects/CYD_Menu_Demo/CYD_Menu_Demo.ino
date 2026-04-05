/*
 * CYD 2.4R USBC - Menu Demo
 * Board: DIYTZT ESP32 2.4" TFT 240x320 with Touch
 * 
 * Main menu with sub-screens:
 * - LED/Button Check: RGB sliders and BOOT button test
 * - WiFi Check: Connect and show IP
 * - Settings: Portrait/Landscape toggle
 * 
 * SETUP: Create wifi_credentials.tkn file - see README.md
 */

#include <SPI.h>
#include <WiFi.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <Preferences.h>

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// WiFi credentials from external file
#include "wifi_credentials.tkn"

const char* ssid     = WIFI_SSID;
const char* password = WIFI_PASSWORD;

TFT_eSPI tft = TFT_eSPI();
Preferences prefs;

// Touchscreen pins
#define XPT2046_IRQ   36
#define XPT2046_MOSI  13
#define XPT2046_MISO  12
#define XPT2046_CLK   14
#define XPT2046_CS    33

// BOOT button
#define BOOT_BTN 0

// RGB LED pins (active LOW)
#define LED_RED   4
#define LED_GREEN 17
#define LED_BLUE  16

#define PWM_FREQ  5000
#define PWM_RES   8

SPIClass touchscreenSPI = SPIClass(HSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

// Screen dimensions (updated on rotation change)
int SCREEN_WIDTH = 320;
int SCREEN_HEIGHT = 240;
int rotation = 1;  // 1=landscape, 0=portrait

// App states
enum AppState {
  STATE_MENU,
  STATE_LED_CHECK,
  STATE_WIFI_CHECK,
  STATE_TOUCH_TEST,
  STATE_SETTINGS
};

AppState currentState = STATE_MENU;

// Menu button structure
struct MenuButton {
  int x, y, w, h;
  const char* label;
  uint16_t color;
};

// Menu buttons (positions set in drawMenu based on orientation)
MenuButton menuButtons[4];
MenuButton backButton;
MenuButton settingsButtons[2];

// LED slider values
int redVal = 128;
int greenVal = 128;
int blueVal = 128;
int intensityVal = 255;

// Slider layout (updated for orientation)
int sliderX, sliderW, sliderH, sliderGap;
int sliderY[4];

// Touch debounce
unsigned long lastTouch = 0;
#define TOUCH_DEBOUNCE 200

// ==================== UTILITY FUNCTIONS ====================

void getTouchPoint(int &x, int &y) {
  TS_Point p = touchscreen.getPoint();
  
  if (rotation == 1) {  // Landscape
    x = map(p.y, 320, 3800, 1, 320);
    y = map(p.x, 240, 3700, 1, 240);
  } else {  // Portrait (rotation=0)
    y = map(p.y, 320, 3800, 1, 320);
    x = map(p.x, 240, 3700, 240, 1);  // X is inverted in portrait
  }
}

bool isTouched() {
  return touchscreen.tirqTouched() && touchscreen.touched();
}

bool touchDebounce() {
  if (millis() - lastTouch > TOUCH_DEBOUNCE) {
    lastTouch = millis();
    return true;
  }
  return false;
}

bool buttonPressed(MenuButton &btn, int tx, int ty) {
  return tx >= btn.x && tx <= btn.x + btn.w &&
         ty >= btn.y && ty <= btn.y + btn.h;
}

void drawButton(MenuButton &btn) {
  tft.fillRoundRect(btn.x, btn.y, btn.w, btn.h, 8, btn.color);
  tft.drawRoundRect(btn.x, btn.y, btn.w, btn.h, 8, TFT_WHITE);
  
  int textX = btn.x + btn.w / 2;
  int textY = btn.y + btn.h / 2 - 8;
  tft.setTextColor(TFT_WHITE, btn.color);
  tft.drawCentreString(btn.label, textX, textY, 2);
}

void saveSettings() {
  prefs.begin("cyd", false);
  prefs.putInt("rotation", rotation);
  prefs.end();
}

void loadSettings() {
  prefs.begin("cyd", true);
  rotation = prefs.getInt("rotation", 1);  // Default landscape
  prefs.end();
}

void applyRotation() {
  tft.setRotation(rotation);
  if (rotation == 1) {  // Landscape
    SCREEN_WIDTH = 320;
    SCREEN_HEIGHT = 240;
  } else {  // Portrait
    SCREEN_WIDTH = 240;
    SCREEN_HEIGHT = 320;
  }
}

// ==================== MENU SCREEN ====================

void setupMenuButtons() {
  int btnW = SCREEN_WIDTH - 40;
  int btnH = (rotation == 1) ? 40 : 45;
  int startY = (rotation == 1) ? 45 : 55;
  int gap = (rotation == 1) ? 48 : 60;
  
  menuButtons[0] = {20, startY, btnW, btnH, "LED / Button Check", TFT_BLUE};
  menuButtons[1] = {20, startY + gap, btnW, btnH, "WiFi Check", TFT_DARKGREEN};
  menuButtons[2] = {20, startY + gap * 2, btnW, btnH, "Touch Test", TFT_ORANGE};
  menuButtons[3] = {20, startY + gap * 3, btnW, btnH, "Settings", TFT_PURPLE};
}

void drawMenu() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString("CYD 2.4R Demo", SCREEN_WIDTH / 2, 10, 4);
  
  setupMenuButtons();
  for (int i = 0; i < 4; i++) {
    drawButton(menuButtons[i]);
  }
}

void handleMenuTouch(int x, int y) {
  for (int i = 0; i < 4; i++) {
    if (buttonPressed(menuButtons[i], x, y)) {
      switch (i) {
        case 0: currentState = STATE_LED_CHECK; drawLEDScreen(); break;
        case 1: currentState = STATE_WIFI_CHECK; drawWiFiScreen(); break;
        case 2: currentState = STATE_TOUCH_TEST; drawTouchTestScreen(); break;
        case 3: currentState = STATE_SETTINGS; drawSettingsScreen(); break;
      }
      return;
    }
  }
}

// ==================== LED/BUTTON CHECK SCREEN ====================

void setupSliderLayout() {
  if (rotation == 1) {  // Landscape
    sliderX = 50;
    sliderW = 180;
    sliderH = 25;
    sliderGap = 35;
    sliderY[0] = 50;
    sliderY[1] = 85;
    sliderY[2] = 120;
    sliderY[3] = 155;
  } else {  // Portrait
    sliderX = 30;
    sliderW = 140;
    sliderH = 30;
    sliderGap = 50;
    sliderY[0] = 60;
    sliderY[1] = 110;
    sliderY[2] = 160;
    sliderY[3] = 210;
  }
}

void updateLED() {
  float scale = intensityVal / 255.0;
  ledcWrite(LED_RED, 255 - (int)(redVal * scale));
  ledcWrite(LED_GREEN, 255 - (int)(greenVal * scale));
  ledcWrite(LED_BLUE, 255 - (int)(blueVal * scale));
}

void drawSlider(int index) {
  int y = sliderY[index];
  int val;
  uint16_t color;
  const char* label;
  
  switch (index) {
    case 0: val = redVal; color = TFT_RED; label = "R"; break;
    case 1: val = greenVal; color = TFT_GREEN; label = "G"; break;
    case 2: val = blueVal; color = TFT_BLUE; label = "B"; break;
    case 3: val = intensityVal; color = TFT_WHITE; label = "I"; break;
  }
  
  tft.setTextColor(color, TFT_BLACK);
  tft.drawString(label, 10, y + 3, 4);
  
  tft.fillRect(sliderX, y, sliderW, sliderH, TFT_DARKGREY);
  int fillW = map(val, 0, 255, 0, sliderW);
  tft.fillRect(sliderX, y, fillW, sliderH, color);
  tft.drawRect(sliderX, y, sliderW, sliderH, TFT_WHITE);
  
  tft.fillRect(sliderX + sliderW + 5, y, 40, sliderH, TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(String(val), sliderX + sliderW + 8, y + 3, 2);
}

void drawLEDScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString("LED / Button Check", SCREEN_WIDTH / 2, 10, 2);
  
  setupSliderLayout();
  for (int i = 0; i < 4; i++) {
    drawSlider(i);
  }
  
  // Back button
  int btnY = (rotation == 1) ? 200 : 280;
  backButton = {10, btnY, 80, 30, "< Back", TFT_DARKGREY};
  drawButton(backButton);
  
  // Boot hint
  int hintY = (rotation == 1) ? 205 : 285;
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.drawString("Press BOOT to test", SCREEN_WIDTH / 2 - 30, hintY, 1);
  
  updateLED();
}

int handleSliderTouch(int x, int y) {
  for (int i = 0; i < 4; i++) {
    int sy = sliderY[i];
    if (y >= sy - 5 && y <= sy + sliderH + 5 &&
        x >= sliderX && x <= sliderX + sliderW) {
      int newVal = map(x, sliderX, sliderX + sliderW, 0, 255);
      newVal = constrain(newVal, 0, 255);
      
      switch (i) {
        case 0: redVal = newVal; break;
        case 1: greenVal = newVal; break;
        case 2: blueVal = newVal; break;
        case 3: intensityVal = newVal; break;
      }
      return i;
    }
  }
  return -1;
}

void handleLEDScreen() {
  // Check BOOT button
  static bool bootWasPressed = false;
  if (digitalRead(BOOT_BTN) == LOW) {
    if (!bootWasPressed) {
      int hintY = (rotation == 1) ? 200 : 275;
      tft.fillRect(SCREEN_WIDTH / 2 - 50, hintY, 120, 20, TFT_MAGENTA);
      tft.setTextColor(TFT_WHITE, TFT_MAGENTA);
      tft.drawString("BOOT PRESSED!", SCREEN_WIDTH / 2 - 45, hintY + 3, 2);
      bootWasPressed = true;
    }
  } else if (bootWasPressed) {
    int hintY = (rotation == 1) ? 200 : 275;
    tft.fillRect(SCREEN_WIDTH / 2 - 50, hintY, 120, 20, TFT_BLACK);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString("Press BOOT to test", SCREEN_WIDTH / 2 - 30, hintY + 5, 1);
    bootWasPressed = false;
  }
  
  // Check touch
  if (isTouched()) {
    int x, y;
    getTouchPoint(x, y);
    
    // Back button
    if (buttonPressed(backButton, x, y) && touchDebounce()) {
      // Turn off LED
      ledcWrite(LED_RED, 255);
      ledcWrite(LED_GREEN, 255);
      ledcWrite(LED_BLUE, 255);
      currentState = STATE_MENU;
      drawMenu();
      return;
    }
    
    // Sliders
    int idx = handleSliderTouch(x, y);
    if (idx >= 0) {
      drawSlider(idx);
      updateLED();
    }
  }
}

// ==================== WIFI CHECK SCREEN ====================

void drawWiFiScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString("WiFi Check", SCREEN_WIDTH / 2, 10, 2);
  
  int centerX = SCREEN_WIDTH / 2;
  int startY = (rotation == 1) ? 50 : 70;
  
  tft.drawCentreString("Connecting to:", centerX, startY, 2);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawCentreString(ssid, centerX, startY + 25, 2);
  
  // Connect
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  int dotY = startY + 55;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(".", 50 + (attempts * 10), dotY, 2);
    attempts++;
  }
  
  tft.fillRect(0, dotY - 5, SCREEN_WIDTH, 30, TFT_BLACK);
  
  int resultY = startY + 60;
  if (WiFi.status() == WL_CONNECTED) {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawCentreString("Connected!", centerX, resultY, 2);
    
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawCentreString("IP Address:", centerX, resultY + 30, 2);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawCentreString(WiFi.localIP().toString(), centerX, resultY + 55, 4);
    
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawCentreString("RSSI: " + String(WiFi.RSSI()) + " dBm", centerX, resultY + 90, 2);
  } else {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawCentreString("Connection Failed!", centerX, resultY, 2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawCentreString("Check credentials", centerX, resultY + 30, 2);
  }
  
  // Back button
  int btnY = (rotation == 1) ? 200 : 280;
  backButton = {10, btnY, 80, 30, "< Back", TFT_DARKGREY};
  drawButton(backButton);
}

void handleWiFiScreen() {
  if (isTouched() && touchDebounce()) {
    int x, y;
    getTouchPoint(x, y);
    
    if (buttonPressed(backButton, x, y)) {
      WiFi.disconnect();
      currentState = STATE_MENU;
      drawMenu();
    }
  }
}

// ==================== TOUCH TEST SCREEN ====================

void drawTouchTestScreen() {
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  
  int centerX = SCREEN_WIDTH / 2;
  tft.drawCentreString("Touch Test", centerX, 10, 2);
  tft.drawCentreString("Touch anywhere to draw", centerX, 30, 1);
  
  // Back button
  int btnY = (rotation == 1) ? 200 : 280;
  backButton = {10, btnY, 80, 30, "< Back", TFT_DARKGREY};
  drawButton(backButton);
  
  // Clear button
  MenuButton clearBtn = {SCREEN_WIDTH - 90, btnY, 80, 30, "Clear", TFT_RED};
  tft.fillRoundRect(clearBtn.x, clearBtn.y, clearBtn.w, clearBtn.h, 8, clearBtn.color);
  tft.drawRoundRect(clearBtn.x, clearBtn.y, clearBtn.w, clearBtn.h, 8, TFT_WHITE);
  tft.setTextColor(TFT_WHITE, clearBtn.color);
  tft.drawCentreString("Clear", clearBtn.x + clearBtn.w/2, clearBtn.y + 7, 2);
}

void handleTouchTestScreen() {
  if (isTouched()) {
    int x, y;
    getTouchPoint(x, y);
    
    // Back button check
    if (buttonPressed(backButton, x, y) && touchDebounce()) {
      currentState = STATE_MENU;
      drawMenu();
      return;
    }
    
    // Clear button check
    int btnY = (rotation == 1) ? 200 : 280;
    if (x >= SCREEN_WIDTH - 90 && x <= SCREEN_WIDTH - 10 &&
        y >= btnY && y <= btnY + 30 && touchDebounce()) {
      drawTouchTestScreen();
      return;
    }
    
    // Draw box at touch point (avoid button areas)
    if (y < btnY - 5) {
      tft.fillRect(x - 5, y - 5, 10, 10, TFT_BLACK);
    }
  }
}

// ==================== SETTINGS SCREEN ====================

void drawSettingsScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString("Settings", SCREEN_WIDTH / 2, 10, 2);
  
  int btnW = SCREEN_WIDTH - 60;
  int btnH = 45;
  int startY = (rotation == 1) ? 60 : 80;
  int gap = 60;
  
  // Highlight current orientation
  uint16_t landColor = (rotation == 1) ? TFT_BLUE : TFT_DARKGREY;
  uint16_t portColor = (rotation == 0) ? TFT_BLUE : TFT_DARKGREY;
  
  settingsButtons[0] = {30, startY, btnW, btnH, "Landscape", landColor};
  settingsButtons[1] = {30, startY + gap, btnW, btnH, "Portrait", portColor};
  
  drawButton(settingsButtons[0]);
  drawButton(settingsButtons[1]);
  
  // Current setting indicator
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  int indicatorY = (rotation == 1) ? startY + 17 : startY + gap + 17;
  tft.drawString("*", 15, indicatorY, 2);
  
  // Back button
  int btnY = (rotation == 1) ? 200 : 280;
  backButton = {10, btnY, 80, 30, "< Back", TFT_DARKGREY};
  drawButton(backButton);
}

void handleSettingsScreen() {
  if (isTouched() && touchDebounce()) {
    int x, y;
    getTouchPoint(x, y);
    
    // Back button
    if (buttonPressed(backButton, x, y)) {
      currentState = STATE_MENU;
      drawMenu();
      return;
    }
    
    // Landscape button
    if (buttonPressed(settingsButtons[0], x, y) && rotation != 1) {
      rotation = 1;
      saveSettings();
      applyRotation();
      drawSettingsScreen();
      return;
    }
    
    // Portrait button
    if (buttonPressed(settingsButtons[1], x, y) && rotation != 0) {
      rotation = 0;
      saveSettings();
      applyRotation();
      drawSettingsScreen();
      return;
    }
  }
}

// ==================== MAIN ====================

void setup() {
  Serial.begin(115200);
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  // Load saved settings
  loadSettings();

  // BOOT button
  pinMode(BOOT_BTN, INPUT_PULLUP);

  // RGB LED
  ledcAttach(LED_RED, PWM_FREQ, PWM_RES);
  ledcAttach(LED_GREEN, PWM_FREQ, PWM_RES);
  ledcAttach(LED_BLUE, PWM_FREQ, PWM_RES);
  ledcWrite(LED_RED, 255);
  ledcWrite(LED_GREEN, 255);
  ledcWrite(LED_BLUE, 255);

  // Touchscreen
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);

  // Display
  tft.init();
  applyRotation();
  tft.invertDisplay(true);
  
  drawMenu();
}

void loop() {
  switch (currentState) {
    case STATE_MENU:
      if (isTouched() && touchDebounce()) {
        int x, y;
        getTouchPoint(x, y);
        handleMenuTouch(x, y);
      }
      break;
      
    case STATE_LED_CHECK:
      handleLEDScreen();
      break;
      
    case STATE_WIFI_CHECK:
      handleWiFiScreen();
      break;
      
    case STATE_TOUCH_TEST:
      handleTouchTestScreen();
      break;
      
    case STATE_SETTINGS:
      handleSettingsScreen();
      break;
  }
}
