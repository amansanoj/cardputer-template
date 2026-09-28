#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include "cardputer_shell.h"

/**
 * Base class for all Cardputer application views.
 * Simply inherit from AppView and implement render() and handleKey()!
 */
class AppView {
public:
  AppView() : dirty(true) {}
  virtual ~AppView() {}

  // Called when the view becomes active
  virtual void onEnter() { markDirty(); }

  // Called when the view is about to be replaced
  virtual void onExit() {}

  // Periodic update logic (e.g. state machines, timers)
  virtual void update() {}

  // Render content in the middle area (x: 0..240, y: contentY..contentY+contentH)
  virtual void render(GFXcanvas16& canvas, int16_t contentY, int16_t contentH) = 0;

  // Handle keyboard events. Return true if consumed.
  virtual bool handleKey(char key) = 0;

  // Screen title displayed in the center of the header
  virtual const char* getTitle() const = 0;

  // Footer action buttons (e.g. key shortcuts and labels)
  virtual size_t getFooterButtons(FooterButton* outButtons, size_t maxButtons) = 0;

  // Dirty frame tracking: avoids pushing redundant 65KB SPI frames to Wokwi/display
  bool isDirty() const { return dirty; }
  void markDirty() { dirty = true; }
  void clearDirty() { dirty = false; }

protected:
  bool dirty;
};
