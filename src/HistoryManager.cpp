#include "HistoryManager.h"

HistoryManager historyManager;

void HistoryManager::init() {
    _logs.reserve(MAX_ENTRIES);
    _auths.reserve(MAX_ENTRIES);
    _starts.reserve(MAX_ENTRIES);
    _otas.reserve(MAX_ENTRIES);
}

void HistoryManager::addEntry(HistoryType type, const HistoryEntry& entry) {
    auto& vec = (type == HistoryType::LOG)   ? _logs
              : (type == HistoryType::AUTH)  ? _auths
              : (type == HistoryType::START) ? _starts
              : _otas;
    if (vec.size() >= MAX_ENTRIES) {
        vec.erase(vec.begin());  // Drop oldest
    }
    vec.push_back(entry);
}

void HistoryManager::recordAuth(const String& method, const String& id, bool granted) {
    HistoryEntry e;
    e.timestamp = millis();
    e.message = method + (granted ? " GRANTED: " : " DENIED: ") + id;
    e.detail = id;
    e.success = granted;
    addEntry(HistoryType::AUTH, e);
}

void HistoryManager::recordStart(const String& action, bool success, const String& reason) {
    HistoryEntry e;
    e.timestamp = millis();
    e.message = action + (success ? " SUCCESS" : " FAILED");
    if (reason.length() > 0) e.message += " - " + reason;
    e.detail = action;
    e.success = success;
    addEntry(HistoryType::START, e);
}

void HistoryManager::recordOta(const String& version, bool success, const String& detail) {
    HistoryEntry e;
    e.timestamp = millis();
    e.message = "OTA " + version + (success ? " SUCCESS" : " FAILED");
    if (detail.length() > 0) e.message += " - " + detail;
    e.detail = version;
    e.success = success;
    addEntry(HistoryType::OTA, e);
}

void HistoryManager::recordLog(const String& level, const String& message) {
    HistoryEntry e;
    e.timestamp = millis();
    e.message = "[" + level + "] " + message;
    e.detail = level;
    e.success = (level != "ERR" && level != "WARN" && level != "ERROR");
    addEntry(HistoryType::LOG, e);
}

String HistoryManager::getHistoryJson(HistoryType type) const {
    const auto& vec = (type == HistoryType::LOG)   ? _logs
                    : (type == HistoryType::AUTH)  ? _auths
                    : (type == HistoryType::START) ? _starts
                    : _otas;

    String json = "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i > 0) json += ",";
        const auto& e = vec[i];
        json += "{\"ts\":" + String(e.timestamp) + ",";
        json += "\"time\":\"" + formatTime(e.timestamp) + "\",";
        json += "\"msg\":\"" + escapeJson(e.message) + "\",";
        json += "\"detail\":\"" + escapeJson(e.detail) + "\",";
        json += "\"ok\":" + String(e.success ? "true" : "false") + "}";
    }
    json += "]";
    return json;
}

size_t HistoryManager::count(HistoryType type) const {
    const auto& vec = (type == HistoryType::LOG)   ? _logs
                    : (type == HistoryType::AUTH)  ? _auths
                    : (type == HistoryType::START) ? _starts
                    : _otas;
    return vec.size();
}

String HistoryManager::escapeJson(const String& input) {
    String out;
    out.reserve(input.length() + 8);
    for (size_t i = 0; i < input.length(); ++i) {
        char ch = input[i];
        switch (ch) {
            case '\\': out += "\\\\"; break;
            case '"':  out += "\\\""; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += ch;     break;
        }
    }
    return out;
}

String HistoryManager::formatTime(unsigned long ms) {
    unsigned long sec = ms / 1000;
    unsigned long min = sec / 60;
    unsigned long hr = min / 60;
    char buf[16];
    snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", hr % 24, min % 60, sec % 60);
    return String(buf);
}
