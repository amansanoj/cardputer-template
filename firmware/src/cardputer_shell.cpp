#include "cardputer_shell.h"
#include <WiFi.h>
#include <time.h>

// IMPORTANT: Pass (TFT_CS, TFT_DC, TFT_RST) to use ESP32 HARDWARE SPI!
// If MOSI/SCLK are passed here, Adafruit GFX falls back to slow software bit-banging!
CardputerShell::CardputerShell()
  : tft(TFT_CS, TFT_DC, TFT_RST),
    canvas(SCREEN_WIDTH, SCREEN_HEIGHT),
    needsRedraw(true) {}

void CardputerShell::begin() {
  if (TFT_BL >= 0) {
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH); // Turn on display backlight
  }

  // 1. Initialize hardware SPI bus with custom pins (Cardputer-Adv or Wokwi)
  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);

  // 2. Initialize ST7789 hardware display controller (135x240 native panel)
  tft.init(SCREEN_HEIGHT, SCREEN_WIDTH);

  // 3. Re-bind SPI pins on ESP32-S3 (tft.init internally resets pins to defaults)
  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);

  tft.setRotation(1); // Landscape: 240x135
  tft.fillScreen(COLOR_BG);

  canvas.setTextWrap(false);
  canvas.fillScreen(COLOR_BG);
  tft.drawRGBBitmap(0, 0, canvas.getBuffer(), SCREEN_WIDTH, SCREEN_HEIGHT);
  needsRedraw = true;
}

void CardputerShell::beginFrame() {
  // Clear full double-buffer canvas for zero-flicker rendering
  canvas.fillScreen(COLOR_BG);
}

void CardputerShell::flush() {
  tft.drawRGBBitmap(0, 0, canvas.getBuffer(), SCREEN_WIDTH, SCREEN_HEIGHT);
}

void CardputerShell::getBatteryInfo(uint8_t &pct, bool &isCharging) {
#if defined(BAT_ADC_PIN) && (BAT_ADC_PIN >= 0)
  static uint32_t smoothedMv = 0;
  uint32_t rawMv = analogReadMilliVolts(BAT_ADC_PIN) * 2;
  if (smoothedMv == 0) smoothedMv = rawMv;
  else smoothedMv = (smoothedMv * 7 + rawMv) / 8;

  // Active USB charging rail typically pulls voltage above 4250mV
  isCharging = (smoothedMv >= 4250);

  // Realistic piecewise LiPo discharge curve
  if (smoothedMv >= 4180) {
    pct = 100;
  } else if (smoothedMv >= 4000) {
    pct = 85 + (uint8_t)(((smoothedMv - 4000) * 15) / 180);
  } else if (smoothedMv >= 3850) {
    pct = 60 + (uint8_t)(((smoothedMv - 3850) * 25) / 150);
  } else if (smoothedMv >= 3700) {
    pct = 30 + (uint8_t)(((smoothedMv - 3700) * 30) / 150);
  } else if (smoothedMv >= 3500) {
    pct = 10 + (uint8_t)(((smoothedMv - 3500) * 20) / 200);
  } else if (smoothedMv >= 3300) {
    pct = (uint8_t)(((smoothedMv - 3300) * 10) / 200);
  } else {
    pct = 0;
  }
  if (pct > 100) pct = 100;
#else
  // Wokwi simulation / USB DevKit fallback
  isCharging = true;
  pct = 100;
#endif
}

void CardputerShell::drawControlIcon(int16_t x, int16_t y, ControlIcon icon, uint16_t color) {
  switch (icon) {
    case ICON_PREV: // |◀
      canvas.fillRect(x, y, 2, 7, color);
      canvas.fillTriangle(x + 6, y, x + 6, y + 6, x + 2, y + 3, color);
      break;
    case ICON_REWIND: // ◀◀
      canvas.fillTriangle(x + 3, y, x + 3, y + 6, x, y + 3, color);
      canvas.fillTriangle(x + 7, y, x + 7, y + 6, x + 4, y + 3, color);
      break;
    case ICON_PLAY: // ▶
      canvas.fillTriangle(x, y, x, y + 6, x + 6, y + 3, color);
      break;
    case ICON_PAUSE: // ❚❚
      canvas.fillRect(x, y, 2, 7, color);
      canvas.fillRect(x + 4, y, 2, 7, color);
      break;
    case ICON_PLAYPAUSE: // ▶||
      canvas.fillTriangle(x, y, x, y + 6, x + 4, y + 3, color);
      canvas.fillRect(x + 6, y, 1, 7, color);
      break;
    case ICON_FORWARD: // ▶▶
      canvas.fillTriangle(x, y, x, y + 6, x + 3, y + 3, color);
      canvas.fillTriangle(x + 4, y, x + 4, y + 6, x + 7, y + 3, color);
      break;
    case ICON_NEXT: // ▶|
      canvas.fillTriangle(x, y, x, y + 6, x + 4, y + 3, color);
      canvas.fillRect(x + 5, y, 2, 7, color);
      break;
    case ICON_UP: // ▲
      canvas.fillTriangle(x + 3, y, x, y + 6, x + 6, y + 6, color);
      break;
    case ICON_DOWN: // ▼
      canvas.fillTriangle(x, y, x + 6, y, x + 3, y + 6, color);
      break;
    case ICON_SELECT: // ↵
      canvas.drawFastHLine(x + 2, y + 4, 4, color);
      canvas.drawFastVLine(x + 5, y + 1, 4, color);
      canvas.drawPixel(x + 1, y + 3, color);
      canvas.drawPixel(x, y + 4, color);
      canvas.drawPixel(x + 1, y + 5, color);
      break;
    default:
      break;
  }
}

