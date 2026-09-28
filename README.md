# Cardputer App Template

A modular, extensible application shell and starter template for the **M5Stack Cardputer** and **Cardputer-Adv** (ESP32-S3).

This template gives all your Cardputer applications a unified, polished look-and-feel with a standardized header and footer, while providing a clean canvas for your custom views.

---

## What’s Included

* **Unified Header Bar (Top 16px):**
  * Live clock (synchronized from local RTC / NTP)
  * Centered app title in primary accent (`#afbdd9`)
  * Wi-Fi status indicator (3-bar signal meter)
  * Battery percentage & battery capsule with active charging detection
* **Unified Action Bar (Bottom 17px):**
  * Auto-distributed action pills (`[ Key | Label ]` with optional icons)
  * Dynamically configured by the active view
* **Middle Content Viewport (102px):**
  * Clean `240 × 102` rendering area (`y: 16` to `y: 118`)
* **TCA8418 Keyboard Driver:**
  * Physical 56-key matrix scanning on Cardputer-Adv
  * Virtual Serial keystroke injection for Wokwi simulation
* **Captive Setup Portal (`ConfigManager`):**
  * Press `'S'` anywhere to launch the Wi-Fi setup SoftAP
  * Responsive dark-mode Web UI + Serial CLI fallback
* **Dual Target Environments:**
  * `esp32s3`: Ready for instant browser simulation in **Wokwi**
  * `cardputer-adv`: Ready for 1-click flashing to **Cardputer-Adv** hardware

---

## Project Structure

```
cardputer-template/
├── diagram.json             # Wokwi simulation hardware wiring
├── wokwi.toml               # Wokwi simulator configuration
├── st7789.chip.*            # Wokwi custom ST7789 display simulation chip
└── firmware/
    ├── platformio.ini       # Dual-target build configuration
    ├── include/
    │   ├── config.h         # Pinouts, 60-30-10 color palette, metrics
    │   ├── cardputer_shell.h# Header & Footer shell renderer
    │   ├── app_view.h       # Abstract base class for app views
    │   ├── keyboard_driver.h# TCA8418 matrix keyboard driver
    │   ├── config_manager.h # Wi-Fi NVS storage & Captive Portal
    │   └── demo_view.h      # Example starter dashboard view
    └── src/
        ├── main.cpp         # Shell lifecycle & view manager
        ├── cardputer_shell.cpp
        ├── keyboard_driver.cpp
        ├── config_manager.cpp
        └── demo_view.cpp    # Sample interactive metrics card view
```

---

## How to Create a New App View in 3 Steps

### 1. Inherit from `AppView`

Create `my_app_view.h`:
```cpp
#pragma once
#include "app_view.h"

class MyAppView : public AppView {
public:
  void render(GFXcanvas16& canvas, int16_t contentY, int16_t contentH) override {
    canvas.setTextSize(1);
    canvas.setTextColor(COLOR_PRIMARY);
    canvas.setCursor(16, contentY + 20);
    canvas.print("Hello from My Cardputer App!");
  }

  bool handleKey(char key) override {
    if (key == 'a') {
      Serial.println("Key A pressed!");
      return true;
    }
    return false;
  }

  const char* getTitle() const override {
    return "My App";
  }

  size_t getFooterButtons(FooterButton* outButtons, size_t maxButtons) override {
    outButtons[0] = { "A", "Action", ICON_NONE };
    outButtons[1] = { "S", "Setup", ICON_NONE };
    return 2;
  }
};
```

### 2. Mount it in `main.cpp`

Replace `DemoView demoView` with your view:
```cpp
MyAppView myApp;
AppView* currentView = &myApp;
```

### 3. Build & Run

```bash
# Test virtually in Wokwi Simulator:
pio run -d firmware -e esp32s3

# Flash to physical Cardputer-Adv:
pio run -d firmware -e cardputer-adv -t upload
```

---

## Color Palette (60-30-10 Rule)

| Token | Hex (RGB565) | Hex (sRGB) | Purpose |
| :--- | :--- | :--- | :--- |
| `COLOR_BG` | `0x0000` | `#050505` | Deep canvas background (60%) |
| `COLOR_HEADER_BG` | `0x0861` | `#0d0d0d` | Header dark strip |
| `COLOR_FOOTER_BG` | `0x0861` | `#0d0d0d` | Footer dark strip |
| `COLOR_DIVIDER` | `0x18E3` | `#1a1a1a` | Subtle container borders |
| `COLOR_PRIMARY` | `0xADFB` | `#afbdd9` | App titles & active elements (30%) |
| `COLOR_SECONDARY` | `0xF506` | `#f0a133` | Keyboard shortcuts & accents (10%) |
| `COLOR_ACCENT` | `0xDCD3` | `#df9a9e` | Alerts & active badges (10%) |
