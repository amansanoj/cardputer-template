#pragma once
#include <stdint.h>

// Target selection (default to Cardputer-Adv unless TARGET_WOKWI_SIMULATOR is set)
#if !defined(TARGET_WOKWI_SIMULATOR) && !defined(TARGET_M5_CARDPUTER_ADV)
#define TARGET_M5_CARDPUTER_ADV
#endif

#if defined(TARGET_M5_CARDPUTER_ADV)
// =====================================================================
// M5Stack Cardputer-Adv (SKU: K132-Adv, Stamp-S3A core) Pinout
// =====================================================================
#define TFT_CS 37
#define TFT_DC 34   // RS (Command/Data) is G34 on Cardputer-Adv
#define TFT_RST 33  // RST is G33
#define TFT_MOSI 35 // DAT (SPI MOSI) is G35 on Cardputer-Adv
#define TFT_SCLK 36 // SCK (SPI Clock) is G36 on Cardputer-Adv
#define TFT_BL 38   // DISP_BL & RGB LED PWR_EN switch (Set HIGH)
#define BAT_ADC_PIN 10 // Battery voltage sensing ADC (ratio 2.0)
#define BOARD_NAME "Cardputer-Adv"

// MicroSD Card SPI Configuration
#define HAS_SD_CARD   1
#define SD_SPI_SCK    40
#define SD_SPI_MISO   39
#define SD_SPI_MOSI   14
#define SD_SPI_CS     12
#define SD_SPI_CS_ALT 5
#else // TARGET_WOKWI_SIMULATOR
// =====================================================================
// Wokwi simulation pinout (ESP32-S3 hardware SPI default pins)
// =====================================================================
#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST 8
#define TFT_MOSI 11
#define TFT_SCLK 12
#define TFT_BL -1 // No backlight pin needed in Wokwi
#define BAT_ADC_PIN -1
#define BOARD_NAME "Wokwi Sim"

#define HAS_SD_CARD   0
#define SD_SPI_SCK    -1
#define SD_SPI_MISO   -1
#define SD_SPI_MOSI   -1
#define SD_SPI_CS     -1
#define SD_SPI_CS_ALT -1
#endif

// =====================================================================
// DISPLAY & SHELL METRICS
// =====================================================================
#define SCREEN_WIDTH        240
#define SCREEN_HEIGHT       135
#define HEADER_HEIGHT       16
#define FOOTER_HEIGHT       17
#define CONTENT_START_Y     HEADER_HEIGHT
#define CONTENT_HEIGHT      (SCREEN_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT) // 102px
#define PADDING_LEFT        8
#define PADDING_RIGHT       8

// =====================================================================
// 60-30-10 COLOR PALETTE (RGB565)
// =====================================================================
// 60% Dominant (Canvas, Structural Containers, Body Text)
#define COLOR_BG            0x0000 // Deep Canvas Background (#050505 / ST7789 pitch black)
#define COLOR_HEADER_BG     0x0861 // Header dark strip (#0d0d0d)
#define COLOR_FOOTER_BG     0x0861 // Footer dark strip (#0d0d0d)
#define COLOR_DIVIDER       0x18E3 // Subtle divider border (#1a1a1a)
#define COLOR_CARD_BG       0x0861 // Card surface (#0d0d0d)
#define COLOR_PILL_BG       0x10A2 // Button pill background (#151515)
#define COLOR_PILL_BORDER   0x3186 // Button pill border (#333333)
#define COLOR_TEXT          0xE73C // Main text (#e6e6e6)
#define COLOR_MUTED         0x8410 // Muted labels & timestamps (#808080)
#define COLOR_BAR_BG        0x2124 // Track / Progress background (#252525)

// 30% Structural Hierarchy & Brand (Primary: #afbdd9)
#define COLOR_PRIMARY       0xADFB // App Name / Titles / Active Playhead (#afbdd9)
#define COLOR_PRIMARY_LIGHT 0xDF1D // Primary light shade (#dce2ef)
#define COLOR_PRIMARY_DIM   0x320D // Primary dark border/bg (#314368)

// 10% High-Impact Accents (Secondary: #f0a133, Accent: #df9a9e)
#define COLOR_SECONDARY     0xF506 // UI Hints & Keyboard Shortcuts (#f0a133)
#define COLOR_SECONDARY_DIM 0x59C0 // Secondary dark shade (#5f3a07)
#define COLOR_ACCENT        0xDCD3 // Alerts & Badges (#df9a9e)
#define COLOR_ACCENT_BG     0x2061 // Accent container background (#270c0e)
#define COLOR_ACCENT_LIGHT  0xE596 // Accent light shade (#e7b1b4)

// Status Colors
#define COLOR_SUCCESS       0x2E65 // Green (#2cd85e)
#define COLOR_WARNING       0xFD20 // Amber (#ffa500)
#define COLOR_ERROR         0xD986 // Red (#d9383a)

// =====================================================================
// WI-FI & SETUP CONFIGURATION
// =====================================================================
#define AP_SSID             "cardputer-app-setup"
#define WOKWI_DEFAULT_SSID  "Wokwi-GUEST"
#define WOKWI_DEFAULT_PASS  ""

// Default Timezone offset (UTC+4 = 14400s)
#define DEFAULT_GMT_OFFSET_SEC 14400
#define DEFAULT_DAYLIGHT_OFFSET_SEC 0
