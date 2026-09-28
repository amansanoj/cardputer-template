#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include "config.h"

enum ControlIcon {
  ICON_NONE = 0,
  ICON_PLAY,
  ICON_PAUSE,
  ICON_PLAYPAUSE,
  ICON_PREV,
  ICON_NEXT,
  ICON_REWIND,
  ICON_FORWARD,
  ICON_UP,
  ICON_DOWN,
  ICON_SELECT
};

struct FooterButton {
  const char* keyHint; // e.g. "Spc", "Enter", "A", "Tab", "S"
  const char* label;   // e.g. "Run", "Back", "Menu"
  ControlIcon icon;    // optional icon
};

class CardputerShell {
public:
  CardputerShell();
  void begin();

  // Lifecycle & Frame Management
  void beginFrame();
  void flush();

  // Dirty frame tracking (prevents constant 65KB SPI frame hammering)
  bool isDirty() const { return needsRedraw; }
  void requestRedraw() { needsRedraw = true; }
  void clearDirty() { needsRedraw = false; }

  // Header & Footer Standard Components
  void renderHeader(const char* title, const char* clockStr = nullptr, bool wifiConnected = false, int8_t wifiRssi = -100);
  void renderFooter(const FooterButton* buttons, size_t buttonCount);
  void renderFooterHint(const char* hintText, uint16_t color = COLOR_MUTED);

  // Status Overlays
  void renderStatus(const char* title, const char* subtitle, uint16_t accentColor = COLOR_PRIMARY);

  // Canvas & Geometry Access
  GFXcanvas16& getCanvas() { return canvas; }
  Adafruit_ST7789& getTft() { return tft; }

  int16_t getContentX() const { return 0; }
  int16_t getContentY() const { return HEADER_HEIGHT; }
  int16_t getContentW() const { return SCREEN_WIDTH; }
  int16_t getContentH() const { return CONTENT_HEIGHT; }

  // Power & Battery
  void getBatteryInfo(uint8_t &pct, bool &isCharging);

private:
  Adafruit_ST7789 tft;
  GFXcanvas16 canvas;
  bool needsRedraw;

  void drawControlIcon(int16_t x, int16_t y, ControlIcon icon, uint16_t color);
};
