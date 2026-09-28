#include "demo_view.h"
#include <WiFi.h>

DemoView::DemoView() : counter(42), selectedTab(0), lastTick(0) {}

void DemoView::onEnter() {
  Serial.println("[DemoView] Entered active view.");
  markDirty();
}

void DemoView::update() {
  uint32_t sec = millis() / 1000;
  if (sec != lastTick) {
    lastTick = sec;
    markDirty(); // Uptime counter updated
  }
}

void DemoView::render(GFXcanvas16& canvas, int16_t contentY, int16_t contentH) {
  // Draw two sleek 60-30-10 cards side-by-side
  const int16_t pad = 8;
  const int16_t cardY = contentY + pad;
  const int16_t cardH = contentH - (pad * 2);
  const int16_t colW = (SCREEN_WIDTH - (pad * 3)) / 2; // 108px

  // Card 1: System Info
  const int16_t c1X = pad;
  canvas.fillRoundRect(c1X, cardY, colW, cardH, 4, COLOR_CARD_BG);
  canvas.drawRoundRect(c1X, cardY, colW, cardH, 4, COLOR_DIVIDER);

  canvas.setTextSize(1);
  canvas.setTextColor(COLOR_PRIMARY);
  canvas.setCursor(c1X + 8, cardY + 8);
  canvas.print("SYSTEM");

  canvas.setTextColor(COLOR_MUTED);
  canvas.setCursor(c1X + 8, cardY + 22);
  canvas.print("Chip: S3 240M");

  char ramBuf[20];
  snprintf(ramBuf, sizeof(ramBuf), "Free: %uK", (unsigned int)(ESP.getFreeHeap() / 1024));
  canvas.setCursor(c1X + 8, cardY + 34);
  canvas.print(ramBuf);

  uint32_t sec = millis() / 1000;
  char upBuf[20];
  snprintf(upBuf, sizeof(upBuf), "Up: %02u:%02u", (unsigned int)(sec / 60), (unsigned int)(sec % 60));
  canvas.setCursor(c1X + 8, cardY + 46);
  canvas.print(upBuf);

  canvas.setCursor(c1X + 8, cardY + 58);
  canvas.setTextColor((WiFi.status() == WL_CONNECTED) ? COLOR_SUCCESS : COLOR_SECONDARY);
  canvas.print((WiFi.status() == WL_CONNECTED) ? "WiFi: Ready" : "WiFi: Idle");

  // Card 2: Interactive Counter & Progress
  const int16_t c2X = c1X + colW + pad;
  canvas.fillRoundRect(c2X, cardY, colW, cardH, 4, COLOR_CARD_BG);
  canvas.drawRoundRect(c2X, cardY, colW, cardH, 4, COLOR_DIVIDER);

  canvas.setTextColor(COLOR_PRIMARY);
  canvas.setCursor(c2X + 8, cardY + 8);
  canvas.print("METRICS");

  canvas.setTextColor(COLOR_TEXT);
  char countBuf[24];
  snprintf(countBuf, sizeof(countBuf), "Value: %d", (int)counter);
  canvas.setCursor(c2X + 8, cardY + 24);
  canvas.print(countBuf);

  // Dynamic progress bar
  const int16_t barX = c2X + 8;
  const int16_t barY = cardY + 40;
  const int16_t barW = colW - 16;
  const int16_t barH = 6;
  canvas.fillRoundRect(barX, barY, barW, barH, 2, COLOR_BAR_BG);

  int16_t fillW = (abs(counter) % 100) * barW / 100;
  if (fillW > 0) {
    canvas.fillRoundRect(barX, barY, fillW, barH, 2, COLOR_SECONDARY);
  }

  canvas.setTextColor(COLOR_MUTED);
  canvas.setCursor(c2X + 8, cardY + 54);
  canvas.print("Press Up/Down");
}

bool DemoView::handleKey(char key) {
  if (key == ',' || key == '<' || key == 'w' || key == 'W') {
    counter++;
    markDirty();
    return true;
  } else if (key == '.' || key == '>' || key == 's' || key == 'S') {
    counter--;
    markDirty();
    return true;
  } else if (key == ' ' || key == '\n') {
    counter = 0;
    markDirty();
    return true;
  }
  return false;
}

size_t DemoView::getFooterButtons(FooterButton* outButtons, size_t maxButtons) {
  if (maxButtons < 4) return 0;
  outButtons[0] = { ",", "Inc", ICON_UP };
  outButtons[1] = { ".", "Dec", ICON_DOWN };
  outButtons[2] = { "Spc", "Reset", ICON_NONE };
  outButtons[3] = { "S", "Setup", ICON_NONE };
  return 4;
}
