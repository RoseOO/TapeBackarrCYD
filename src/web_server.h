#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include "settings.h"
#include "wifi_manager.h"
#include "api_client.h"

class ConfigWebServer {
public:
    void begin(SettingsManager& settings, WiFiManager& wifi,
               APIClient& api);
    void handleClient();

private:
    WebServer _server{80};
    SettingsManager* _settings = nullptr;
    WiFiManager* _wifi = nullptr;
    APIClient* _api = nullptr;

    void handleRoot();
    void handleSave();
    void handleStatus();
    void handleReboot();
    void handleReset();
    void handleScan();

    String buildPage();
    String htmlEscape(const String& str);
};