void CardputerShell::renderHeader(const char* title, const char* clockStr, bool wifiConnected, int8_t wifiRssi) {
  // 1. Header dark bar
  canvas.fillRect(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, COLOR_HEADER_BG);

  // 2. Resolve Clock Time:
  char displayClock[12] = "--:--";
  if (clockStr && strlen(clockStr) > 0 && strcmp(clockStr, "--:--") != 0) {
    strncpy(displayClock, clockStr, sizeof(displayClock) - 1);
  } else {
    time_t nowSec = time(nullptr);
    struct tm tmInfo;
    if (localtime_r(&nowSec, &tmInfo) && tmInfo.tm_year >= (2024 - 1900)) {
      snprintf(displayClock, sizeof(displayClock), "%02d:%02d", tmInfo.tm_hour, tmInfo.tm_min);
    } else {
      uint32_t totalSec = millis() / 1000;
      snprintf(displayClock, sizeof(displayClock), "%02u:%02u", (totalSec / 60) % 100, totalSec % 60);
    }
  }

  canvas.setTextSize(1);
  canvas.setTextColor(COLOR_TEXT);
  canvas.setCursor(6, 4);
  canvas.print(displayClock);

  // 3. Screen Title in center: COLOR_PRIMARY (#afbdd9)
  if (title && strlen(title) > 0) {
    int16_t titleW = strlen(title) * 6;
    int16_t titleX = (SCREEN_WIDTH - titleW) / 2;
    canvas.setTextColor(COLOR_PRIMARY);
    canvas.setCursor(titleX, 4);
    canvas.print(title);
  }

  // 4. Status indicators on right: Wi-Fi RSSI + Battery Capsule
  uint8_t batPct = 100;
  bool isCharging = false;
  getBatteryInfo(batPct, isCharging);

  const int16_t rightX = SCREEN_WIDTH - 6;
  const int16_t batW = 15;
  const int16_t batH = 8;
  const int16_t bx = rightX - batW;
  const int16_t by = 4;

  canvas.drawRoundRect(bx, by, batW - 2, batH, 2, COLOR_MUTED);
  canvas.fillRect(bx + batW - 2, by + 2, 2, 4, COLOR_MUTED);

  if (isCharging) {
    canvas.drawLine(bx + 7, by + 1, bx + 5, by + 4, COLOR_SECONDARY);
    canvas.drawLine(bx + 5, by + 4, bx + 8, by + 4, COLOR_SECONDARY);
    canvas.drawLine(bx + 8, by + 4, bx + 6, by + 7, COLOR_SECONDARY);
  } else {
    int16_t fillW = (batPct * 9) / 100;
    if (fillW > 0) {
      uint16_t fillColor = (batPct <= 15) ? COLOR_ACCENT : COLOR_PRIMARY;
      canvas.fillRect(bx + 2, by + 2, fillW, 4, fillColor);
    }
  }

  char pctStr[8];
  snprintf(pctStr, sizeof(pctStr), "%u%%", batPct);
  size_t pctLen = strlen(pctStr);
  int16_t pctX = bx - 3 - (pctLen * 6);
  canvas.setTextSize(1);
  canvas.setTextColor(COLOR_TEXT);
  canvas.setCursor(pctX, by);
  canvas.print(pctStr);

  int16_t wx = pctX - 6 - 8;
  uint16_t b1 = (wifiConnected && wifiRssi > -90) ? COLOR_SECONDARY : COLOR_BAR_BG;
  uint16_t b2 = (wifiConnected && wifiRssi > -75) ? COLOR_SECONDARY : COLOR_BAR_BG;
  uint16_t b3 = (wifiConnected && wifiRssi > -65) ? COLOR_SECONDARY : COLOR_BAR_BG;

  canvas.fillRect(wx, by + 5, 2, 3, b1);
  canvas.fillRect(wx + 3, by + 3, 2, 5, b2);
  canvas.fillRect(wx + 6, by + 1, 2, 7, b3);

  canvas.drawFastHLine(0, HEADER_HEIGHT - 1, SCREEN_WIDTH, COLOR_DIVIDER);
}

