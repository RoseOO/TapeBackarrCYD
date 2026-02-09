#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include "settings.h"

enum WiFiState {
    WIFI_STATE_DISCONNECTED,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_AP_MODE
};

class WiFiManager {
public:
    void begin(SettingsManager& settings);
    void update();

    WiFiState getState() const { return _state; }
    String getIP() const;
    String getAPName() const { return _apName; }
    bool isConnected() const { return _state == WIFI_STATE_CONNECTED; }

    void startAP();
    void startSTA();
    void disconnect();

private:
    SettingsManager* _settings = nullptr;
    WiFiState _state = WIFI_STATE_DISCONNECTED;
    String _apName;
    unsigned long _connectStart = 0;
    static const unsigned long CONNECT_TIMEOUT = 15000;
};
