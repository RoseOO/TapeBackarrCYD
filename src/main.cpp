/*
 * TapeBackarr CYD Monitor
 *
 * ESP32 Cheap Yellow Display (CYD2USB) application for monitoring
 * TapeBackarr tape drive backup system.
 *
 * Features:
 *   - Real-time dashboard with tape/job/drive statistics
 *   - Active job monitoring with progress
 *   - Drive status display with loaded tape info and format type
 *   - LTFS format progress monitoring
 *   - Tape change alerts with LED notification
 *   - Touch-based screen navigation
 *   - Web-based configuration interface
 *   - WiFi AP fallback for initial setup
 *   - CYD IP address shown on connection error screens
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

#define FW_VERSION "1.1.0"

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
bool          alertDismissed = false;  // Locally dismissed, re-shows if server still pending
bool          initialBoot    = true;

// Cached data
DashboardData              dashboardData = {};
std::vector<ActiveJobData> activeJobs;
std::vector<DriveData>     drives;
std::vector<TapeChangeData> tapeChanges;
LTFSFormatStatus           ltfsFormatStatus = {};

#define TOUCH_DEBOUNCE 300  // ms

void fetchAllData() {
    if (!wifiMgr.isConnected() || !settings.isConfigured()) return;

    bool hadJobs = !activeJobs.empty();

    dashboardData = apiClient.fetchDashboard();
    yield(); webServer.handleClient();
    activeJobs    = apiClient.fetchActiveJobs();
    yield(); webServer.handleClient();
    drives        = apiClient.fetchDrives();
    yield(); webServer.handleClient();
    tapeChanges   = apiClient.fetchTapeChanges();
    yield(); webServer.handleClient();
    ltfsFormatStatus = apiClient.fetchLTFSFormatStatus();
    yield(); webServer.handleClient();

    // Auto-switch to Jobs tab when a new job appears
    if (!hadJobs && !activeJobs.empty()) {
        currentTab = 1;
    }

    // Alert persists as long as server reports pending tape changes.
    // If server clears the event (tape was changed), reset everything.
    if (tapeChanges.empty()) {
        hasAlert = false;
        alertDismissed = false;
    } else {
        hasAlert = true;
    }
}

void refreshDisplay() {
    // Show alert if there are pending tape changes and not locally dismissed
    if (hasAlert && !alertDismissed) {
        display.showTapeAlert(tapeChanges[0].reason);
        return;
    }

    // Show LTFS format progress if a format operation is active
    if (ltfsFormatStatus.valid && ltfsFormatStatus.active) {
        display.showLTFSFormat(ltfsFormatStatus);
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
    uint16_t tx = 0, ty = 0;
    if (!display.readTouch(tx, ty)) return;

    unsigned long now = millis();
    if (now - lastTouchTime < TOUCH_DEBOUNCE) return;
    lastTouchTime = now;

    // If alert showing, temporarily dismiss it (will re-appear on next
    // poll if the server still reports pending tape change events)
    if (display.getCurrentScreen() == SCREEN_ALERT) {
        alertDismissed = true;
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
            display.showError("Not configured - open web UI", wifiMgr.getIP());
        }
    }

    // Periodic polling
    unsigned long pollMs = (unsigned long)settings.get().pollInterval * 1000UL;
    if (wifiMgr.isConnected() && settings.isConfigured() &&
        millis() - lastPoll >= pollMs) {
        lastPoll = millis();
        fetchAllData();

        if (!apiClient.isConnected()) {
            display.showError(apiClient.getLastError(), wifiMgr.getIP());
        } else {
            refreshDisplay();
        }
    }
}