void CardputerShell::renderFooter(const FooterButton* buttons, size_t buttonCount) {
  const int16_t fy = SCREEN_HEIGHT - FOOTER_HEIGHT; // 135 - 17 = 118

  canvas.drawFastHLine(0, fy - 1, SCREEN_WIDTH, COLOR_DIVIDER);
  canvas.fillRect(0, fy, SCREEN_WIDTH, FOOTER_HEIGHT, COLOR_FOOTER_BG);

  if (!buttons || buttonCount == 0) return;

  int16_t slotW = SCREEN_WIDTH / buttonCount;
  for (size_t i = 0; i < buttonCount; i++) {
    const FooterButton &btn = buttons[i];
    int16_t slotX = i * slotW;
    int16_t pillW = slotW - 6;
    int16_t pillX = slotX + 3;
    int16_t pillY = fy + 2;
    int16_t pillH = FOOTER_HEIGHT - 4; // 13px

    canvas.fillRoundRect(pillX, pillY, pillW, pillH, 3, COLOR_PILL_BG);
    canvas.drawRoundRect(pillX, pillY, pillW, pillH, 3, COLOR_PILL_BORDER);

    int16_t keyLen = btn.keyHint ? strlen(btn.keyHint) : 0;
    int16_t labelLen = btn.label ? strlen(btn.label) : 0;
    int16_t iconW = (btn.icon != ICON_NONE) ? 7 : 0;

    bool showLabel = (labelLen > 0) && ((iconW + (keyLen + labelLen + 3) * 6) <= (pillW - 6));
    int16_t textW = keyLen * 6;
    if (showLabel) {
      textW += (labelLen + 1) * 6;
    }
    int16_t gap = (iconW > 0 && textW > 0) ? 3 : 0;
    int16_t totalW = iconW + gap + textW;

    int16_t startX = pillX + (pillW - totalW) / 2;
    int16_t contentY = pillY + (pillH - 7) / 2;

    if (btn.icon != ICON_NONE) {
      drawControlIcon(startX, contentY, btn.icon, COLOR_TEXT);
      startX += iconW + gap;
    }

    if (keyLen > 0) {
      canvas.setTextSize(1);
      canvas.setTextColor(COLOR_SECONDARY);
      canvas.setCursor(startX, contentY);
      canvas.print(btn.keyHint);
      startX += keyLen * 6;
    }

    if (showLabel) {
      startX += 6;
      canvas.setTextSize(1);
      canvas.setTextColor(COLOR_MUTED);
      canvas.setCursor(startX, contentY);
      canvas.print(btn.label);
    }
  }
}

void CardputerShell::renderFooterHint(const char* hintText, uint16_t color) {
  const int16_t fy = SCREEN_HEIGHT - FOOTER_HEIGHT;
  canvas.drawFastHLine(0, fy - 1, SCREEN_WIDTH, COLOR_DIVIDER);
  canvas.fillRect(0, fy, SCREEN_WIDTH, FOOTER_HEIGHT, COLOR_FOOTER_BG);

  if (hintText && strlen(hintText) > 0) {
    canvas.setTextSize(1);
    canvas.setTextColor(color);
    int16_t textW = strlen(hintText) * 6;
    int16_t textX = (SCREEN_WIDTH - textW) / 2;
    canvas.setCursor(textX, fy + 5);
    canvas.print(hintText);
  }
}

void CardputerShell::renderStatus(const char* title, const char* subtitle, uint16_t accentColor) {
  canvas.fillScreen(COLOR_BG);
  renderHeader(title);

  const int16_t cardX = 16;
  const int16_t cardY = 32;
  const int16_t cardW = SCREEN_WIDTH - 32;
  const int16_t cardH = 68;

  canvas.fillRoundRect(cardX, cardY, cardW, cardH, 4, COLOR_CARD_BG);
  canvas.drawRoundRect(cardX, cardY, cardW, cardH, 4, COLOR_DIVIDER);

  canvas.setTextSize(1);
  canvas.setTextColor(accentColor);
  int16_t tW = strlen(title) * 6;
  canvas.setCursor(cardX + (cardW - tW) / 2, cardY + 18);
  canvas.print(title);

  if (subtitle && strlen(subtitle) > 0) {
    canvas.setTextColor(COLOR_MUTED);
    int16_t sW = strlen(subtitle) * 6;
    canvas.setCursor(cardX + (cardW - sW) / 2, cardY + 36);
    canvas.print(subtitle);
  }

  FooterButton hintBtns[] = {
    { "S", "Setup", ICON_NONE },
    { "R", "Reboot", ICON_NONE }
  };
  renderFooter(hintBtns, 2);
  flush();
  needsRedraw = true;
}
