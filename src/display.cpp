#include "display.h"

// XPT2046 touch pins for CYD (separate SPI bus)
#define XPT2046_IRQ  36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK  25
#define XPT2046_CS   33

// Screen dimensions
#define SCREEN_W 320
#define SCREEN_H 240

// Layout constants
#define STATUS_BAR_H  20
#define TAB_BAR_H     30
#define CONTENT_Y     (STATUS_BAR_H)
#define CONTENT_H     (SCREEN_H - STATUS_BAR_H - TAB_BAR_H)
#define TAB_BAR_Y     (SCREEN_H - TAB_BAR_H)
#define TAB_COUNT     3

void Display::begin() {
    _tft.init();
    _tft.setRotation(1);  // Landscape
    _tft.fillScreen(COLOR_BG);

    // Backlight
    pinMode(TFT_BL, OUTPUT);
    setBrightness(100);

    // RGB LED
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_BLUE, OUTPUT);
    setLED(false, false, false);

    // Touch calibration will use TFT_eSPI built-in
    uint16_t calData[5] = {239, 3587, 268, 3580, 7};
    _tft.setTouch(calData);
}

void Display::update() {
    // Blink LED during alerts
    if (_currentScreen == SCREEN_ALERT) {
        if (millis() - _lastAlertBlink > 500) {
            _alertState = !_alertState;
            alertLED(_alertState);
            _lastAlertBlink = millis();
        }
    }
}

void Display::showBoot(const String& version) {
    _currentScreen = SCREEN_BOOT;
    _tft.fillScreen(COLOR_BG);

    _tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    _tft.setTextDatum(MC_DATUM);

    _tft.setTextSize(1);
    _tft.setFreeFont(nullptr);
    _tft.drawString("TAPEBACKARR", SCREEN_W / 2, 80, 4);

    _tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    _tft.drawString("CYD Monitor", SCREEN_W / 2, 120, 2);

    _tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    _tft.drawString(version, SCREEN_W / 2, 150, 2);

    _tft.drawString("Initializing...", SCREEN_W / 2, 190, 2);
    _tft.setTextDatum(TL_DATUM);
}

void Display::showAPMode(const String& apName, const String& ip) {
    _currentScreen = SCREEN_AP_MODE;
    _tft.fillScreen(COLOR_BG);

    _tft.setTextColor(COLOR_WARNING, COLOR_BG);
    _tft.setTextDatum(MC_DATUM);
    _tft.drawString("SETUP MODE", SCREEN_W / 2, 30, 4);

    _tft.setTextColor(COLOR_TEXT, COLOR_BG);
    _tft.drawString("Connect to WiFi:", SCREEN_W / 2, 75, 2);

    _tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    _tft.drawString(apName, SCREEN_W / 2, 100, 4);

    _tft.setTextColor(COLOR_TEXT, COLOR_BG);
    _tft.drawString("Then open browser:", SCREEN_W / 2, 140, 2);

    _tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    _tft.drawString("http://" + ip, SCREEN_W / 2, 165, 4);

    _tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    _tft.drawString("Configure WiFi & API settings", SCREEN_W / 2, 210, 2);

    _tft.setTextDatum(TL_DATUM);
    setLED(false, false, true);  // Blue LED in AP mode
}

void Display::showConnecting(const String& ssid) {
    _currentScreen = SCREEN_CONNECTING;
    _tft.fillScreen(COLOR_BG);

    _tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    _tft.setTextDatum(MC_DATUM);
    _tft.drawString("Connecting...", SCREEN_W / 2, 100, 4);

    _tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    _tft.drawString(ssid, SCREEN_W / 2, 140, 2);
    _tft.setTextDatum(TL_DATUM);
}

