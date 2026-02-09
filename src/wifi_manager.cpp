#include "wifi_manager.h"

void WiFiManager::begin(SettingsManager& settings) {
    _settings = &settings;
    _apName = settings.get().deviceName;
    if (_apName.isEmpty()) {
        _apName = DEFAULT_DEVICE_NAME;
    }

    WiFi.setHostname(_apName.c_str());

    if (settings.get().wifiSSID.length() > 0) {
        startSTA();
    } else {
        startAP();
    }
}

void WiFiManager::update() {
    if (_state == WIFI_STATE_CONNECTING) {
        if (WiFi.status() == WL_CONNECTED) {
            _state = WIFI_STATE_CONNECTED;
            Serial.print("WiFi connected, IP: ");
            Serial.println(WiFi.localIP());
        } else if (millis() - _connectStart > CONNECT_TIMEOUT) {
            Serial.println("WiFi connection timeout, starting AP");
            startAP();
        }
    } else if (_state == WIFI_STATE_CONNECTED) {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("WiFi disconnected, reconnecting...");
            startSTA();
        }
    }
}

String WiFiManager::getIP() const {
    if (_state == WIFI_STATE_CONNECTED) {
        return WiFi.localIP().toString();
    } else if (_state == WIFI_STATE_AP_MODE) {
        return WiFi.softAPIP().toString();
    }
    return "0.0.0.0";
}

void WiFiManager::startAP() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(_apName.c_str());
    _state = WIFI_STATE_AP_MODE;
    Serial.print("AP started: ");
    Serial.println(_apName);
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
}

void WiFiManager::startSTA() {
    if (!_settings || _settings->get().wifiSSID.isEmpty()) {
        startAP();
        return;
    }

    WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);
    WiFi.begin(_settings->get().wifiSSID.c_str(),
               _settings->get().wifiPassword.c_str());
    _state = WIFI_STATE_CONNECTING;
    _connectStart = millis();
    Serial.print("Connecting to WiFi: ");
    Serial.println(_settings->get().wifiSSID);
}

void WiFiManager::disconnect() {
    WiFi.disconnect(true);
    _state = WIFI_STATE_DISCONNECTED;
}
