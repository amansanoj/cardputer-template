#include "config_manager.h"
#include "keyboard_driver.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

static const char PREFS_NAMESPACE[] = "app-cfg";
static const char KEY_SSID[] = "wifi_ssid";
static const char KEY_PASS[] = "wifi_pass";
static const char KEY_CONF[] = "configured";

ConfigManager::ConfigManager() {}

bool ConfigManager::load(AppConfig& config) {
  prefs.begin(PREFS_NAMESPACE, true);
  config.isConfigured = prefs.getBool(KEY_CONF, false);
  config.wifiSsid = prefs.getString(KEY_SSID, "");
  config.wifiPassword = prefs.getString(KEY_PASS, "");
  prefs.end();

#if defined(TARGET_WOKWI_SIMULATOR)
  if (!config.isConfigured || config.wifiSsid.length() == 0) {
    config.wifiSsid = WOKWI_DEFAULT_SSID;
    config.wifiPassword = WOKWI_DEFAULT_PASS;
    config.isConfigured = true;
    save(config);
    return true;
  }
#endif

  return config.isConfigured;
}

void ConfigManager::save(const AppConfig& config) {
  prefs.begin(PREFS_NAMESPACE, false);
  prefs.putString(KEY_SSID, config.wifiSsid);
  prefs.putString(KEY_PASS, config.wifiPassword);
  prefs.putBool(KEY_CONF, config.isConfigured);
  prefs.end();
  Serial.println("[Config] Settings saved to NVS!");
}

void ConfigManager::clear() {
  prefs.begin(PREFS_NAMESPACE, false);
  prefs.clear();
  prefs.end();
  Serial.println("[Config] Settings cleared!");
}

bool ConfigManager::runSetupPortal(CardputerShell& shell, AppConfig& config, KeyboardDriver* keyboard) {
  Serial.println("[Setup] Starting Captive Web Setup Portal...");

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID);
  delay(100);

  IPAddress apIP = WiFi.softAPIP();
  Serial.printf("[Setup] AP IP address: %s\n", apIP.toString().c_str());

  DNSServer dnsServer;
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(53, "*", apIP);

  WebServer server(80);
  bool configSaved = false;

  auto handleRoot = [&]() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Cardputer App Setup</title>
  <style>
    body { font-family: -apple-system, sans-serif; background: #080808; color: #e6e6e6; margin: 0; padding: 20px; }
    .card { max-width: 360px; margin: 0 auto; background: #121212; border: 1px solid #282828; border-radius: 12px; padding: 24px; }
    h2 { color: #afbdd9; margin-top: 0; font-size: 20px; }
    label { font-size: 13px; color: #808080; display: block; margin-top: 14px; margin-bottom: 6px; }
    input { width: 100%; box-sizing: border-box; background: #1a1a1a; border: 1px solid #333; color: #fff; padding: 10px 12px; border-radius: 6px; font-size: 15px; }
    button { width: 100%; margin-top: 24px; padding: 12px; background: #afbdd9; color: #050505; border: none; border-radius: 6px; font-size: 15px; font-weight: 600; cursor: pointer; }
  </style>
</head>
<body>
  <div class="card">
    <h2>Cardputer Setup</h2>
    <form action="/save" method="POST">
      <label>Wi-Fi Network (SSID)</label>
      <input type="text" name="ssid" value=")rawliteral" + config.wifiSsid + R"rawliteral(" required>
      <label>Wi-Fi Password</label>
      <input type="password" name="pass" value=")rawliteral" + config.wifiPassword + R"rawliteral(">
      <button type="submit">Save & Reboot</button>
    </form>
  </div>
</body>
</html>
)rawliteral";
    server.send(200, "text/html", html);
  };

  auto handleSave = [&]() {
    if (server.hasArg("ssid")) config.wifiSsid = server.arg("ssid");
    if (server.hasArg("pass")) config.wifiPassword = server.arg("pass");
    config.isConfigured = true;
    save(config);
    configSaved = true;

    String html = R"rawliteral(
<!DOCTYPE html><html><body style="background:#080808;color:#2cd85e;font-family:sans-serif;padding:30px;text-align:center;">
<h2>Configuration Saved!</h2><p>Cardputer is rebooting into the new network...</p>
</body></html>
)rawliteral";
    server.send(200, "text/html", html);
  };

  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.onNotFound([&]() {
    server.sendHeader("Location", "http://" + apIP.toString() + "/", true);
    server.send(302, "text/plain", "");
  });
  server.begin();

  shell.renderStatus("Setup Portal Active", AP_SSID, COLOR_SECONDARY);

  unsigned long startTime = millis();
  while (!configSaved) {
    dnsServer.processNextRequest();
    server.handleClient();

    if (keyboard) {
      char k = keyboard->getKey();
      if (k == 'x' || k == 'X' || k == 27) { // X or Esc to exit
        Serial.println("[Setup] Cancelled by user.");
        break;
      }
    }

    // Serial CLI
    if (Serial.available()) {
      String line = Serial.readStringUntil('\n');
      line.trim();
      if (line.equalsIgnoreCase("REBOOT") || line.equalsIgnoreCase("R")) {
        ESP.restart();
      } else if (line.equalsIgnoreCase("EXIT") || line.equalsIgnoreCase("X")) {
        break;
      } else if (line.startsWith("SET:")) {
        String data = line.substring(4);
        int c1 = data.indexOf(',');
        if (c1 >= 0) {
          config.wifiSsid = data.substring(0, c1);
          config.wifiPassword = data.substring(c1 + 1);
          config.isConfigured = true;
          save(config);
          configSaved = true;
        }
      }
    }

    delay(5);
  }

  server.stop();
  dnsServer.stop();
  WiFi.softAPdisconnect(true);
  return configSaved;
}
