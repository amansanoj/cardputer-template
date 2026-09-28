#pragma once
#include "app_view.h"

class DemoView : public AppView {
public:
  DemoView();

  void onEnter() override;
  void update() override;
  void render(GFXcanvas16& canvas, int16_t contentY, int16_t contentH) override;
  bool handleKey(char key) override;
  const char* getTitle() const override { return "System Dashboard"; }
  size_t getFooterButtons(FooterButton* outButtons, size_t maxButtons) override;

private:
  int32_t counter;
  uint8_t selectedTab;
  unsigned long lastTick;
};
