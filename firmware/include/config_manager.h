#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "config.h"
#include "cardputer_shell.h"

struct AppConfig {
  String wifiSsid = "";
  String wifiPassword = "";
  bool isConfigured = false;
};

class KeyboardDriver;

class ConfigManager {
public:
  ConfigManager();
  bool load(AppConfig& config);
  void save(const AppConfig& config);
  void clear();
  bool runSetupPortal(CardputerShell& shell, AppConfig& config, KeyboardDriver* keyboard = nullptr);

private:
  Preferences prefs;
};
