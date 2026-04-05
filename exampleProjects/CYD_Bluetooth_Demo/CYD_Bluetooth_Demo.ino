/*
  CYD_Bluetooth_Demo.ino
  Minimal BLE demo for CYD 2.4R USBC (ESP32)
  - Advertises a BLE service and characteristic
  - Allows connection from Windows/iOS/Android BLE apps
  - Touchscreen GUI shows BLE status and allows sending a test message
  - Serial monitor shows debug info
  - GUI remains responsive even if BLE fails
*/

#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <BLEDevice.h>

// Nordic UART Service (NUS) UUIDs
#define SERVICE_UUID        "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E" // Write
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E" // Notify

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

// GUI button
struct Btn { int x, y, w, h; const char* label; };
Btn btnSend = {20, 200, 200, 40, "Send Test Msg"};

// BLE globals
BLEServer* pServer = nullptr;
BLECharacteristic* pCharacteristicRX = nullptr;
BLECharacteristic* pCharacteristicTX = nullptr;
bool deviceConnected = false;
bool oldDeviceConnected = false;

void drawBtn(const Btn& b, uint16_t color) {
  tft.fillRoundRect(b.x, b.y, b.w, b.h, 8, color);
  tft.drawRoundRect(b.x, b.y, b.w, b.h, 8, TFT_BLACK);
  tft.setTextColor(TFT_BLACK, color);
  tft.drawCentreString(b.label, b.x + b.w/2, b.y + 12, 2);
}

void drawUI(const char* msg = nullptr) {
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawCentreString("CYD BLE Demo", SCREEN_WIDTH/2, 10, 2);
  tft.drawCentreString(deviceConnected ? "BLE Connected" : "BLE Advertising", SCREEN_WIDTH/2, 40, 2);
  drawBtn(btnSend, TFT_CYAN);
  if (msg) tft.drawCentreString(msg, SCREEN_WIDTH/2, 270, 2);
}

class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("BLE device connected");
    drawUI();
  }
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("BLE device disconnected");
    drawUI();
  }
};

class MyCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string rxValue = pCharacteristic->getValue();
    if (rxValue.length() > 0) {
      Serial.print("Received over BLE: ");
      for (int i = 0; i < rxValue.length(); i++) Serial.print(rxValue[i]);
      Serial.println();
      // Optionally, show on GUI or echo back
    }
  }
};

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
  drawUI();
  // BLE setup (robust, with service UUID in advertising and debug output)
  BLEDevice::init("CYD-BLE");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  // RX Characteristic (Write from client)
  pCharacteristicRX = pService->createCharacteristic(
    CHARACTERISTIC_UUID_RX,
    BLECharacteristic::PROPERTY_WRITE
  );
  pCharacteristicRX->setCallbacks(new MyCallbacks());
  // TX Characteristic (Notify to client)
  pCharacteristicTX = pService->createCharacteristic(
    CHARACTERISTIC_UUID_TX,
    BLECharacteristic::PROPERTY_NOTIFY
  );
  pService->start();
  // Add service UUID to advertising for better discoverability
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iOS connection issues
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("BLE UART (NUS) advertising started");
  Serial.print("Device name: ");
  Serial.println("CYD-BLE");
  Serial.print("Service UUID: ");
  Serial.println(SERVICE_UUID);
  Serial.print("RX Characteristic UUID: ");
  Serial.println(CHARACTERISTIC_UUID_RX);
  Serial.print("TX Characteristic UUID: ");
  Serial.println(CHARACTERISTIC_UUID_TX);
}

unsigned long lastHeartbeat = 0;
const unsigned long heartbeatInterval = 1000; // ms

void loop() {
  // Touch handling
  if (touchscreen.tirqTouched() && touchscreen.touched()) {
    TS_Point p = touchscreen.getPoint();
    int touchY = map(p.y, 320, 3800, 1, 320);
    int touchX = map(p.x, 240, 3700, 240, 1);
    if (inBtn(btnSend, touchX, touchY)) {
      if (deviceConnected && pCharacteristicTX) {
        pCharacteristicTX->setValue("Test message from CYD");
        pCharacteristicTX->notify();
        drawUI("Sent test message");
        Serial.println("Sent test message over BLE");
      } else {
        drawUI("Not connected");
      }
      delay(500);
      drawUI();
    }
    delay(200); // Debounce
  }
  // BLE connection state change
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // Give the bluetooth stack the chance to get things ready
    pServer->startAdvertising(); // restart advertising
    Serial.println("Restart advertising");
    oldDeviceConnected = deviceConnected;
  }
  if (deviceConnected && !oldDeviceConnected) {
    Serial.println("Device connected (state change)");
    oldDeviceConnected = deviceConnected;
  }
  // Heartbeat notification
  if (deviceConnected && pCharacteristicTX) {
    unsigned long now = millis();
    if (now - lastHeartbeat > heartbeatInterval) {
      String hb = "HB:" + String(now/1000);
      pCharacteristicTX->setValue(hb.c_str());
      pCharacteristicTX->notify();
      Serial.print("Sent heartbeat: ");
      Serial.println(hb);
      lastHeartbeat = now;
    }
  }
}

bool inBtn(const Btn& b, int x, int y) {
  return (x >= b.x && x <= b.x + b.w && y >= b.y && y <= b.y + b.h);
}
