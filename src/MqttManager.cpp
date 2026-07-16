#include "MqttManager.h"
#include "Logger.h"
#include "Metrics.h"
#include "BatteryVoltage.h"
#include "VehicleStatus.h"
#include "Config.h"
#include <WiFi.h>
#include "NetworkManager.h"
#include "StatusJsonBuilder.h"
#include "TaskManager.h"
#include <mbedtls/md.h>

extern BatteryVoltage batteryVoltage;
extern VehicleStatusManager vehicleStatus;
#if ENABLE_CELLULAR
#include "CellularManager.h"
extern CellularManager cellularManager;
#endif

// ========== MQTT Config ==========
static const char* mqtt_broker_primary   = "broker.emqx.io";
static const char* mqtt_broker_backup    = "test.mosquitto.org";
static const int   mqtt_port             = 1883;
static const char* mqtt_client_id        = "CarKeyV5_Pro_888X";
static const char* topic_sub             = "carkey/v5/cmd";
static const char* topic_pub             = "carkey/v5/status";
static const char* kHmacSecret = "CarKeyV5_Secret_2026";

// ========== HMAC-SHA256 ==========
static String hmacSha256Hex(const String& key, const String& message) {
    byte hmacResult[32];
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t mdType = MBEDTLS_MD_SHA256;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(mdType), 1);
    mbedtls_md_hmac_starts(&ctx, (const unsigned char*)key.c_str(), key.length());
    mbedtls_md_hmac_update(&ctx, (const unsigned char*)message.c_str(), message.length());
    mbedtls_md_hmac_finish(&ctx, hmacResult);
    mbedtls_md_free(&ctx);
    String hex;
    for (int i = 0; i < 32; i++) {
        if (hmacResult[i] < 0x10) hex += "0";
        hex += String(hmacResult[i], HEX);
    }
    hex.toUpperCase();
    return hex;
}

MqttManager* MqttManager::_instance = nullptr;

void MqttManager::init(NetworkManager* netMgr) {
    if (_initialized) return;
    _initialized = true;
    _netMgr = netMgr;
    Logger::info("[MQTT] init WiFi MQTT engine");
    _mqttClient.setClient(_wifiClient);
    _mqttClient.setServer(mqtt_broker_primary, mqtt_port);
    _mqttClient.setCallback(onMqttMessage);
    _mqttClient.setBufferSize(512);
    _instance = this;
}

void MqttManager::update() {
    if (!_initialized) return;
    const unsigned long now = millis();
    
    if (_connected) {
        if (_netMgr && _netMgr->isCellularOnline()) {
            // 4G MQTT: PubSubClient not involved, skip connected check
        } else {
            if (_mqttClient.connected()) {
                _mqttClient.loop();
            }
            if (!_mqttClient.connected()) {
                Logger::warn("[MQTT] disconnected");
                _connected = false;
                _offlineSinceMs = now;
            }
        }
    }
    
    if (!_connected) {
        bool shouldConnect = false;
        const char* via = "";
        
        const bool cellularOnline = _netMgr && _netMgr->isCellularOnline();
        
        if (cellularOnline) {
            // 4G MQTT handled by CellularManager via AT native commands
            // MqttManager tracks state but doesn't use PubSubClient for 4G
            _connected = true;
            _reconnectDelayMs = 5000;
            _offlineSinceMs = 0;
            // Don't attempt WiFi MQTT when 4G is available
            shouldConnect = false;
        } else if (WiFi.status() == WL_CONNECTED) {
            // WiFi as backup: only use if 4G has been offline for >30s
            // This gives 4G time to initialize (CPIN鈫扖EREG鈫?..鈫扢CONNECT 鈮?15-30s)
            const unsigned long offlineDuration = _offlineSinceMs == 0 ? 0 : (millis() - _offlineSinceMs);
            if (offlineDuration >= 30000) {
                shouldConnect = true;
                via = "WiFi";
                _mqttClient.setClient(_wifiClient);
            }
        }
        
        if (shouldConnect) {
            if (_offlineSinceMs == 0) _offlineSinceMs = now;
            if (now - _lastReconnectAttempt >= _reconnectDelayMs) {
                _lastReconnectAttempt = now;
                _reconnectDelayMs = min(_reconnectDelayMs * 2UL, 120000UL);
                Metrics::onMqttReconnectAttempt();
                Logger::info("[MQTT] connecting...");
                if (_mqttClient.connect(mqtt_client_id)) {
                    _mqttClient.subscribe(topic_sub);
                    _connected = true;
                    _reconnectDelayMs = 5000;
                    _offlineSinceMs = 0;
                    Logger::info(String("[MQTT] online (") + via + ")");
                    Metrics::onMqttReconnectResult(true, "ok");
                }
            }
        }
    }
    
    if (_connected && now - _lastStatusPublish >= kStatusPublishIntervalMs) {
        publishStatusJson();
        _lastStatusPublish = now;
    }
}

void MqttManager::publishStatus(const String& msg) {
    if (!_connected) return;
    _mqttClient.publish(topic_pub, msg.c_str());
}

void MqttManager::publishStatusJson() {
    if (!_connected) return;
    // Refresh cellular signal before building status JSON
#if ENABLE_CELLULAR
    cellularManager.getSignalQuality();
#endif
    String json = "{" + buildBaseStatusString();
    json += ",\"uptime\":" + String(millis() / 1000);
    json += "}";
    Logger::info("[MQTT] published status via WiFi");
    _mqttClient.publish(topic_pub, json.c_str());
}

