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
    String phase;          // initializing, scanning, streaming, cataloging, completed, failed, cancelled
    String status;         // running, paused, cancelled
    int64_t fileCount;
    int64_t totalFiles;
    int64_t totalBytes;
    int64_t bytesWritten;
    double writeSpeed;     // bytes per second
    String tapeLabel;
    int64_t tapeCapacityBytes;
    int64_t tapeUsedBytes;
    double estimatedSecondsRemaining;
    String startTime;
    // Scan progress fields
    int64_t scanFilesFound;
    int64_t scanDirsScanned;
    int64_t scanBytesFound;
    bool valid;
};

// Drive info from /api/v1/drives
struct DriveData {
    int id;
    String displayName;
    String vendor;
    String model;
    String status;        // ready, busy, offline, error
    String currentTape;
    String formatType;    // raw, ltfs
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

// LTFS format progress from /api/v1/ltfs/format/status
struct LTFSFormatStatus {
    bool active;
    String phase;        // formatting, verifying, mounting, labeling, finalizing
    String devicePath;
    int progressPct;
    unsigned long elapsedSec;
    String error;
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
    LTFSFormatStatus fetchLTFSFormatStatus();

    bool isConnected() const { return _connected; }
    String getLastError() const { return _lastError; }

private:
    SettingsManager* _settings = nullptr;
    bool _connected = false;
    String _lastError;

    String buildURL(const String& path);
    String httpGet(const String& url);
};