void Display::showDashboard(const DashboardData& data) {
    _currentScreen = SCREEN_DASHBOARD;
    _tft.fillScreen(COLOR_BG);

    drawStatusBar(true, data.valid, "");
    drawTabBar(0);

    int y = CONTENT_Y + 5;

    // Title
    _tft.setTextColor(COLOR_TEXT, COLOR_BG);
    _tft.drawString("Dashboard", 10, y, 4);
    y += 30;

    // Cards row 1
    drawCard(5, y, 100, 50, "Tapes", String(data.totalTapes), COLOR_ACCENT);
    drawCard(110, y, 100, 50, "Active", String(data.activeTapes), COLOR_SUCCESS);
    drawCard(215, y, 100, 50, "Full", String(data.fullTapes), COLOR_WARNING);
    y += 58;

    // Cards row 2
    drawCard(5, y, 100, 50, "Jobs", String(data.totalJobs), COLOR_ACCENT);
    drawCard(110, y, 100, 50, "Running", String(data.activeJobs),
             data.activeJobs > 0 ? COLOR_SUCCESS : COLOR_TEXT_DIM);
    drawCard(215, y, 100, 50, "Drives", String(data.totalDrives), COLOR_ACCENT);
    y += 58;

    // Storage bar
    float usedPct = 0;
    if (data.totalCapacityBytes > 0) {
        usedPct = (float)data.usedCapacityBytes / (float)data.totalCapacityBytes;
    }
    _tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    _tft.drawString("Storage: " + formatBytes(data.usedCapacityBytes) +
                     " / " + formatBytes(data.totalCapacityBytes), 10, y, 2);
    y += 18;
    drawProgressBar(10, y, SCREEN_W - 20, 12, usedPct,
                    usedPct > 0.9f ? COLOR_ERROR : COLOR_PROGRESS_FG);

    setLED(false, true, false);  // Green LED when connected
}

void Display::showActiveJobs(const std::vector<ActiveJobData>& jobs) {
    _currentScreen = SCREEN_JOBS;
    _tft.fillScreen(COLOR_BG);

    drawStatusBar(true, true, "");
    drawTabBar(1);

    int y = CONTENT_Y + 5;
    _tft.setTextColor(COLOR_TEXT, COLOR_BG);
    _tft.drawString("Active Jobs", 10, y, 4);
    y += 30;

    if (jobs.empty()) {
        _tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
        _tft.setTextDatum(MC_DATUM);
        _tft.drawString("No active jobs", SCREEN_W / 2, y + 50, 4);
        _tft.setTextDatum(TL_DATUM);
        return;
    }

    for (size_t i = 0; i < jobs.size() && i < 3; i++) {
        const auto& job = jobs[i];

        // Job card
        _tft.fillRoundRect(5, y, SCREEN_W - 10, 48, 4, COLOR_CARD_BG);

        // Status indicator
        uint16_t statusColor = COLOR_TEXT_DIM;
        if (job.status == "running") statusColor = COLOR_SUCCESS;
        else if (job.status == "paused") statusColor = COLOR_WARNING;
        _tft.fillCircle(15, y + 15, 5, statusColor);

        // Job name
        _tft.setTextColor(COLOR_TEXT, COLOR_CARD_BG);
        _tft.drawString(job.name.substring(0, 25), 28, y + 5, 2);

        // Stats
        _tft.setTextColor(COLOR_TEXT_DIM, COLOR_CARD_BG);
        String stats = String(job.filesProcessed) + " files | " +
                       formatBytes(job.bytesProcessed);
        _tft.drawString(stats, 28, y + 27, 1);

        // Status badge
        _tft.setTextColor(statusColor, COLOR_CARD_BG);
        _tft.drawString(job.status, SCREEN_W - 70, y + 5, 2);

        y += 55;
    }
}

