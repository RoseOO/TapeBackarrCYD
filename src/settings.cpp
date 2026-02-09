#include "settings.h"

void SettingsManager::begin() {
    _prefs.begin("tapebackarr", false);
    load();
}

void SettingsManager::load() {
    _settings.wifiSSID       = _prefs.getString("wifi_ssid", "");
    _settings.wifiPassword   = _prefs.getString("wifi_pass", "");
    _settings.serverHost     = _prefs.getString("srv_host", DEFAULT_SERVER_HOST);
    _settings.serverPort     = _prefs.getUShort("srv_port", DEFAULT_SERVER_PORT);
    _settings.apiKey         = _prefs.getString("api_key", DEFAULT_API_KEY);
    _settings.useHTTPS       = _prefs.getBool("use_https", false);
    _settings.brightness     = _prefs.getUChar("brightness", DEFAULT_BRIGHTNESS);
    _settings.pollInterval   = _prefs.getUShort("poll_int", DEFAULT_POLL_INTERVAL);
    _settings.deviceName     = _prefs.getString("dev_name", DEFAULT_DEVICE_NAME);
}

void SettingsManager::save() {
    _prefs.putString("wifi_ssid", _settings.wifiSSID);
    _prefs.putString("wifi_pass", _settings.wifiPassword);
    _prefs.putString("srv_host", _settings.serverHost);
    _prefs.putUShort("srv_port", _settings.serverPort);
    _prefs.putString("api_key", _settings.apiKey);
    _prefs.putBool("use_https", _settings.useHTTPS);
    _prefs.putUChar("brightness", _settings.brightness);
    _prefs.putUShort("poll_int", _settings.pollInterval);
    _prefs.putString("dev_name", _settings.deviceName);
}

void SettingsManager::reset() {
    _prefs.clear();
    load();
}

bool SettingsManager::isConfigured() const {
    return _settings.wifiSSID.length() > 0 &&
           _settings.serverHost.length() > 0 &&
           _settings.apiKey.length() > 0;
}
