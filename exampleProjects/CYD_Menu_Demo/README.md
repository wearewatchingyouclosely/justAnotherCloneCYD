# CYD Menu Demo

A comprehensive demo combining all CYD 2.4R features with a touch-based menu system.

## Features

- **Main Menu** - Touch-based navigation
- **LED/Button Check** - RGB sliders with intensity control, BOOT button test
- **WiFi Check** - Connect and display IP address, signal strength
- **Settings** - Toggle between Portrait and Landscape (persists across reboots)

## Setup

### WiFi Credentials

Create a file named `wifi_credentials.tkn` in this folder with:

```cpp
// WiFi Credentials - DO NOT COMMIT TO GIT
#define WIFI_SSID     "YourNetworkName"
#define WIFI_PASSWORD "YourPassword"
```

## Usage

1. Upload the sketch
2. Navigate using touch:
   - Tap menu items to enter screens
   - Use sliders by dragging
   - Tap "< Back" to return to menu
3. Orientation setting is saved automatically

## Notes

- Settings are stored in ESP32's flash memory using Preferences library
- LED is turned off when leaving the LED check screen
- WiFi is disconnected when leaving WiFi check screen