void Display::showDrives(const std::vector<DriveData>& drives) {
    _currentScreen = SCREEN_DRIVES;
    _tft.fillScreen(COLOR_BG);

    drawStatusBar(true, true, "");
    drawTabBar(2);

    int y = CONTENT_Y + 5;
    _tft.setTextColor(COLOR_TEXT, COLOR_BG);
    _tft.drawString("Drives", 10, y, 4);
    y += 30;

    if (drives.empty()) {
        _tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
        _tft.setTextDatum(MC_DATUM);
        _tft.drawString("No drives found", SCREEN_W / 2, y + 50, 4);
        _tft.setTextDatum(TL_DATUM);
        return;
    }

    for (size_t i = 0; i < drives.size() && i < 3; i++) {
        const auto& drive = drives[i];

        _tft.fillRoundRect(5, y, SCREEN_W - 10, 48, 4, COLOR_CARD_BG);

        // Status indicator
        uint16_t statusColor = COLOR_TEXT_DIM;
        if (drive.status == "ready") statusColor = COLOR_SUCCESS;
        else if (drive.status == "busy") statusColor = COLOR_WARNING;
        else if (drive.status == "error") statusColor = COLOR_ERROR;
        _tft.fillCircle(15, y + 15, 5, statusColor);

        // Drive name
        _tft.setTextColor(COLOR_TEXT, COLOR_CARD_BG);
        _tft.drawString(drive.displayName.substring(0, 22), 28, y + 5, 2);

        // Tape info
        _tft.setTextColor(COLOR_TEXT_DIM, COLOR_CARD_BG);
        String tape = "Tape: " + drive.currentTape;
        _tft.drawString(tape.substring(0, 35), 28, y + 27, 1);

        // Status badge
        _tft.setTextColor(statusColor, COLOR_CARD_BG);
        _tft.drawString(drive.status, SCREEN_W - 60, y + 5, 2);

        y += 55;
    }
}

void Display::showTapeAlert(const String& message) {
    _currentScreen = SCREEN_ALERT;
    _tft.fillScreen(COLOR_BG);

    // Alert header
    _tft.fillRect(0, 0, SCREEN_W, 50, COLOR_ERROR);
    _tft.setTextColor(COLOR_TEXT, COLOR_ERROR);
    _tft.setTextDatum(MC_DATUM);
    _tft.drawString("! TAPE CHANGE REQUIRED !", SCREEN_W / 2, 25, 4);

    // Message
    _tft.setTextColor(COLOR_WARNING, COLOR_BG);
    _tft.drawString(message, SCREEN_W / 2, 100, 2);

    _tft.setTextColor(COLOR_TEXT, COLOR_BG);
    _tft.drawString("Please insert a new tape", SCREEN_W / 2, 140, 2);
    _tft.drawString("into the drive", SCREEN_W / 2, 165, 2);

    _tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    _tft.drawString("Touch screen to dismiss", SCREEN_W / 2, 210, 2);

    _tft.setTextDatum(TL_DATUM);
    _lastAlertBlink = millis();
}

void Display::showError(const String& error) {
    _tft.fillRect(0, CONTENT_Y, SCREEN_W, CONTENT_H, COLOR_BG);

    _tft.setTextColor(COLOR_ERROR, COLOR_BG);
    _tft.setTextDatum(MC_DATUM);
    _tft.drawString("Connection Error", SCREEN_W / 2,
                     CONTENT_Y + CONTENT_H / 2 - 20, 4);

    _tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    _tft.drawString(error.substring(0, 35), SCREEN_W / 2,
                     CONTENT_Y + CONTENT_H / 2 + 15, 2);
    _tft.setTextDatum(TL_DATUM);

    setLED(true, false, false);  // Red LED on error
}

void Display::drawStatusBar(bool wifiConnected, bool apiConnected,
                             const String& ip) {
    _tft.fillRect(0, 0, SCREEN_W, STATUS_BAR_H, COLOR_HEADER_BG);

    _tft.setTextColor(COLOR_TEXT, COLOR_HEADER_BG);
    _tft.drawString("TapeBackarr", 5, 3, 2);

    // WiFi indicator
    uint16_t wifiColor = wifiConnected ? COLOR_SUCCESS : COLOR_ERROR;
    _tft.fillCircle(SCREEN_W - 50, STATUS_BAR_H / 2, 4, wifiColor);

    // API indicator
    uint16_t apiColor = apiConnected ? COLOR_SUCCESS : COLOR_ERROR;
    _tft.fillCircle(SCREEN_W - 30, STATUS_BAR_H / 2, 4, apiColor);

    // Connection label
    _tft.setTextColor(COLOR_TEXT_DIM, COLOR_HEADER_BG);
    _tft.drawString("W", SCREEN_W - 57, 4, 1);
    _tft.drawString("A", SCREEN_W - 37, 4, 1);
}

