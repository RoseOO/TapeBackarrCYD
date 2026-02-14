#include "api_client.h"
#include <WiFi.h>

void APIClient::begin(SettingsManager& settings) {
    _settings = &settings;
}

String APIClient::buildURL(const String& path) {
    String protocol = _settings->get().useHTTPS ? "https" : "http";
    return protocol + "://" + _settings->get().serverHost + ":" +
           String(_settings->get().serverPort) + path;
}

String APIClient::httpGet(const String& url) {
    HTTPClient http;
    http.begin(url);
    http.addHeader("X-API-Key", _settings->get().apiKey);
    http.addHeader("Accept", "application/json");
    http.setTimeout(3000);

    int httpCode = http.GET();
    String payload;

    if (httpCode == HTTP_CODE_OK) {
        payload = http.getString();
        _connected = true;
        _lastError = "";
    } else if (httpCode > 0) {
        _lastError = "HTTP " + String(httpCode);
        _connected = false;
    } else {
        _lastError = http.errorToString(httpCode);
        _connected = false;
    }

    http.end();
    return payload;
}

bool APIClient::testConnection() {
    String url = buildURL("/api/v1/health");
    String resp = httpGet(url);
    return _connected;
}

DashboardData APIClient::fetchDashboard() {
    DashboardData data = {};
    data.valid = false;

    String url = buildURL("/api/v1/dashboard");
    String resp = httpGet(url);
    if (resp.isEmpty()) return data;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, resp);
    if (err) {
        _lastError = "JSON: " + String(err.c_str());
        return data;
    }

    data.totalTapes        = doc["total_tapes"] | 0;
    data.activeTapes       = doc["active_tapes"] | 0;
    data.fullTapes         = 0;  // Not provided by API
    data.totalJobs         = doc["total_jobs"] | 0;
    data.activeJobs        = doc["running_jobs"] | 0;
    data.totalDrives       = doc["drive_status"].is<const char*>() ? 1 : 0;

    // Sum capacity across all pools
    data.totalCapacityBytes = 0;
    data.usedCapacityBytes  = 0;
    JsonArray pools = doc["pool_storage"].as<JsonArray>();
    for (JsonObject pool : pools) {
        data.totalCapacityBytes += pool["total_capacity_bytes"] | (int64_t)0;
        data.usedCapacityBytes  += pool["total_used_bytes"] | (int64_t)0;
    }
    data.valid = true;

    return data;
}

std::vector<ActiveJobData> APIClient::fetchActiveJobs() {
    std::vector<ActiveJobData> jobs;

    String url = buildURL("/api/v1/jobs/active");
    String resp = httpGet(url);
    if (resp.isEmpty()) return jobs;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, resp);
    if (err) {
        _lastError = "JSON: " + String(err.c_str());
        return jobs;
    }

    JsonArray arr = doc.as<JsonArray>();
    for (JsonObject obj : arr) {
        ActiveJobData job;
        job.id             = obj["job_id"] | 0;
        job.name           = obj["job_name"] | "Unknown";
        job.phase          = obj["phase"] | "";
        job.status         = obj["status"] | "unknown";
        job.fileCount      = obj["file_count"] | (int64_t)0;
        job.totalFiles     = obj["total_files"] | (int64_t)0;
        job.totalBytes     = obj["total_bytes"] | (int64_t)0;
        job.bytesWritten   = obj["bytes_written"] | (int64_t)0;
        job.writeSpeed     = obj["write_speed"] | 0.0;
        job.tapeLabel      = obj["tape_label"] | "";
        job.tapeCapacityBytes = obj["tape_capacity_bytes"] | (int64_t)0;
        job.tapeUsedBytes  = obj["tape_used_bytes"] | (int64_t)0;
        job.estimatedSecondsRemaining = obj["estimated_seconds_remaining"] | 0.0;
        job.tapeEstimatedSecondsRemaining = obj["tape_estimated_seconds_remaining"] | 0.0;
        job.startTime      = obj["start_time"] | "";
        job.scanFilesFound = obj["scan_files_found"] | (int64_t)0;
        job.scanDirsScanned = obj["scan_dirs_scanned"] | (int64_t)0;
        job.scanBytesFound = obj["scan_bytes_found"] | (int64_t)0;
        job.valid = true;
        jobs.push_back(job);
    }

    return jobs;
}

std::vector<DriveData> APIClient::fetchDrives() {
    std::vector<DriveData> drives;

    String url = buildURL("/api/v1/drives");
    String resp = httpGet(url);
    if (resp.isEmpty()) return drives;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, resp);
    if (err) {
        _lastError = "JSON: " + String(err.c_str());
        return drives;
    }

    JsonArray arr = doc.as<JsonArray>();
    for (JsonObject obj : arr) {
        DriveData drive;
        drive.id          = obj["id"] | 0;
        drive.displayName = obj["display_name"] | "Unknown";
        drive.vendor      = obj["vendor"] | "";
        drive.model       = obj["model"] | "";
        drive.status      = obj["status"] | "unknown";
        drive.currentTape = obj["current_tape"] | "None";
        drive.formatType  = obj["format_type"] | "";
        drive.devicePath  = obj["device_path"] | "";
        drive.enabled     = obj["enabled"] | false;
        drive.valid = true;
        drives.push_back(drive);
    }

    return drives;
}

std::vector<TapeChangeData> APIClient::fetchTapeChanges() {
    std::vector<TapeChangeData> changes;

    // Fetch active jobs and check for tape change needs via events
    String url = buildURL("/api/v1/events");
    String resp = httpGet(url);
    if (resp.isEmpty()) return changes;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, resp);
    if (err) return changes;

    JsonArray arr = doc.as<JsonArray>();
    for (JsonObject obj : arr) {
        String type = obj["type"] | "";
        if (type == "tape_change_required" || type == "tape_full") {
            TapeChangeData change;
            change.id            = obj["id"] | 0;
            change.reason        = obj["type"] | "";
            change.status        = obj["status"] | "pending";
            change.currentTapeId = obj["tape_id"] | 0;
            change.valid = true;
            changes.push_back(change);
        }
    }

    return changes;
}

LTFSFormatStatus APIClient::fetchLTFSFormatStatus() {
    LTFSFormatStatus status = {};
    status.valid = false;

    String url = buildURL("/api/v1/ltfs/format/status");
    String resp = httpGet(url);
    if (resp.isEmpty()) return status;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, resp);
    if (err) {
        _lastError = "JSON: " + String(err.c_str());
        return status;
    }

    status.active     = doc["active"] | false;
    status.phase      = doc["phase"] | "";
    status.devicePath = doc["device_path"] | "";
    status.progressPct = doc["progress_pct"] | 0;
    status.elapsedSec  = doc["elapsed_seconds"] | (unsigned long)0;
    status.error       = doc["error"] | "";
    status.valid = true;

    return status;
}
