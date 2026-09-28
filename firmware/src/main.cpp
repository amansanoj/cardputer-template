#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include "config.h"
#include "cardputer_shell.h"
#include "keyboard_driver.h"
#include "config_manager.h"
#include "demo_view.h"

// Hardware and Framework Shell
CardputerShell shell;
KeyboardDriver keyboard;
ConfigManager configManager;
AppConfig appConfig;

// Active Application View
DemoView demoView;
AppView* currentView = &demoView;

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.printf("\n==================================\n  %s Shell v1.0.0\n==================================\n", BOARD_NAME);

  // 1. Initialize Display & Shell
  shell.begin();
  shell.renderStatus(BOARD_NAME, "Starting up...", COLOR_PRIMARY);

  // 2. Initialize Keyboard Driver
  keyboard.begin();

  // 3. Load Persistent Configuration
  configManager.load(appConfig);

  // 4. Initialize SNTP Timekeeper (NTP pool sync with local timezone offset)
  configTime(DEFAULT_GMT_OFFSET_SEC, DEFAULT_DAYLIGHT_OFFSET_SEC, "pool.ntp.org", "time.google.com");

  // 5. Connect to Wi-Fi if credentials exist
  if (appConfig.isConfigured && appConfig.wifiSsid.length() > 0) {
    Serial.printf("[WiFi] Connecting to %s...\n", appConfig.wifiSsid.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.begin(appConfig.wifiSsid.c_str(), appConfig.wifiPassword.c_str());
  }

  // 6. Initialize Active View
  if (currentView) {
    currentView->onEnter();
  }

  shell.requestRedraw();
}

void loop() {
  // 1. Handle Keyboard Input
  char key = keyboard.getKey();
  if (key != 0) {
    if (key == 's' || key == 'S') {
      bool saved = configManager.runSetupPortal(shell, appConfig, &keyboard);
      if (saved) {
        Serial.println("[Main] Configuration saved. Rebooting...");
        delay(100);
        ESP.restart();
      }
      shell.requestRedraw();
      if (currentView) currentView->onEnter();
    } else {
      if (currentView) {
        if (currentView->handleKey(key)) {
          shell.requestRedraw();
        }
      }
    }
  }

  // 2. Periodic View Logic
  if (currentView) {
    currentView->update();
  }

  // 3. Monitor Wi-Fi state changes
  static bool lastWifi = false;
  bool curWifi = (WiFi.status() == WL_CONNECTED);
  if (curWifi != lastWifi) {
    lastWifi = curWifi;
    shell.requestRedraw();
  }

  // 4. Intelligent Dirty Frame Check:
  // ONLY blit 65KB over SPI to Wokwi / display when pixels actually changed!
  // Prevents the "CRT scanline monitor" slow-draw effect.
  bool viewDirty = currentView ? currentView->isDirty() : false;
  if (shell.isDirty() || viewDirty) {
    shell.beginFrame();

    int8_t rssi = curWifi ? WiFi.RSSI() : -100;
    const char* title = currentView ? currentView->getTitle() : "App";
    shell.renderHeader(title, nullptr, curWifi, rssi);

    if (currentView) {
      currentView->render(shell.getCanvas(), shell.getContentY(), shell.getContentH());
      currentView->clearDirty();
    }

    FooterButton buttons[5];
    size_t count = 0;
    if (currentView) {
      count = currentView->getFooterButtons(buttons, 5);
    }
    shell.renderFooter(buttons, count);

    // Push single crisp frame to physical/virtual display
    shell.flush();
    shell.clearDirty();
  }

  delay(10);
}