void Display::drawTabBar(int activeTab) {
    int tabW = SCREEN_W / TAB_COUNT;

    const char* labels[] = {"Dashboard", "Jobs", "Drives"};

    for (int i = 0; i < TAB_COUNT; i++) {
        uint16_t bg = (i == activeTab) ? COLOR_TAB_ACTIVE : COLOR_TAB_INACTIVE;
        uint16_t fg = (i == activeTab) ? COLOR_BG : COLOR_TEXT_DIM;

        _tft.fillRect(i * tabW, TAB_BAR_Y, tabW, TAB_BAR_H, bg);
        _tft.setTextColor(fg, bg);
        _tft.setTextDatum(MC_DATUM);
        _tft.drawString(labels[i], i * tabW + tabW / 2,
                         TAB_BAR_Y + TAB_BAR_H / 2, 2);
    }
    _tft.setTextDatum(TL_DATUM);

    // Separator lines
    for (int i = 1; i < TAB_COUNT; i++) {
        _tft.drawFastVLine(i * tabW, TAB_BAR_Y, TAB_BAR_H, COLOR_BG);
    }
}

void Display::drawCard(int x, int y, int w, int h, const String& label,
                        const String& value, uint16_t valueColor) {
    _tft.fillRoundRect(x, y, w, h, 4, COLOR_CARD_BG);

    _tft.setTextColor(COLOR_TEXT_DIM, COLOR_CARD_BG);
    _tft.setTextDatum(MC_DATUM);
    _tft.drawString(label, x + w / 2, y + 14, 1);

    _tft.setTextColor(valueColor, COLOR_CARD_BG);
    _tft.drawString(value, x + w / 2, y + 34, 4);

    _tft.setTextDatum(TL_DATUM);
}

void Display::drawProgressBar(int x, int y, int w, int h,
                               float pct, uint16_t color) {
    pct = constrain(pct, 0.0f, 1.0f);
    _tft.fillRoundRect(x, y, w, h, h / 2, COLOR_PROGRESS_BG);
    if (pct > 0.01f) {
        int filled = (int)(w * pct);
        if (filled < h) filled = h;  // Minimum for round rect
        _tft.fillRoundRect(x, y, filled, h, h / 2, color);
    }
}

void Display::setBrightness(uint8_t pct) {
    int duty = map(constrain(pct, 0, 100), 0, 100, 0, 255);
    analogWrite(TFT_BL, duty);
}

bool Display::isTouched() {
    uint16_t x, y;
    return _tft.getTouch(&x, &y);
}

void Display::getTouchPoint(uint16_t& x, uint16_t& y) {
    _tft.getTouch(&x, &y);
}

int Display::getTabFromTouch(uint16_t x, uint16_t y) {
    if (y >= TAB_BAR_Y && y < SCREEN_H) {
        int tabW = SCREEN_W / TAB_COUNT;
        return x / tabW;
    }
    return -1;
}

void Display::setLED(bool r, bool g, bool b) {
    // Active LOW
    digitalWrite(LED_RED, r ? LOW : HIGH);
    digitalWrite(LED_GREEN, g ? LOW : HIGH);
    digitalWrite(LED_BLUE, b ? LOW : HIGH);
}

void Display::alertLED(bool on) {
    setLED(on, false, false);
}

String Display::formatBytes(int64_t bytes) {
    if (bytes < 1024) return String((int)bytes) + " B";
    if (bytes < 1048576) return String((float)bytes / 1024.0f, 1) + " KB";
    if (bytes < 1073741824) return String((float)bytes / 1048576.0f, 1) + " MB";
    if (bytes < 1099511627776LL)
        return String((float)bytes / 1073741824.0f, 1) + " GB";
    return String((float)bytes / 1099511627776.0f, 1) + " TB";
}

String Display::formatDuration(unsigned long seconds) {
    if (seconds < 60) return String(seconds) + "s";
    if (seconds < 3600) return String(seconds / 60) + "m " +
                                String(seconds % 60) + "s";
    return String(seconds / 3600) + "h " +
           String((seconds % 3600) / 60) + "m";
}
