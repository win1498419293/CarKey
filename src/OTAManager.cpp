#include "OTAManager.h"
#include "Config.h"
#include "Logger.h"
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <WiFi.h>

OTAManager otaManager;

namespace {
const char* NVS_NS = "ota_state";
const char* KEY_STATE = "state";
const uint8_t MAX_BOOT_ATTEMPTS = 3;
const char* NVS_VERS = "ota_version";
}

void OTAManager::persistState() {
    Preferences p;
    if (p.begin(NVS_NS, false)) {
        p.putUChar(KEY_STATE, static_cast<uint8_t>(_state));
        p.putUInt("total_sz", _totalSize);
        p.end();
    }
}

void OTAManager::loadStateFromNVS() {
    Preferences p;
    if (p.begin(NVS_NS, true)) {
        _state = static_cast<OTAState>(p.getUChar(KEY_STATE, 0));
        _totalSize = p.getUInt("total_sz", 0);
        p.end();
    }
}

void OTAManager::persistVersion(const String& partitionLabel, const String& version) {
    Preferences p;
    if (p.begin(NVS_VERS, false)) {
        p.putString(partitionLabel.c_str(), version);
        p.end();
    }
}

String OTAManager::loadVersion(const String& partitionLabel) const {
    Preferences p;
    if (p.begin(NVS_VERS, true)) {
        String v = p.getString(partitionLabel.c_str(), "");
        p.end();
        return v;
    }
    return "";
}

void OTAManager::checkBootPartition() {
    const esp_partition_t* running = esp_ota_get_running_partition();
    if (!running) {
        Logger::error("[OTA] Cannot determine running partition");
        return;
    }
    Logger::info(String("[OTA] Running partition: ") + running->label + " subtype=" + String(running->subtype));

    // Persist running version on every boot
    String buildStamp = String(kBuildStamp);
    if (buildStamp.length() > 0) {
        persistVersion(running->label, buildStamp);
        Logger::info("[OTA] Saved version for " + String(running->label) + ": " + buildStamp);
    }

    Preferences p;
    if (!p.begin(NVS_NS, true)) {
        Logger::warn("[OTA] Cannot open ota_state NVS for read");
        return;
    }
    uint8_t savedState = p.getUChar(KEY_STATE, 0);
    p.end();

    OTAState prevState = static_cast<OTAState>(savedState);
    if (prevState == OTAState::OTA_COMPLETE_PENDING) {
        const esp_partition_t* bootPartition = esp_ota_get_boot_partition();
        if (bootPartition && bootPartition != running) {
            Logger::error("[OTA] ROLLBACK DETECTED: running on " + String(running->label) + " but boot target was " + String(bootPartition->label));
            _state = OTAState::OTA_ROLLBACK;
            persistState();
            return;
        }
        esp_err_t err = esp_ota_mark_app_valid_cancel_rollback();
        if (err == ESP_OK) {
            Logger::info("[OTA] New firmware validated, rollback cancelled");
            _state = OTAState::OTA_VALIDATED;
            _otaConfirmedOnBoot = true;
            persistState();
        } else {
            Logger::error(String("[OTA] Failed to mark app valid: 0x") + String(err, HEX));
        }
    } else if (prevState == OTAState::OTA_ROLLBACK) {
        Logger::warn("[OTA] Running on fallback partition after rollback");
        _state = OTAState::OTA_IDLE;
        persistState();
    } else {
        esp_ota_mark_app_valid_cancel_rollback();
    }
}

void OTAManager::init() {
    Logger::info("[OTA] Initializing dual-partition OTA manager");
    loadStateFromNVS();
    checkBootPartition();
    Logger::info(String("[OTA] State: ") + static_cast<int>(_state));
}

bool OTAManager::begin(size_t firmwareSize) {
    if (_state == OTAState::OTA_IN_PROGRESS) {
        _lastError = "OTA already in progress";
        Logger::error("[OTA] " + _lastError);
        return false;
    }
    _totalSize = (firmwareSize > 0) ? firmwareSize : UPDATE_SIZE_UNKNOWN;
    _writtenSize = 0;
    _lastError = "";
    if (!Update.begin(_totalSize)) {
        _lastError = String("Update.begin failed: ") + Update.errorString();
        Logger::error("[OTA] " + _lastError);
        Update.printError(Serial);
        return false;
    }
    _state = OTAState::OTA_IN_PROGRESS;
    persistState();
    const esp_partition_t* target = esp_ota_get_next_update_partition(nullptr);
    if (target) {
        Logger::info(String("[OTA] Writing to partition: ") + target->label + " size=" + String(target->size));
        // Save current running version as the "previous" version on the target partition
        const esp_partition_t* running = esp_ota_get_running_partition();
        if (running) {
            String runningLabel = running->label;
            String prevVersion = loadVersion(runningLabel);
            if (prevVersion.length() > 0) {
                persistVersion(target->label, prevVersion);
                Logger::info("[OTA] Saved old version to target partition: " + prevVersion);
            }
        }
    }
    return true;
}

