# CYD 2.4R USBC Full Pinout Reference

## Board Overview
```
┌─────────────────────────────────────────┐
│        CYD 2.4R USBC ESP32              │
│     240×320 TFT + Touch Display         │
│                                         │
│  ┌─────────────────────────────────┐    │
│  │                                 │    │
│  │                                 │    │
│  │         2.4" TFT LCD            │    │
│  │         ILI9341 Driver          │    │
│  │         XPT2046 Touch           │    │
│  │                                 │    │
│  │                                 │    │
│  └─────────────────────────────────┘    │
│                                         │
│            [USB-C Port]                 │
└─────────────────────────────────────────┘
```

---

## TFT Display Pins (ILI9341)

| Function | GPIO | Notes |
|----------|------|-------|
| MISO | 12 | SPI Master In, Slave Out |
| MOSI | 13 | SPI Master Out, Slave In |
| SCLK | 14 | SPI Clock |
| CS | 15 | Chip Select |
| DC | 2 | Data/Command select |
| RST | -1 | Connected to ESP32 RST |
| BL | 27 | Backlight (HIGH = ON) |

---

## SD Card Pins (SPI)

| Function | GPIO | Notes |
|----------|------|-------|
| SD_CS | 5 | SD Card Chip Select (CS) |
| MOSI | 13 | Shared with display/touch |
| MISO | 12 | Shared with display/touch |
| SCLK | 14 | Shared with display/touch |

---

## WiFi/Bluetooth

| Function | GPIO | Notes |
|----------|------|-------|
| WiFi/BT | (internal) | ESP32 built-in radio |

---

## RGB LED (onboard)

| Function | GPIO | Notes |
|----------|------|-------|
| LED_R | 4 | Red (active LOW) |
| LED_G | 17 | Green (active LOW) |
| LED_B | 16 | Blue (active LOW) |

---

## Other Pins

| Function | GPIO | Notes |
|----------|------|-------|
| KIT_LED_BUILTIN | 17 | Used in Marauder, same as LED_G |

---

## Peripheral Pin Summary Table

| Peripheral | Pin Name | GPIO | Notes |
|------------|----------|------|-------|
| Display    | TFT_MISO | 12   | SPI MISO |
| Display    | TFT_MOSI | 13   | SPI MOSI |
| Display    | TFT_SCLK | 14   | SPI SCLK |
| Display    | TFT_CS   | 15   | Chip Select |
| Display    | TFT_DC   | 2    | Data/Command |
| Display    | TFT_RST  | -1   | Reset (to ESP32 RST) |
| Display    | TFT_BL   | 27   | Backlight |
| Touch      | XPT2046_CS | 33 | Touch CS |
| Touch      | XPT2046_IRQ | 36 | Touch IRQ (input only) |
| Touch      | XPT2046_MOSI | 13 | Shared with TFT_MOSI |
| Touch      | XPT2046_MISO | 12 | Shared with TFT_MISO |
| Touch      | XPT2046_CLK  | 14 | Shared with TFT_SCLK |
| SD Card    | SD_CS    | 5    | SD Card CS |
| SD Card    | MOSI     | 13   | Shared SPI |
| SD Card    | MISO     | 12   | Shared SPI |
| SD Card    | SCLK     | 14   | Shared SPI |
| LED        | LED_R    | 4    | Red (active LOW) |
| LED        | LED_G    | 17   | Green (active LOW) |
| LED        | LED_B    | 16   | Blue (active LOW) |
| WiFi/BT    | -        | (internal) | ESP32 built-in |

---

---

## Touchscreen Pins (XPT2046)

| Function | GPIO | Notes |
|----------|------|-------|
| T_IRQ | 36 | Touch interrupt (input only) |
| T_DIN | 13 | Touch data in (shared with TFT MOSI) |
| T_OUT | 12 | Touch data out (shared with TFT MISO) |
| T_CLK | 14 | Touch clock (shared with TFT SCLK) |
| T_CS | 33 | Touch chip select |

---

## SPI Bus Sharing

The display and touchscreen share the SPI bus:
- **MOSI:** GPIO 13
- **MISO:** GPIO 12  
- **SCLK:** GPIO 14
- **Port:** HSPI

They use separate chip select lines:
- **Display CS:** GPIO 15
- **Touch CS:** GPIO 33

---

## Pin Defines for Your Sketch

### Touchscreen (put in your sketch)
```cpp
#define XPT2046_IRQ   36   // T_IRQ
#define XPT2046_MOSI  13   // T_DIN
#define XPT2046_MISO  12   // T_OUT
#define XPT2046_CLK   14   // T_CLK
#define XPT2046_CS    33   // T_CS
```

### Display (configured in TFT_eSPI User_Setup.h)
```cpp
#define TFT_MISO  12
#define TFT_MOSI  13
#define TFT_SCLK  14
#define TFT_CS    15
#define TFT_DC     2
#define TFT_RST   -1
#define TFT_BL    27
```

---

## Input-Only GPIO Note

**GPIO 36 (T_IRQ)** is an input-only pin on ESP32. This is fine for the touch interrupt function, but don't try to use it for output.

---

## ESP32-WROOM Module Info

The board uses ESP32-WROOM-32 module:
- **Flash:** Typically 4MB
- **PSRAM:** None (standard WROOM)
- **WiFi:** 802.11 b/g/n
- **Bluetooth:** v4.2 BR/EDR and BLE

---

## Power Notes

- USB-C connector provides 5V power
- Brown-out detector may trigger on power dips (disable if unstable)
- Backlight draws additional current when ON

---

## GPIO Summary Table

| GPIO | Used For | Direction |
|------|----------|-----------|
| 2 | TFT_DC | Output |
| 12 | SPI MISO | I/O |
| 13 | SPI MOSI | Output |
| 14 | SPI SCLK | Output |
| 15 | TFT_CS | Output |
| 27 | TFT_BL | Output |
| 33 | TOUCH_CS | Output |
| 36 | TOUCH_IRQ | Input |
| 4  | LED_R | Output |
| 16 | LED_B | Output |
| 17 | LED_G / KIT_LED_BUILTIN | Output |
| 5  | SD_CS | Output |
