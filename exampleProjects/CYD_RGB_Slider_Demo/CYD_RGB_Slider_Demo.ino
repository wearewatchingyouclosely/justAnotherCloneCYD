/*
 * CYD 2.4R USBC - RGB LED Slider Demo
 * Board: DIYTZT ESP32 2.4" TFT 240x320 with Touch
 * 
 * Control RGB LED with 4 sliders: Red, Green, Blue, and Intensity
 * Press BOOT button to see indicator
 */

#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

TFT_eSPI tft = TFT_eSPI();

// Touchscreen pins
#define XPT2046_IRQ   36
#define XPT2046_MOSI  13
#define XPT2046_MISO  12
#define XPT2046_CLK   14
#define XPT2046_CS    33

// BOOT button on GPIO 0
#define BOOT_BTN 0

// RGB LED pins (active LOW: 0=ON, 1=OFF)
#define LED_RED   4
#define LED_GREEN 17
#define LED_BLUE  16

// PWM settings
#define PWM_FREQ  5000
#define PWM_RES   8

SPIClass touchscreenSPI = SPIClass(HSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

// Landscape mode
#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240
#define ROTATION      1

// Slider values (0-255)
int redVal = 0;
int greenVal = 0;
int blueVal = 0;
int intensityVal = 255;  // Start at full intensity

// Slider layout
#define SLIDER_X      70
#define SLIDER_W      200
#define SLIDER_H      30
#define SLIDER_GAP    45

int sliderY[] = {40, 85, 130, 175};  // Y positions for R, G, B, Intensity
uint16_t sliderColors[] = {TFT_RED, TFT_GREEN, TFT_BLUE, TFT_WHITE};
const char* sliderLabels[] = {"R", "G", "B", "I"};

// Update the RGB LED
void updateLED() {
  // Apply intensity scaling
  float scale = intensityVal / 255.0;
  
  // Invert for active LOW (255 = off, 0 = full on)
  int r = 255 - (int)(redVal * scale);
  int g = 255 - (int)(greenVal * scale);
  int b = 255 - (int)(blueVal * scale);
  
  ledcWrite(LED_RED, r);
  ledcWrite(LED_GREEN, g);
  ledcWrite(LED_BLUE, b);
}

// Draw a single slider
void drawSlider(int index) {
  int y = sliderY[index];
  int val;
  
  switch(index) {
    case 0: val = redVal; break;
    case 1: val = greenVal; break;
    case 2: val = blueVal; break;
    case 3: val = intensityVal; break;
  }
  
  // Label
  tft.setTextColor(sliderColors[index], TFT_BLACK);
  tft.drawString(sliderLabels[index], 15, y + 5, 4);
  
  // Slider background
  tft.fillRect(SLIDER_X, y, SLIDER_W, SLIDER_H, TFT_DARKGREY);
  
  // Filled portion
  int fillW = map(val, 0, 255, 0, SLIDER_W);
  tft.fillRect(SLIDER_X, y, fillW, SLIDER_H, sliderColors[index]);
  
  // Border
  tft.drawRect(SLIDER_X, y, SLIDER_W, SLIDER_H, TFT_WHITE);
  
  // Value text
  tft.fillRect(SLIDER_X + SLIDER_W + 5, y, 45, SLIDER_H, TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(String(val), SLIDER_X + SLIDER_W + 10, y + 5, 2);
}

// Draw all sliders
void drawAllSliders() {
  for (int i = 0; i < 4; i++) {
    drawSlider(i);
  }
}

// Check which slider is touched and update value
int handleSliderTouch(int x, int y) {
  for (int i = 0; i < 4; i++) {
    int sy = sliderY[i];
    if (y >= sy - 5 && y <= sy + SLIDER_H + 5 &&
        x >= SLIDER_X && x <= SLIDER_X + SLIDER_W) {
      // Calculate new value
      int newVal = map(x, SLIDER_X, SLIDER_X + SLIDER_W, 0, 255);
      newVal = constrain(newVal, 0, 255);
      
      switch(i) {
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

// Show BOOT pressed indicator
void showBootPressed() {
  tft.fillRect(0, 220, SCREEN_WIDTH, 20, TFT_MAGENTA);
  tft.setTextColor(TFT_WHITE, TFT_MAGENTA);
  tft.drawCentreString("BOOT PRESSED!", SCREEN_WIDTH / 2, 222, 2);
}

// Show hint when BOOT not pressed
void showBootHint() {
  tft.fillRect(0, 220, SCREEN_WIDTH, 20, TFT_BLACK);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.drawCentreString("Press BOOT button to test", SCREEN_WIDTH / 2, 222, 2);
}

void setup() {
  Serial.begin(115200);
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  // Setup BOOT button
  pinMode(BOOT_BTN, INPUT_PULLUP);

  // Setup RGB LED with PWM
  ledcAttach(LED_RED, PWM_FREQ, PWM_RES);
  ledcAttach(LED_GREEN, PWM_FREQ, PWM_RES);
  ledcAttach(LED_BLUE, PWM_FREQ, PWM_RES);
  ledcWrite(LED_RED, 255);   // Off
  ledcWrite(LED_GREEN, 255); // Off
  ledcWrite(LED_BLUE, 255);  // Off

  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);

  tft.init();
  tft.setRotation(ROTATION);
  tft.invertDisplay(true);
  tft.fillScreen(TFT_BLACK);
  
  // Title
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString("RGB LED Control", SCREEN_WIDTH / 2, 10, 2);
  
  drawAllSliders();
  
  // Show BOOT button hint
  showBootHint();
}

void loop() {
  // Check BOOT button - show message while held
  if (digitalRead(BOOT_BTN) == LOW) {
    showBootPressed();
    while (digitalRead(BOOT_BTN) == LOW) {
      delay(10);
    }
    showBootHint();
  }

  // Check touchscreen
  if (touchscreen.tirqTouched() && touchscreen.touched()) {
    TS_Point p = touchscreen.getPoint();
    
    // Landscape calibration
    int x = map(p.y, 320, 3800, 1, 320);
    int y = map(p.x, 240, 3700, 1, 240);

    // Handle slider touch
    int sliderIdx = handleSliderTouch(x, y);
    if (sliderIdx >= 0) {
      drawSlider(sliderIdx);
      updateLED();
    }
    
    delay(30);
  }
}