void MqttManager::publishLocation() {
    if (!_connected) return;
    String json = "{\"type\":\"location\",\"available\":false}";
    Logger::info("[MQTT] published status via WiFi");
    _mqttClient.publish(topic_pub, json.c_str());
}

void MqttManager::pushDisconnectLog(const String& reason) {
    if (_disconnectLog.size() >= kDisconnectLogMax) {
        _disconnectLog.erase(_disconnectLog.begin());
    }
    _disconnectLog.push_back({millis(), reason});
}

void MqttManager::onMqttMessage(char* topic, byte* payload, unsigned int length) {
    if (!_instance) return;
    String msg;
    for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
    Logger::info("[MQTT] msg on " + String(topic) + ": " + msg);
    
    if (msg.startsWith("{") && msg.endsWith("}")) {
        if (!_instance->verifyCommandSignature(msg)) {
            Logger::error("[MQTT] command signature verification failed");
            _instance->publishStatus("ERROR_SIGNATURE");
            return;
        }
        int cs = msg.indexOf("\"cmd\":\"");
        if (cs == -1) return;
        cs += 7;
        int ce = msg.indexOf("\"", cs);
        msg = msg.substring(cs, ce);
    }
    if (!msg.startsWith("START")) return;
    String pwd = msg.substring(6);
    pwd.trim();
    if (pwd != start_pwd) {
        Logger::error("[MQTT] wrong start password");
        _instance->publishStatus("ERROR_WRONG_PWD");
        _instance->pushDisconnectLog("wrong start password");
        return;
    }
    if (batteryVoltage.isLowForRemoteStart()) {
        Logger::error("[MQTT] start denied low battery (" + String(batteryVoltage.getVoltage(), 2) + "V)");
        _instance->publishStatus("ERROR_LOW_BATTERY");
        return;
    }
    if (TaskManager::sendVehicleCommand(VehicleCommandType::VEHICLE_CMD_START_FROM_MQTT, 20)) {
        _instance->publishStatus("ENGINE_START_ACCEPTED");
    } else {
        _instance->publishStatus("ERROR_QUEUE_BUSY");
    }
}

// ========== Command Signature Verification ==========
bool MqttManager::verifyCommandSignature(const String& jsonPayload) {
    int cmdS = jsonPayload.indexOf("\"cmd\":\"");
    if (cmdS == -1) return false;
    cmdS += 7;
    int cmdE = jsonPayload.indexOf("\"", cmdS);
    String cmd = jsonPayload.substring(cmdS, cmdE);
    
    int tsS = jsonPayload.indexOf("\"ts\":");
    if (tsS == -1) return false;
    tsS += 5;
    int tsE = jsonPayload.indexOf(",", tsS);
    if (tsE == -1) tsE = jsonPayload.indexOf("}", tsS);
    String tsStr = jsonPayload.substring(tsS, tsE);
    tsStr.trim();
    unsigned long ts = (unsigned long)tsStr.toInt();
    
    int nonceS = jsonPayload.indexOf("\"nonce\":\"");
    if (nonceS == -1) return false;
    nonceS += 9;
    int nonceE = jsonPayload.indexOf("\"", nonceS);
    String nonce = jsonPayload.substring(nonceS, nonceE);
    
    int signS = jsonPayload.indexOf("\"sign\":\"");
    if (signS == -1) return false;
    signS += 8;
    int signE = jsonPayload.indexOf("\"", signS);
    String signReceived = jsonPayload.substring(signS, signE);
    
    unsigned long now = millis() / 1000;
    if (ts > now + 30) {
        Logger::warn("[MQTT] cmd ts too far in future");
        pushDisconnectLog("cmd rejected: ts future " + String(ts));
        Metrics::onCommandReceived(false);
        return false;
    }
    if (ts < now - 86400UL * 7) {
        Logger::warn("[MQTT] cmd ts too old");
        pushDisconnectLog("cmd rejected: ts expired " + String(ts));
        Metrics::onCommandReceived(false);
        return false;
    }
    if (isNonceReplay(nonce)) {
        Logger::warn("[MQTT] nonce replay detected");
        pushDisconnectLog("cmd rejected: nonce replay " + nonce);
        Metrics::onCommandReceived(false);
        return false;
    }
    
    String signBase = "cmd=" + cmd + "&ts=" + tsStr + "&nonce=" + nonce;
    String signExpected = hmacSha256Hex(kHmacSecret, signBase);
    if (signReceived != signExpected) {
        Logger::warn("[MQTT] cmd signature mismatch");
        Metrics::onCommandReceived(false);
        pushDisconnectLog("cmd rejected: signature mismatch");
        return false;
    }
    recordNonce(nonce);
    Metrics::onCommandReceived(true);
    Logger::info("[MQTT] cmd verified: " + cmd);
    return true;
}

bool MqttManager::isNonceReplay(const String& nonce) {
    for (size_t i = 0; i < kNonceCacheSize; i++) {
        if (_nonceCache[i] == nonce) return true;
    }
    return false;
}

void MqttManager::recordNonce(const String& nonce) {
    _nonceCache[_nonceCacheIndex] = nonce;
    _nonceCacheIndex = (_nonceCacheIndex + 1) % kNonceCacheSize;
}
