#pragma once

#include <Arduino.h>
#include "LGFX_Config.h"
#include "api_client.h"
#include "wifi_manager.h"

// CYD2USB RGB LED pins (active LOW)
#define LED_RED   4
#define LED_GREEN 16
#define LED_BLUE  17

// Display colors (RGB565)
#define COLOR_BG          0x1082  // Dark background
#define COLOR_CARD_BG     0x2104  // Card background
#define COLOR_TEXT         0xFFFF  // White text
#define COLOR_TEXT_DIM     0x8410  // Grey text
#define COLOR_ACCENT       0x04FF  // Teal accent
#define COLOR_SUCCESS      0x07E0  // Green
#define COLOR_WARNING      0xFDA0  // Orange
#define COLOR_ERROR        0xF800  // Red
#define COLOR_HEADER_BG   0x0019  // Dark blue header
#define COLOR_TAB_ACTIVE  0x04FF  // Active tab
#define COLOR_TAB_INACTIVE 0x2104  // Inactive tab
#define COLOR_PROGRESS_BG 0x2104  // Progress bar background
#define COLOR_PROGRESS_FG 0x04FF  // Progress bar foreground

enum DisplayScreen {
    SCREEN_BOOT,
    SCREEN_AP_MODE,
    SCREEN_CONNECTING,
    SCREEN_DASHBOARD,
    SCREEN_JOBS,
    SCREEN_DRIVES,
    SCREEN_ALERT
};

class Display {
public:
    void begin();
    void update();

    void showBoot(const String& version);
    void showAPMode(const String& apName, const String& ip);
    void showConnecting(const String& ssid);
    void showDashboard(const DashboardData& data);
    void showActiveJobs(const std::vector<ActiveJobData>& jobs);
    void showDrives(const std::vector<DriveData>& drives);
    void showTapeAlert(const String& message);
    void showError(const String& error);

    void drawStatusBar(bool wifiConnected, bool apiConnected,
                       const String& ip);
    void drawTabBar(int activeTab);

    void setBrightness(uint8_t pct);

    // Touch handling
    bool isTouched();
    void getTouchPoint(uint16_t& x, uint16_t& y);
    int getTabFromTouch(uint16_t x, uint16_t y);

    DisplayScreen getCurrentScreen() const { return _currentScreen; }

    // LED control
    void setLED(bool r, bool g, bool b);
    void alertLED(bool on);

private:
    LGFX _tft;
    DisplayScreen _currentScreen = SCREEN_BOOT;
    unsigned long _lastAlertBlink = 0;
    bool _alertState = false;

    void drawHeader(const String& title);
    void drawCard(int x, int y, int w, int h, const String& label,
                  const String& value, uint16_t valueColor = COLOR_TEXT);
    void drawProgressBar(int x, int y, int w, int h,
                         float pct, uint16_t color = COLOR_PROGRESS_FG);
    String formatBytes(int64_t bytes);
    String formatDuration(unsigned long seconds);
};
