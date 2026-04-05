/*
  CYD_SD_Card_Demo.ino
  Demonstrates SD card initialization, file writing, reading, listing, and deleting on CYD hardware.
*/


#include <SPI.h>
#include <SD.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

// Display and touch
TFT_eSPI tft = TFT_eSPI();
#define XPT2046_IRQ   36
#define XPT2046_MOSI  13
#define XPT2046_MISO  12
#define XPT2046_CLK   14
#define XPT2046_CS    33
SPIClass touchscreenSPI = SPIClass(HSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 320
#define ROTATION      0

#define SD_CS 5 // Change to your SD card CS pin if needed
const char* logDir = "/cydsd";
const int MAX_LOG_FILES = 32;
String logFiles[MAX_LOG_FILES];
int logFileCount = 0;
bool sdCardPresent = false;

// Button bounds (x, y, w, h)
struct Btn { int x, y, w, h; const char* label; };

Btn btnWrite = {20, 50, 200, 40, "Write Log"};
Btn btnList  = {20, 100, 200, 40, "List Logs"};
Btn btnRead  = {20, 150, 200, 40, "Read Last Log"};
Btn btnDelete= {20, 200, 200, 40, "Delete All"};
Btn btnBack  = {70, 270, 100, 35, "Back"};

enum Screen { MAIN, LOGVIEW };
Screen currentScreen = MAIN;
String lastLogContent = "";

void drawBtn(const Btn& b, uint16_t color) {
  tft.fillRoundRect(b.x, b.y, b.w, b.h, 8, color);
  tft.drawRoundRect(b.x, b.y, b.w, b.h, 8, TFT_BLACK);
  tft.setTextColor(TFT_BLACK, color);
  tft.drawCentreString(b.label, b.x + b.w/2, b.y + 12, 2);
}

void drawUI(const char* msg = nullptr) {
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawCentreString("CYD SD Card Demo", SCREEN_WIDTH/2, 10, 2);
  tft.drawCentreString(sdCardPresent ? "SD OK" : "No SD", SCREEN_WIDTH/2, 30, 2);
  drawBtn(btnWrite, TFT_GREEN);
  drawBtn(btnList, TFT_CYAN);
  drawBtn(btnRead, TFT_YELLOW);
  drawBtn(btnDelete, TFT_RED);
  if (msg) tft.drawCentreString(msg, SCREEN_WIDTH/2, 240, 2);
}

void drawLogView() {
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawCentreString("Last Log", SCREEN_WIDTH/2, 10, 2);
  // Draw log content (first 12 lines max)
  int y = 35;
  int lines = 0;
  int idx = 0;
  while (lines < 12 && idx < lastLogContent.length()) {
    int nextIdx = lastLogContent.indexOf('\n', idx);
    if (nextIdx == -1) nextIdx = lastLogContent.length();
    String line = lastLogContent.substring(idx, nextIdx);
    tft.drawString(line, 5, y, 1);
    y += 18;
    lines++;
    idx = nextIdx + 1;
  }
  drawBtn(btnBack, TFT_LIGHTGREY);
}

void setup() {
  Serial.begin(115200);
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  tft.init();
  tft.setRotation(ROTATION);
  tft.invertDisplay(true);
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  initSDCard();
  drawUI();
}

void loop() {
  if (touchscreen.tirqTouched() && touchscreen.touched()) {
    TS_Point p = touchscreen.getPoint();
    int touchY = map(p.y, 320, 3800, 1, 320);
    int touchX = map(p.x, 240, 3700, 240, 1);
    if (currentScreen == MAIN) {
      if (inBtn(btnWrite, touchX, touchY)) {
        writeDemoLog();
        drawUI("Log written");
        delay(500);
        drawUI();
      } else if (inBtn(btnList, touchX, touchY)) {
        listLogFiles();
        showLogList();
        delay(1000);
        drawUI();
      } else if (inBtn(btnRead, touchX, touchY)) {
        readLastLog();
        drawLogView();
        currentScreen = LOGVIEW;
      } else if (inBtn(btnDelete, touchX, touchY)) {
        clearAllLogs();
        drawUI("All logs deleted");
        delay(500);
        drawUI();
      }
    } else if (currentScreen == LOGVIEW) {
      if (inBtn(btnBack, touchX, touchY)) {
        drawUI();
        currentScreen = MAIN;
      }
    }
    delay(200); // Debounce
  }
}

bool inBtn(const Btn& b, int x, int y) {
  return (x >= b.x && x <= b.x + b.w && y >= b.y && y <= b.y + b.h);
}

void showLogList() {
  tft.fillRect(0, 230, SCREEN_WIDTH, 80, TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  if (logFileCount == 0) {
    tft.drawCentreString("No logs found", SCREEN_WIDTH/2, 240, 2);
    return;
  }
  int y = 240;
  for (int i = max(0, logFileCount-3); i < logFileCount; i++) {
    tft.drawString(logFiles[i], 10, y, 1);
    y += 16;
  }
}

void initSDCard() {
  if (SD.begin(SD_CS)) {
    sdCardPresent = true;
    if (!SD.exists(logDir)) {
      SD.mkdir(logDir);
    }
  } else {
    sdCardPresent = false;
  }
}

String generateLogFileName() {
  int num = 1;
  char filename[32];
  while (num < 1000) {
    sprintf(filename, "%s/log_%03d.txt", logDir, num);
    if (!SD.exists(filename)) break;
    num++;
  }
  return String(filename);
}

void writeDemoLog() {
  if (!sdCardPresent) return;
  String fname = generateLogFileName();
  File logFile = SD.open(fname, FILE_WRITE);
  if (logFile) {
    logFile.println("=== CYD SD Card Demo Log ===");
    logFile.println("This is a test log entry.");
    logFile.println("You can add more data here.");
    logFile.close();
  }
}

void listLogFiles() {
  logFileCount = 0;
  if (!sdCardPresent) return;
  File dir = SD.open(logDir);
  if (!dir) return;
  while (logFileCount < MAX_LOG_FILES) {
    File entry = dir.openNextFile();
    if (!entry) break;
    String name = entry.name();
    if (name.endsWith(".txt")) {
      logFiles[logFileCount] = String(logDir) + "/" + name;
      logFileCount++;
    }
    entry.close();
  }
  dir.close();
}


void readLastLog() {
  lastLogContent = "";
  listLogFiles();
  if (logFileCount == 0) {
    lastLogContent = "No logs found.";
    return;
  }
  File f = SD.open(logFiles[logFileCount - 1]);
  if (f) {
    while (f.available() && lastLogContent.length() < 1024) {
      lastLogContent += (char)f.read();
    }
    f.close();
  } else {
    lastLogContent = "Failed to open log.";
  }
}

void clearAllLogs() {
  if (!sdCardPresent) return;
  for (int i = 0; i < logFileCount; i++) {
    SD.remove(logFiles[i]);
  }
  logFileCount = 0;
}
