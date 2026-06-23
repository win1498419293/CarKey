#pragma once
#include <Arduino.h>
#include <Update.h>

enum class OTAState : uint8_t {
    OTA_IDLE = 0,
    OTA_IN_PROGRESS,
    OTA_COMPLETE_PENDING,
    OTA_VALIDATED,
    OTA_ROLLBACK
};

class OTAManager {
public:
    void init();

    bool begin(size_t firmwareSize = 0);
    bool write(uint8_t* data, size_t len);
    bool end();
    void abort();

    bool isUpdating() const;
    OTAState getState() const;
    int getProgressPercent() const;
    String getLastError() const;

    bool downloadAndApply(const String& url);
    void feedMqttChunk(uint8_t* data, size_t len);
    bool finalizeMqttOta();

    // Rollback / version info
    String getRunningPartitionLabel() const;
    String getStoredPartitionLabel() const;
    String getRunningVersion() const;
    String getStoredVersion() const;
    bool rollback();
    bool hasStoredFirmware() const;

private:
    OTAState _state = OTAState::OTA_IDLE;
    size_t _totalSize = 0;
    size_t _writtenSize = 0;
    String _lastError;
    bool _otaConfirmedOnBoot = false;

    void persistState();
    void loadStateFromNVS();
    void checkBootPartition();
    void persistVersion(const String& partitionLabel, const String& version);
    String loadVersion(const String& partitionLabel) const;
};

extern OTAManager otaManager;

