#pragma once

#include <Arduino.h>
#include <Preferences.h>

// Default values
#define DEFAULT_DEVICE_NAME    "TapeBackarr-CYD"
#define DEFAULT_SERVER_HOST    ""
#define DEFAULT_SERVER_PORT    8080
#define DEFAULT_API_KEY        ""
#define DEFAULT_POLL_INTERVAL  5
#define DEFAULT_BRIGHTNESS     100

struct AppSettings {
    // WiFi
    String wifiSSID;
    String wifiPassword;

    // TapeBackarr server
    String serverHost;
    uint16_t serverPort;
    String apiKey;
    bool useHTTPS;

    // Display
    uint8_t brightness;
    uint16_t pollInterval; // seconds

    // Device
    String deviceName;
};

class SettingsManager {
public:
    void begin();
    void load();
    void save();
    void reset();

    AppSettings& get() { return _settings; }

    bool isConfigured() const;

private:
    Preferences _prefs;
    AppSettings _settings;
};
