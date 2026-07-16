#pragma once
#include <Arduino.h>

// ========== StatusJsonBuilder ==========
// Unified JSON status generation.
// All callers (WebSocket broadcast, /api/status, MQTT publish)
// use the same buildStatusString() so new fields only need adding here.
//
// Usage:
//   String json = buildStatusString(BuilderMode::API_OR_WS);
//   webManager.broadcastStatus(json);
//
//   String json = buildStatusString(BuilderMode::CELLULAR_MQTT);
//   sendATCommand("AT+MPUB=...", ...);
//
//   String json = buildStatusString(BuilderMode::WIFI_MQTT);
//   wifiClient.publish("carkey/v5/status", json.c_str());

enum class BuilderMode : uint8_t {
    API_OR_WS,      // /api/status + WebSocket broadcast (all fields)
    CELLULAR_MQTT,  // 4G AT MQTT publish (includes GNSS if available)
    WIFI_MQTT       // WiFi MQTT publish (minimal)
};

// Forward declarations — these externs are expected to exist in the global scope
// (same pattern as cellularManager, networkManager, etc. in main.cpp / TaskManager.cpp)
extern bool webAccessLocked;
extern bool bleScanEnabled;
extern bool authMethodNFC;

// ========== buildBaseStatusString ==========
// Returns a comma-separated key:value fragment (no wrapping braces).
// Shared by /api/status, WebSocket, and both MQTT paths.
String buildBaseStatusString();

// ========== buildStatusString ==========
// Returns a complete JSON status string based on the requested mode.
String buildStatusString(BuilderMode mode);

// ========== buildStatusString (simplified) ==========
// Convenience overload: mode = API_OR_WS
inline String buildStatusString() {
    return buildStatusString(BuilderMode::API_OR_WS);
}
