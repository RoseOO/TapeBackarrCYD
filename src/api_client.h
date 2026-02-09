#pragma once

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "settings.h"

// Dashboard stats from /api/v1/dashboard
struct DashboardData {
    int totalTapes;
    int activeTapes;
    int fullTapes;
    int totalJobs;
    int activeJobs;
    int totalDrives;
    int64_t totalCapacityBytes;
    int64_t usedCapacityBytes;
    bool valid;
};

// Active job info from /api/v1/jobs/active
struct ActiveJobData {
    int id;
    String name;
    String status;         // running, paused, pending
    int64_t filesProcessed;
    int64_t bytesProcessed;
    String startTime;
    bool valid;
};

// Drive info from /api/v1/drives
struct DriveData {
    int id;
    String displayName;
    String status;        // ready, busy, offline, error
    String currentTape;
    String devicePath;
    bool enabled;
    bool valid;
};

// Tape change request
struct TapeChangeData {
    int id;
    String reason;       // tape_full, tape_error
    String status;       // pending, acknowledged, completed
    int currentTapeId;
    bool valid;
};

class APIClient {
public:
    void begin(SettingsManager& settings);

    bool testConnection();
    DashboardData fetchDashboard();
    std::vector<ActiveJobData> fetchActiveJobs();
    std::vector<DriveData> fetchDrives();
    std::vector<TapeChangeData> fetchTapeChanges();

    bool isConnected() const { return _connected; }
    String getLastError() const { return _lastError; }

private:
    SettingsManager* _settings = nullptr;
    bool _connected = false;
    String _lastError;

    String buildURL(const String& path);
    String httpGet(const String& url);
};