bool OTAManager::write(uint8_t* data, size_t len) {
    if (_state != OTAState::OTA_IN_PROGRESS) {
        _lastError = "OTA not in progress";
        return false;
    }
    if (len == 0) return true;
    size_t written = Update.write(data, len);
    if (written != len) {
        _lastError = String("Write error: ") + Update.errorString();
        Logger::error("[OTA] " + _lastError);
        Update.printError(Serial);
        _state = OTAState::OTA_IDLE;
        persistState();
        return false;
    }
    _writtenSize += written;
    return true;
}

bool OTAManager::end() {
    if (_state != OTAState::OTA_IN_PROGRESS) {
        _lastError = "OTA not in progress";
        return false;
    }
    if (!Update.end(true)) {
        _lastError = String("Update.end failed: ") + Update.errorString();
        Logger::error("[OTA] " + _lastError);
        Update.printError(Serial);
        _state = OTAState::OTA_IDLE;
        persistState();
        return false;
    }
    _state = OTAState::OTA_COMPLETE_PENDING;
    persistState();
    Logger::info(String("[OTA] Firmware written successfully: ") + String(_writtenSize) + " bytes");
    return true;
}

void OTAManager::abort() {
    if (_state == OTAState::OTA_IN_PROGRESS) {
        Update.abort();
        Logger::warn("[OTA] Aborted by user");
    }
    _state = OTAState::OTA_IDLE;
    _writtenSize = 0;
    persistState();
}

bool OTAManager::isUpdating() const {
    return _state == OTAState::OTA_IN_PROGRESS;
}

OTAState OTAManager::getState() const {
    return _state;
}

int OTAManager::getProgressPercent() const {
    if (_totalSize == 0 || _totalSize == UPDATE_SIZE_UNKNOWN) return 0;
    return static_cast<int>((_writtenSize * 100) / _totalSize);
}

String OTAManager::getLastError() const {
    return _lastError;
}

// ------ Rollback / Version ------

String OTAManager::getRunningPartitionLabel() const {
    const esp_partition_t* running = esp_ota_get_running_partition();
    return running ? String(running->label) : "unknown";
}

String OTAManager::getStoredPartitionLabel() const {
    const esp_partition_t* next = esp_ota_get_next_update_partition(nullptr);
    return next ? String(next->label) : "none";
}

String OTAManager::getRunningVersion() const {
    String label = getRunningPartitionLabel();
    return loadVersion(label);
}

String OTAManager::getStoredVersion() const {
    String runningLabel = getRunningPartitionLabel();
    String storedLabel = getStoredPartitionLabel();
    if (storedLabel == "none" || storedLabel == runningLabel) return "";
    return loadVersion(storedLabel);
}

bool OTAManager::hasStoredFirmware() const {
    String storedVer = getStoredVersion();
    return storedVer.length() > 0;
}

bool OTAManager::rollback() {
    const esp_partition_t* running = esp_ota_get_running_partition();
    if (!running) {
        _lastError = "Cannot determine running partition";
        Logger::error("[OTA] " + _lastError);
        return false;
    }
    const esp_partition_t* target = esp_ota_get_next_update_partition(nullptr);
    if (!target) {
        _lastError = "No stored firmware partition found";
        Logger::error("[OTA] " + _lastError);
        return false;
    }
    String targetVersion = loadVersion(target->label);
    if (targetVersion.length() == 0) {
        _lastError = "No firmware version info on stored partition";
        Logger::error("[OTA] " + _lastError);
        return false;
    }
    Logger::info("[OTA] Rolling back from " + String(running->label) + " to " + String(target->label) + " (version: " + targetVersion + ")");
    esp_err_t err = esp_ota_set_boot_partition(target);
    if (err != ESP_OK) {
        _lastError = "esp_ota_set_boot_partition failed: 0x" + String(err, HEX);
        Logger::error("[OTA] " + _lastError);
        return false;
    }
    _state = OTAState::OTA_ROLLBACK;
    persistState();
    Logger::info("[OTA] Rollback successful, rebooting...");
    return true;
}

// ------ OTA Download ------

bool OTAManager::downloadAndApply(const String& url) {
    if (WiFi.status() != WL_CONNECTED) {
        _lastError = "WiFi not connected";
        Logger::error("[OTA] " + _lastError);
        return false;
    }
    Logger::info("[OTA] Downloading firmware from: " + url);
    HTTPClient http;
    http.begin(url);
    http.setTimeout(30000);
    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        _lastError = "HTTP GET failed: " + String(httpCode);
        Logger::error("[OTA] " + _lastError);
        http.end();
        return false;
    }
    size_t contentLength = http.getSize();
    Logger::info("[OTA] Content-Length: " + String(contentLength));
    if (!begin(contentLength)) {
        http.end();
        return false;
    }
    WiFiClient* stream = http.getStreamPtr();
    uint8_t buf[1024];
    size_t remaining = contentLength > 0 ? contentLength : SIZE_MAX;
    while (http.connected() && remaining > 0) {
        size_t toRead = remaining < sizeof(buf) ? remaining : sizeof(buf);
        int len = stream->read(buf, toRead);
        if (len <= 0) break;
        if (!write(buf, len)) {
            http.end();
            return false;
        }
        remaining -= len;
    }
    http.end();
    return end();
}

void OTAManager::feedMqttChunk(uint8_t* data, size_t len) {
    write(data, len);
}

bool OTAManager::finalizeMqttOta() {
    return end();
}

