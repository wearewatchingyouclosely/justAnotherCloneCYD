#include <TinyGPSPlus.h>

#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

// Display and touchscreen objects
TFT_eSPI tft = TFT_eSPI();
#define XPT2046_IRQ   36
#define XPT2046_MOSI  13
#define XPT2046_MISO  12
#define XPT2046_CLK   14
#define XPT2046_CS    33
SPIClass touchscreenSPI = SPIClass(HSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240
#define ROTATION      1

// Define GPS serial pins for CYD (IO21 = RX, IO22 = TX)
#define GPS_RX 21 // ESP32 RX (connect to GPS TX)
#define GPS_TX 22 // ESP32 TX (connect to GPS RX)

HardwareSerial GPSSerial(1); // Use UART1
TinyGPSPlus gps;

void setup() {
	Serial.begin(115200);
	GPSSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
	Serial.println("CYD GPS Demo: Waiting for GPS data...");

	// Disable brownout detector (prevents random resets on power dips)
	WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

	// Initialize touchscreen on HSPI
	touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
	touchscreen.begin(touchscreenSPI);

	// Initialize display
	tft.init();
	tft.setRotation(ROTATION);
	tft.invertDisplay(true);  // Required for CYD 2.4R!
	tft.fillScreen(TFT_WHITE);
	tft.setTextColor(TFT_BLACK, TFT_WHITE);

	int centerX = SCREEN_WIDTH / 2;
	int centerY = SCREEN_HEIGHT / 2;
	tft.drawCentreString("CYD 2.4R GPS Demo", centerX, centerY - 20, 2);
	tft.drawCentreString("Waiting for GPS...", centerX, centerY, 2);
}

void loop() {
	static unsigned long lastFixTime = 0;
	static unsigned long lastPrint = 0;
	static bool lastFixValid = false;
	static double lastLat = 0, lastLng = 0;
	static int lastDate = 0, lastMonth = 0, lastYear = 0, lastHour = 0, lastMin = 0, lastSec = 0;
	bool newFix = false;

	while (GPSSerial.available() > 0) {
		if (gps.encode(GPSSerial.read())) {
			if (gps.location.isValid()) {
				lastFixTime = millis();
				lastFixValid = true;
				lastLat = gps.location.lat();
				lastLng = gps.location.lng();
				lastDate = gps.date.day();
				lastMonth = gps.date.month();
				lastYear = gps.date.year();
				lastHour = gps.time.hour();
				lastMin = gps.time.minute();
				lastSec = gps.time.second();
				newFix = true;
				// Serial output
				Serial.print("Lat: "); Serial.print(lastLat, 6);
				Serial.print(", Lng: "); Serial.print(lastLng, 6);
				Serial.print(", Date: ");
				if (gps.date.isValid()) {
					Serial.print(lastMonth); Serial.print("/");
					Serial.print(lastDate); Serial.print("/");
					Serial.print(lastYear);
				} else {
					Serial.print("??/??/????");
				}
				Serial.print(", Time: ");
				if (gps.time.isValid()) {
					if (lastHour < 10) Serial.print('0');
					Serial.print(lastHour); Serial.print(":");
					if (lastMin < 10) Serial.print('0');
					Serial.print(lastMin); Serial.print(":");
					if (lastSec < 10) Serial.print('0');
					Serial.print(lastSec);
				} else {
					Serial.print("??:??:??");
				}
				Serial.println();
			}
		}
	}

	// Print/update GUI every second
	if (millis() - lastPrint > 1000) {
		lastPrint = millis();
		unsigned long sinceFix = (millis() - lastFixTime) / 1000;

		tft.fillScreen(TFT_WHITE);
		tft.setTextColor(TFT_BLACK, TFT_WHITE);
		int centerX = SCREEN_WIDTH / 2;
		int y = 30;
		tft.drawCentreString("CYD 2.4R GPS Demo", centerX, y, 2);
		y += 30;
		if (lastFixValid) {
			tft.setTextColor(TFT_DARKGREEN, TFT_WHITE);
			tft.drawCentreString("GPS Fix Acquired", centerX, y, 2);
			y += 30;
			tft.setTextColor(TFT_BLACK, TFT_WHITE);
			tft.drawCentreString("Lat: " + String(lastLat, 6), centerX, y, 2);
			y += 25;
			tft.drawCentreString("Lng: " + String(lastLng, 6), centerX, y, 2);
			y += 25;
			char dateStr[32];
			snprintf(dateStr, sizeof(dateStr), "Date: %02d/%02d/%04d", lastMonth, lastDate, lastYear);
			tft.drawCentreString(dateStr, centerX, y, 2);
			y += 25;
			char timeStr[32];
			snprintf(timeStr, sizeof(timeStr), "Time: %02d:%02d:%02d", lastHour, lastMin, lastSec);
			tft.drawCentreString(timeStr, centerX, y, 2);
			y += 25;
			tft.setTextColor(TFT_BLUE, TFT_WHITE);
			tft.drawCentreString("Seconds since last fix: " + String(sinceFix), centerX, y, 2);
		} else {
			tft.setTextColor(TFT_RED, TFT_WHITE);
			tft.drawCentreString("Waiting for GPS fix...", centerX, y, 2);
			y += 30;
			tft.setTextColor(TFT_BLACK, TFT_WHITE);
			tft.drawCentreString("Seconds since last fix: " + String(sinceFix), centerX, y, 2);
		}
	}
}
