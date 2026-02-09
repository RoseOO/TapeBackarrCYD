/*
 * TapeBackarr CYD Monitor
 *
 * ESP32 Cheap Yellow Display (CYD2USB) application for monitoring
 * TapeBackarr tape drive backup system.
 *
 * Features:
 *   - Real-time dashboard with tape/job/drive statistics
 *   - Active job monitoring with progress
 *   - Drive status display with loaded tape info
 *   - Tape change alerts with LED notification
 *   - Touch-based screen navigation
 *   - Web-based configuration interface
 *   - WiFi AP fallback for initial setup
 *
 * Hardware: ESP32-2432S028 (CYD2USB variant with USB-C)
 *           2.8" 320x240 ILI9341 TFT + XPT2046 touch
 *
 * https://github.com/RoseOO/TapeBackarr
 * https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display
 */

#include <Arduino.h>
#include "settings.h"
#include "wifi_manager.h"
#include "api_client.h"
#include "display.h"
#include "web_server.h"

#define FW_VERSION "1.0.0"

// Global instances
SettingsManager settings;
WiFiManager     wifiMgr;
APIClient       apiClient;
Display         display;
ConfigWebServer webServer;

// State
unsigned long lastPoll       = 0;
unsigned long lastTouchTime  = 0;
int           currentTab     = 0;
bool          hasAlert       = false;
bool          initialBoot    = true;

// Cached data
DashboardData              dashboardData = {};
std::vector<ActiveJobData> activeJobs;
std::vector<DriveData>     drives;
std::vector<TapeChangeData> tapeChanges;

#define TOUCH_DEBOUNCE 300  // ms

void fetchAllData() {
    if (!wifiMgr.isConnected() || !settings.isConfigured()) return;

    dashboardData = apiClient.fetchDashboard();
    activeJobs    = apiClient.fetchActiveJobs();
    drives        = apiClient.fetchDrives();
    tapeChanges   = apiClient.fetchTapeChanges();

    // Check for tape change alerts
    hasAlert = !tapeChanges.empty();
}

void refreshDisplay() {
    if (hasAlert) {
        String reason = tapeChanges.empty() ? "Tape change needed" : tapeChanges[0].reason;
        display.showTapeAlert(reason);
        return;
    }

    switch (currentTab) {
        case 0:
            display.showDashboard(dashboardData);
            break;
        case 1:
            display.showActiveJobs(activeJobs);
            break;
        case 2:
            display.showDrives(drives);
            break;
    }
}

void handleTouch() {
    if (!display.isTouched()) return;

    unsigned long now = millis();
    if (now - lastTouchTime < TOUCH_DEBOUNCE) return;
    lastTouchTime = now;

    uint16_t tx, ty;
    display.getTouchPoint(tx, ty);

    // If alert showing, dismiss it
    if (hasAlert) {
        hasAlert = false;
        tapeChanges.clear();
        refreshDisplay();
        return;
    }

    // Check tab bar
    int tab = display.getTabFromTouch(tx, ty);
    if (tab >= 0 && tab != currentTab) {
        currentTab = tab;
        refreshDisplay();
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.println("=== TapeBackarr CYD v" FW_VERSION " ===");

    // Initialize display first for visual feedback
    display.begin();
    display.showBoot("v" FW_VERSION);

    // Load settings
    settings.begin();
    display.setBrightness(settings.get().brightness);

    delay(1500);  // Show boot screen briefly

    // Initialize WiFi
    wifiMgr.begin(settings);

    if (wifiMgr.getState() == WIFI_STATE_AP_MODE) {
        display.showAPMode(wifiMgr.getAPName(), wifiMgr.getIP());
    } else {
        display.showConnecting(settings.get().wifiSSID);
    }

    // Initialize API client
    apiClient.begin(settings);

    // Start web server (works in both STA and AP mode)
    webServer.begin(settings, wifiMgr, apiClient);

    Serial.println("Setup complete");
}

void loop() {
    // Update WiFi state
    wifiMgr.update();

    // Handle web server requests
    webServer.handleClient();

    // Update display animations
    display.update();

    // Handle touch input
    handleTouch();

    // Show AP mode screen if in AP mode
    if (wifiMgr.getState() == WIFI_STATE_AP_MODE) {
        if (display.getCurrentScreen() != SCREEN_AP_MODE) {
            display.showAPMode(wifiMgr.getAPName(), wifiMgr.getIP());
        }
        return;
    }

    // Show connecting screen while connecting
    if (wifiMgr.getState() == WIFI_STATE_CONNECTING) {
        if (display.getCurrentScreen() != SCREEN_CONNECTING) {
            display.showConnecting(settings.get().wifiSSID);
        }
        return;
    }

    // Once connected, do initial data fetch
    if (initialBoot && wifiMgr.isConnected()) {
        initialBoot = false;
        Serial.println("WiFi connected, fetching initial data...");

        if (settings.isConfigured()) {
            fetchAllData();
            refreshDisplay();
        } else {
            display.showError("Not configured - open web UI");
        }
    }

    // Periodic polling
    unsigned long pollMs = (unsigned long)settings.get().pollInterval * 1000UL;
    if (wifiMgr.isConnected() && settings.isConfigured() &&
        millis() - lastPoll >= pollMs) {
        lastPoll = millis();
        fetchAllData();

        if (!apiClient.isConnected()) {
            display.showError(apiClient.getLastError());
        } else {
            refreshDisplay();
        }
    }
}
