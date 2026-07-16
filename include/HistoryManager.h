#pragma once
#include <Arduino.h>
#include <vector>

// Circular buffer for operational history records.
// Stores: auth events (NFC/BLE), engine starts, OTA updates, system logs.
// Accessible via /api/history/* endpoints for the Web dashboard.

enum class HistoryType : uint8_t {
    LOG,    // General system log
    AUTH,   // Authentication events (NFC card / BLE device)
    START,  // Engine start/stop records
    OTA     // OTA firmware update records
};

struct HistoryEntry {
    unsigned long timestamp;  // millis() when recorded
    String message;           // Human-readable description
    String detail;            // Extra detail (device name, version, etc.)
    bool success;             // Whether the operation succeeded
};

class HistoryManager {
public:
    static constexpr size_t MAX_ENTRIES = 100;  // Per-type max entries

    void init();

    // Record an event (adds timestamp automatically)
    void recordAuth(const String& method, const String& id, bool granted);
    void recordStart(const String& action, bool success, const String& reason = "");
    void recordOta(const String& version, bool success, const String& detail = "");
    void recordLog(const String& level, const String& message);

    // Get all entries of a given type as JSON array string
    String getHistoryJson(HistoryType type) const;

    size_t count(HistoryType type) const;

private:
    void addEntry(HistoryType type, const HistoryEntry& entry);
    std::vector<HistoryEntry> _logs;
    std::vector<HistoryEntry> _auths;
    std::vector<HistoryEntry> _starts;
    std::vector<HistoryEntry> _otas;

    static String escapeJson(const String& input);
    static String formatTime(unsigned long ms);
};

extern HistoryManager historyManager;
