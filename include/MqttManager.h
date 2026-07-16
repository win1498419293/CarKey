#pragma once
#include <Arduino.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include "NetworkManager.h"
#include <vector>

// ========== MqttManager ==========
// WiFi-based MQTT client using PubSubClient
// Falls back gracefully when WiFi is not available
class MqttManager {
public:
    void init(NetworkManager* netMgr);
    void update();

    // Publish status/location/messages
    void publishStatus(const String& msg);
    void publishStatusJson();
    void publishLocation();

    // Connection state
    bool isConnected() const { return _connected; }
    
    // --- HMAC-SHA256 command verification (shared from CellularManager) ---
    bool verifyCommandSignature(const String& jsonPayload);
    void pushDisconnectLog(const String& reason);

private:
    NetworkManager* _netMgr = nullptr;
    WiFiClient _wifiClient;
    PubSubClient _mqttClient;
    
    bool _initialized = false;
    bool _connected = false;
    unsigned long _lastReconnectAttempt = 0;
    unsigned long _reconnectDelayMs = 5000;
    unsigned long _lastStatusPublish = 0;
    unsigned long _lastCsqCheck = 0;   // Not used for WiFi, placeholder
    unsigned long _lastLocationPublish = 0;
    
    static constexpr unsigned long kStatusPublishIntervalMs = 60000;
    static constexpr unsigned long kLocationPublishIntervalMs = 300000;
    static constexpr size_t kDisconnectLogMax = 10;
    
    struct DisconnectRecord {
        unsigned long atMs;
        String reason;
    };
    std::vector<DisconnectRecord> _disconnectLog;
    
    // Nonce cache for command replay protection
    static constexpr size_t kNonceCacheSize = 32;
    String _nonceCache[kNonceCacheSize];
    uint8_t _nonceCacheIndex = 0;
    
    String _pendingPublish;
    unsigned long _lastMqttLoop = 0;
    unsigned long _offlineSinceMs = 0;
    
    static void onMqttMessage(char* topic, byte* payload, unsigned int length);
    static MqttManager* _instance;
    
    bool isNonceReplay(const String& nonce);
    void recordNonce(const String& nonce);
};

extern MqttManager mqttManager;
