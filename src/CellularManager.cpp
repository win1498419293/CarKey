#include "Config.h"
#if ENABLE_CELLULAR
#include "CellularManager.h"

#include "BatteryVoltage.h"
#include "Config.h"
#include "Logger.h"
#include "Metrics.h"
#include "TaskManager.h"
#include "VehicleStatus.h"
#include "NetworkManager.h"
#include "StatusJsonBuilder.h"

extern BatteryVoltage batteryVoltage;
extern VehicleStatusManager vehicleStatus;
CellularManager cellularManager;

#define RXD2 16
#define TXD2 17
#define PWRKEY_PIN 4      // GPIO4 controls Air780EP PWRKEY (active low 1s)

// ========== HMAC-SHA256 ==========
#include <mbedtls/md.h>

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

// ========== ???????????? ==========
static const char* mqtt_broker_primary   = "broker.emqx.io";
static const char* mqtt_broker_primary_ip = "35.172.255.228";
static const char* mqtt_broker_backup    = "test.mosquitto.org";
static const int   mqtt_port             = 1883;
static const char* mqtt_client_id        = "CarKeyV5_4G_888X";
static const char* topic_sub             = "carkey/v5/cmd";
static const char* topic_pub             = "carkey/v5/status";
static const char* kHmacSecret = "CarKeyV5_Secret_2026";

// ========== AT ???????????? ==========
bool CellularManager::sendATCommand(String cmd, String expectedResponse, unsigned long timeout) {
    return sendATCommandLine(cmd, expectedResponse, timeout, false);
}

String CellularManager::sendATCommandRaw(String cmd, unsigned long timeout) {
    String result;
    if (sendATCommandLine(cmd, "", timeout, true)) {
        result = _tcpReply;
    }
    _tcpReply = "";
    return result;
}

// ========== ????????? ==========
void CellularManager::init() {
    if (_initialized) return;
    _initialized = true;
    Logger::info("[Cellular] init 4G MQTT engine");
    
    Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
    delay(500);
    while (Serial2.available()) { Serial2.read(); delay(5); }
    
    mqttState = 0;
    useBackupBroker = false;
    reconnectDelayMs = 5000;
    nextReconnectAt = millis();
    offlineSinceMs = millis();
    modemReady = false;
    _networkRegistered = false;
    modemInitAttempts = 0;
    nextModemProbeAt = millis() + 3000;
    lowPowerMode = false;
    lastHeartbeat = 0;
    heartbeatIntervalMs = 60000;
    _connStep = MqttConnStep::POWER_ON;
    _connStepStartMs = millis();
    _connTimeoutMs = 60000;  // generous first-boot timeout
}

// ========== AT line-level communication ==========
// Read one complete line (terminated by \n) from Serial2
// Returns empty string on timeout
String CellularManager::readATLine(unsigned long timeoutMs, bool logRaw) {
    const unsigned long deadline = millis() + timeoutMs;
    while (millis() < deadline) {
        while (Serial2.available()) {
            char c = static_cast<char>(Serial2.read());
            if (c == '\r') continue;
            if (c == '\n') {
                // Complete line received
                _atLineBuf.trim();
                String line = _atLineBuf;
                _atLineBuf = "";
                _atLineInProgress = false;
                // Log every raw line if asked
                if (logRaw && line.length() > 0) {
                    Logger::info("[AT LINE] " + line);
                }
                return line;
            }
            _atLineBuf += c;
            _atLineInProgress = true;
        }
        delay(1);
    }
    // Timeout: save partial buffer
    if (_atLineBuf.length() > 0) {
        if (logRaw) {
            Logger::info("[AT LINE] (partial) " + _atLineBuf);
        }
    }
    return "";
}

// Send AT command and wait for expected response (line-based parsing)
// Returns true if expectedResponse is found in any response line
// Filters out: command echo, RDY, ^boot, ^ready, etc.
bool CellularManager::sendATCommandLine(String cmd, String expectedResponse, unsigned long timeout, bool logRaw) {
    // Flush residual serial data
    while (Serial2.available()) { Serial2.read(); delay(2); }
    _atLineBuf = "";
    _atLineInProgress = false;
    
    Logger::debug("[AT->] " + cmd);
    Serial2.println(cmd);
    
    const unsigned long deadline = millis() + timeout;
    bool foundExpected = false;
    String fullLog;
    
    while (millis() < deadline) {
        String line = readATLine(200, false);  // Read one line, 200ms per attempt (19200 baud)
        if (line.length() == 0) continue;  // Timeout on this attempt, keep polling
        
        // === Filter out non-response lines ===
        // 1. Command echo (the AT command itself echoed back)
        if (line.equals(cmd)) {
            if (logRaw) Logger::info("[AT ECHO] " + line);
            continue;
        }
        // 2. Startup banners
        if (line.indexOf("RDY") >= 0 || line.indexOf("^boot") >= 0 || line.indexOf("^ready") >= 0 || line.indexOf("Call Ready") >= 0) {
            if (logRaw) Logger::info("[AT BOOT] " + line);
            continue;
        }
        // 3. Empty lines
        if (line.length() == 0) continue;
        
        // This is a real response line - log it
        if (logRaw) {
            Logger::info("[AT LINE] " + line);
        }
        
        // Check for expected response
        if (expectedResponse.length() > 0 && line.indexOf(expectedResponse) >= 0) {
            Logger::debug("[AT<- OK] " + line);
            _tcpReply = line;
            foundExpected = true;
            // Continue reading to drain remaining lines within timeout
            continue;
        }
        
        // Check for ERROR (unless we are expecting ERROR)
        if (expectedResponse != "ERROR" && line.indexOf("ERROR") >= 0) {
            Logger::debug("[AT<- ERR] " + line);
            _tcpReply = line;
            foundExpected = false;
            // Continue draining
            continue;
        }
        
        // Store last non-filtered line for raw reply
        _tcpReply = line;
    }
    
    // Drain any remaining response lines
    _atLineBuf = "";
    _atLineInProgress = false;
    
    if (!foundExpected && expectedResponse.length() > 0) {
        Logger::warn("[AT<- TIMEOUT] cmd=" + cmd + " expected=" + expectedResponse + " last=" + _tcpReply);
    }
    
    return foundExpected;
}


// ========== Power Control ==========
void CellularManager::pulsePwrkey() {
    // Air780EP auto-powers-on, no PWRKEY pulse needed
    // Just wait for module to boot and flush boot messages
    Logger::info("[Cellular] waiting for module to power up...");
    delay(1000);
    int flushed = 0;
    unsigned long flushStart = millis();
    while (Serial2.available() && (millis() - flushStart < 2000)) {
        Serial2.read();
        flushed++;
        delay(2);
    }
    Logger::info("[Cellular] flushed " + String(flushed) + " bytes from Serial2");
    Logger::info("[Cellular] module should be ready");
}
void CellularManager::powerOn() {
    Logger::info("[Cellular] power on module");
    pulsePwrkey();
    _connStep = MqttConnStep::POWER_WAIT;
    _connStepStartMs = millis();
    _connTimeoutMs = 5000;
}

void CellularManager::powerOff() {
    Logger::info("[Cellular] power off module");
    digitalWrite(PWRKEY_PIN, LOW);
    delay(2000);
    digitalWrite(PWRKEY_PIN, HIGH);
}

void CellularManager::updateModemInit() {
    if (modemReady) return;
    
    const unsigned long now = millis();
    
    // Unified state machine using _connStep (no separate static variables)
    // Steps: POWER_ON -> pulsePwrkey -> POWER_WAIT -> AT_PROBE -> ATE0_SET -> modemReady
    
    switch (_connStep) {
        case MqttConnStep::POWER_ON: {
            // Module auto-powers-on, just flush boot messages and probe
            pulsePwrkey();
            _connStep = MqttConnStep::POWER_WAIT;
            _connStepStartMs = now;
            _connTimeoutMs = 3000;
            return;
        }
        
                case MqttConnStep::POWER_WAIT: {
            // Wait for module to stabilize, then probe AT at 115200
            if (now - _connStepStartMs >= _connTimeoutMs) {
                _connStep = MqttConnStep::AT_PROBE;
                _connStepStartMs = now;
                _connTimeoutMs = 5000;
                _connReply = "";
                Logger::debug("[AT->] AT");
                Serial2.println("AT");
            }
            return;
        }
        
        case MqttConnStep::AT_PROBE: {
            if (_connReply.length() > 0) {
                String line = _connReply;
                _connReply = "";
                if (line.indexOf("OK") >= 0) {
                    // Found working baud rate!
                    Logger::info("[Cellular] AT OK at current baud rate");
                    _connStep = MqttConnStep::ATE0_SET;
                    _connStepStartMs = now;
                    _connTimeoutMs = 3000;
                    _connReply = "";
                    Logger::debug("[AT->] ATE0");
                    Serial2.println("ATE0");
                    return;
                }
                // Got something but not OK - might be garbage at wrong baud
                if (line.length() > 0 && line.indexOf("AT") < 0 && line.indexOf("RDY") < 0) {
                    Logger::warn("[Cellular] garbled response at current baud: '" + line + "'");
                }
            }
            if (now - _connStepStartMs >= _connTimeoutMs) {
                modemInitAttempts++;
                Logger::warn("[Cellular] modem not responding at baud " + String(Serial2.baudRate()) + ", retry #" + String(modemInitAttempts));
                // Switch to POWER_WAIT which will try next baud rate
                _connStep = MqttConnStep::POWER_WAIT;
                _connStepStartMs = now;  // Immediately trigger baud scan in POWER_WAIT
                _connTimeoutMs = 500;
            }
            return;
        }
        
        case MqttConnStep::ATE0_SET: {
            if (_connReply.length() > 0) {
                String line = _connReply;
                _connReply = "";
                if (line.indexOf("OK") >= 0) {
                    modemReady = true;
                    mqttState = 0;
                    reconnectDelayMs = 5000;
                    nextReconnectAt = now;
                    offlineSinceMs = now;
                    Logger::info("[Cellular] modem ready");
                    _connStep = MqttConnStep::IDLE;
                    return;
                }
            }
            if (now - _connStepStartMs >= _connTimeoutMs) {
                modemReady = true;
                mqttState = 0;
                reconnectDelayMs = 5000;
                nextReconnectAt = now;
                offlineSinceMs = now;
                Logger::info("[Cellular] modem ready (ATE0 assumed)");
                _connStep = MqttConnStep::IDLE;
            }
            return;
        }
        
        default:
            // Unexpected state, reset to POWER_ON
            _connStep = MqttConnStep::POWER_ON;
            _connStepStartMs = now;
            _connTimeoutMs = 60000;
            break;
    }
}


// ========== ???????????? ==========
void CellularManager::recoverConnection(CellularRecoveryLevel level) {
    switch (level) {
        case CellularRecoveryLevel::MQTT_ONLY:
            Logger::info("[Cellular] recover MQTT only");
            Metrics::onCellularRecovery("mqtt");
            mqttState = 0;
            _connStep = MqttConnStep::IDLE;
            reconnectDelayMs = 5000;
            nextReconnectAt = millis() + 1000;
            break;
        case CellularRecoveryLevel::PDP_RESTART:
            Logger::info("[Cellular] recover PDP context (async)");
            Metrics::onCellularRecovery("pdp");
            mqttState = 0;
            // Don't do blocking sendATCommand - restart async connect from CGDCONT
            _connStep = MqttConnStep::CGDCONT_SET;
            _connStepStartMs = millis();
            _connTimeoutMs = 15000;
            _connReply = "";
            while (Serial2.available()) { Serial2.read(); delay(1); }
            Serial2.println("AT+CGDCONT=1,\"IP\",\"CMNET\"");
            reconnectDelayMs = 5000;
            nextReconnectAt = millis() + reconnectDelayMs;
            break;
        case CellularRecoveryLevel::CFUN_RESTART:
            Logger::warn("[Cellular] recover CFUN restart (async)");
            Metrics::onCellularRecovery("cfun");
            mqttState = 0;
            modemReady = false;
            // Non-blocking: just restart from POWER_ON
            _connStep = MqttConnStep::POWER_ON;
            _connStepStartMs = millis();
            _connTimeoutMs = 60000;
            reconnectDelayMs = 5000;
            nextReconnectAt = millis() + reconnectDelayMs;
            useBackupBroker = false;
            offlineSinceMs = millis();
            break;
    }
}

void CellularManager::scheduleNextReconnect() {
    nextReconnectAt = millis() + reconnectDelayMs;
    reconnectDelayMs = min(reconnectDelayMs * 2UL, 120000UL);
    useBackupBroker = !useBackupBroker;
}

void CellularManager::pushDisconnectLog(const String& reason) {
    DisconnectRecord rec;
    rec.atMs = millis();
    rec.reason = reason;
    disconnectLog.push_back(rec);
    if (disconnectLog.size() > kDisconnectLogMax) {
        disconnectLog.erase(disconnectLog.begin());
    }
}

void CellularManager::startAsyncConnect() {
    _connStep = MqttConnStep::CPIN_CHECK;
    _connStepStartMs = millis();
    _connTimeoutMs = 15000;
    _connUseBackup = useBackupBroker;
    _connActiveBroker = _connUseBackup ? mqtt_broker_backup : mqtt_broker_primary;
    _connMqttPort = mqtt_port;
    mqttState = 0;
    while (Serial2.available()) { Serial2.read(); delay(2); }
    _connReply = "";
    Logger::info(String("[Cellular] async connect via ") + _connActiveBroker);
    Serial2.println("AT+CPIN?");
}

void CellularManager::updateConnection() {
    // Serial2 draining done by update() - do NOT read Serial2 here!
    if (_connStep == MqttConnStep::IDLE || _connStep == MqttConnStep::PUBSUB_ONLINE || _connStep == MqttConnStep::WAIT_RETRY) {
        return;
    }
    const unsigned long elapsed = millis() - _connStepStartMs;
    if (elapsed >= _connTimeoutMs) {
        Logger::warn("[Cellular] conn step timeout, retry later");
        _connStep = MqttConnStep::WAIT_RETRY;
        scheduleNextReconnect();
        return;
    }
    // process _connReply if fed by update() drain loop
    if (_connReply.length() > 0) {
        processConnReply();
    }
}

bool CellularManager::processConnReply() {
    const String& reply = _connReply;
    switch (_connStep) {
        case MqttConnStep::CPIN_CHECK:
            if (reply.indexOf("+CPIN: READY") >= 0) {
                _connStep = MqttConnStep::CGDCONT_SET;
                _connStepStartMs = millis(); _connTimeoutMs = 3000; _connReply = "";
                Serial2.println("AT+CGDCONT=1,\"IP\",\"CMNET\"");
                return true;
            }
            if (reply.indexOf("+CPIN:") >= 0 && reply.indexOf("READY") < 0) {
                Logger::warn("[Cellular] SIM not ready: " + reply);
                _connStep = MqttConnStep::WAIT_RETRY;
                scheduleNextReconnect();
                return true;
            }
            break;

        case MqttConnStep::CGDCONT_SET:
            if (reply.indexOf("OK") >= 0) {
                _connStep = MqttConnStep::CEREG_CHECK;
                _connStepStartMs = millis(); _connTimeoutMs = 10000; _connReply = "";
                Serial2.println("AT+CEREG?");
                return true;
            }
            break;

        case MqttConnStep::CEREG_CHECK: {
            int stat = 0;
            if ((reply.indexOf("+CEREG:") >= 0 || reply.indexOf("+CGREG:") >= 0 || reply.indexOf("+CREG:") >= 0) &&
                (sscanf(reply.c_str(), "+CEREG: %*d,%d", &stat) >= 1 ||
                 sscanf(reply.c_str(), "+CGREG: %*d,%d", &stat) >= 1 ||
                 sscanf(reply.c_str(), "+CREG: %*d,%d", &stat) >= 1)) {
                if (stat == 1 || stat == 5) {
                    _connStep = MqttConnStep::CGATT_CHECK;
                    _connStepStartMs = millis(); _connTimeoutMs = 3000; _connReply = "";
                    Serial2.println("AT+CGATT?");
                    return true;
                }
                if (stat == 2) {
                    Logger::info("[Cellular] still searching network...");
                    _connStepStartMs = millis();
                    return true;
                }
                Logger::warn("[Cellular] not registered (stat=" + String(stat) + ")");
                _connStep = MqttConnStep::WAIT_RETRY;
                scheduleNextReconnect();
                return true;
            }
            break;
        }

        case MqttConnStep::CGATT_CHECK:
            if (reply.indexOf("+CGATT: 1") >= 0) {
                _connStep = MqttConnStep::CGACT_CHECK;
                _connStepStartMs = millis(); _connTimeoutMs = 3000; _connReply = "";
                Serial2.println("AT+CGACT?");
                return true;
            }
            break;

        case MqttConnStep::CGACT_CHECK:
            if (reply.indexOf("+CGACT:") >= 0 && (reply.indexOf("1,1") >= 0 || reply.indexOf("0,1") >= 0)) {
                _connStep = MqttConnStep::MCONFIG_SET;
                _connStepStartMs = millis(); _connTimeoutMs = 3000; _connReply = "";
                Logger::info("[Cellular] PDP ready, configuring MQTT...");
                String cfgCmd = "AT+MCONFIG=\"" + String(mqtt_client_id) + "\"";
                Serial2.println(cfgCmd);
                return true;
            }
            break;

        case MqttConnStep::MCONFIG_SET:
            if (reply.indexOf("OK") >= 0) {
                _connStep = MqttConnStep::MIPSTART_CONNECT;
                _connStepStartMs = millis(); _connTimeoutMs = 30000; _connReply = "";
                const char* brokerIp = mqtt_broker_primary_ip;
                String ipCmd = "AT+MIPSTART=\"" + String(brokerIp) + "\"," + String(mqtt_port);
                Logger::info(String("[Cellular] MIPSTART to ") + brokerIp + ":" + mqtt_port);
                // Disconnect any stale TCP first, then connect to broker
                Serial2.println("AT+MDISCONNECT");
                delay(100);
                Serial2.println(ipCmd);
                return true;
            }
            break;

        case MqttConnStep::MIPSTART_CONNECT:
            if (reply.indexOf("CONNECT OK") >= 0) {
                _connStep = MqttConnStep::MCONNECT_SESSION;
                _connStepStartMs = millis(); _connTimeoutMs = 10000; _connReply = "";
                Logger::info("[Cellular] TCP connected, sending MCONNECT...");
                Serial2.println("AT+MCONNECT=1,120");
                return true;
            }
            if (reply.indexOf("ALREADY CONNECT") >= 0) {
                Logger::warn("[Cellular] stale TCP, will retry on next cycle");
                _connStep = MqttConnStep::WAIT_RETRY;
                scheduleNextReconnect();
                return true;
            }
            if (reply.indexOf("CONNECT FAIL") >= 0) {
                Logger::warn("[Cellular] MIPSTART failed: CONNECT FAIL");
                _connStep = MqttConnStep::WAIT_RETRY;
                scheduleNextReconnect();
                return true;
            }
            break;

        case MqttConnStep::MCONNECT_SESSION:
            if (reply.indexOf("CONNACK OK") >= 0) {
                _connStep = MqttConnStep::MSUBSCRIBE;
                _connStepStartMs = millis(); _connTimeoutMs = 5000; _connReply = "";
                Logger::info("[Cellular] MQTT session established, subscribing...");
                String subCmd = "AT+MSUB=\"" + String(topic_sub) + "\",0";
                Serial2.println(subCmd);
                return true;
            }
            if (reply.indexOf("+CME ERROR") >= 0) {
                Logger::warn("[Cellular] MCONNECT failed: " + reply);
                _connStep = MqttConnStep::WAIT_RETRY;
                scheduleNextReconnect();
                return true;
            }
            break;

        case MqttConnStep::MSUBSCRIBE:
            if (reply.indexOf("SUBACK") >= 0 || reply.indexOf("+MSUB:") >= 0) {
                _connStep = MqttConnStep::PUBSUB_ONLINE;
                _connReply = "";
                mqttState = 1;
                reconnectDelayMs = 5000;
                nextReconnectAt = millis() + reconnectDelayMs;
                Logger::info("[Cellular] MQTT online (AT native)");
                Metrics::onMqttReconnectResult(true, "");
                return true;
            }
            break;

        case MqttConnStep::PUBSUB_ONLINE:
            return false;

        default:
            return false;
    }
    return false;
}

void CellularManager::enterLowPowerMode() {
    if (lowPowerMode) return;
    lowPowerMode = true;
    heartbeatIntervalMs = 300000; // 5 min
    Logger::info("[Cellular] enter low power mode, heartbeat=300s");
    sendATCommand("AT+CFUN=0", "OK", 10000);
}

void CellularManager::exitLowPowerMode() {
    if (!lowPowerMode) return;
    lowPowerMode = false;
    heartbeatIntervalMs = 60000;
    Logger::info("[Cellular] exit low power mode, heartbeat=60s");
    sendATCommand("AT+CFUN=1", "OK", 15000);
}


void CellularManager::recoverModem() {
    Logger::warn("[Cellular] offline 10min, recovering modem via CFUN");
    pushDisconnectLog("offline 10min, CFUN restart");
    recoverConnection(CellularRecoveryLevel::CFUN_RESTART);
}

// ========== P0: getSignalQuality ==========
CellularSignal CellularManager::getSignalQuality() {
    // Direct serial read to avoid _tcpReply being overwritten by trailing "OK" lines
    int rssi = 0, ber = 99;
    Serial2.println("AT+CSQ");
    Logger::debug("[AT->] AT+CSQ");
    unsigned long deadline = millis() + 2000;
    String line;
    while (millis() < deadline) {
        if (Serial2.available()) {
            char ch = Serial2.read();
            if (ch == '\n') {
                line.trim();
                if (line.length() > 0) {
                    if (line.indexOf("+CSQ:") >= 0) {
                        sscanf(line.c_str(), "+CSQ: %d,%d", &rssi, &ber);
                    }
                    if (line == "OK") break;
                }
                line = "";
            } else if (ch != '\r') {
                line += ch;
            }
        }
    }
    if (rssi > 0 && rssi <= 31) {
        currentSignal.rssi = rssi;
        currentSignal.ber = ber;
        Logger::debug("[CSQ] parsed rssi=" + String(rssi) + " ber=" + String(ber));
    } else {
        currentSignal.rssi = 99;
        currentSignal.ber = 99;
        Logger::debug("[CSQ] parse failed rssi=" + String(rssi));
    }
    if (rssi >= 0 && rssi <= 31) currentSignal.rssiDbm = -113 + 2 * rssi;
    else currentSignal.rssiDbm = 0;
    if (rssi >= 20) currentSignal.level = "excellent";
    else if (rssi >= 15) currentSignal.level = "good";
    else if (rssi >= 10) currentSignal.level = "weak";
    else if (rssi >= 0) currentSignal.level = "danger";
    else currentSignal.level = "unknown";
    Metrics::onSignalQuality(currentSignal.rssi, currentSignal.rssiDbm);
    return currentSignal;
}

// ========== P0: publishStatusJson ==========
void CellularManager::publishStatusJson() {
    if (mqttState != 1) return;

    // --- Real-time AT queries for fields that change between polls ---
    getSignalQuality();

    String ip = "";
    String r = sendATCommandRaw("AT+CGPADDR", 3000);
    int s = r.indexOf('"');
    int e = r.indexOf('"', s + 1);
    if (s != -1 && e != -1) ip = r.substring(s + 1, e);

    int ceregStat = 0;
    r = sendATCommandRaw("AT+CEREG?", 3000);
    if (sscanf(r.c_str(), "+CEREG: %*d,%d", &ceregStat) < 1) ceregStat = -1;

    // --- Build JSON using unified StatusJsonBuilder ---
    // Start with base string from include/StatusJsonBuilder.h (shared fields: acc, engine, battery, csq, etc.)
    // Then append real-time values at end so they override (JSON last-key-wins).
    String json = "{" + buildBaseStatusString();
    json += ",\"csq\":" + String(currentSignal.rssi);
    json += ",\"rssi_dbm\":" + String(currentSignal.rssiDbm);
    json += ",\"signal\":\"" + String(currentSignal.level) + "\"";
    json += ",\"cereg\":" + String(ceregStat);
    json += ",\"ip\":\"" + ip + "\"";
    json += ",\"online\":true";
    json += ",\"mqtt\":true";
    json += ",\"uptime\":" + String(millis() / 1000);
    if (lastGNSS.valid) {
        json += ",\"lat\":" + String(lastGNSS.latitude, 6);
        json += ",\"lng\":" + String(lastGNSS.longitude, 6);
        json += ",\"speed\":" + String(lastGNSS.speed, 1);
        json += ",\"sat\":" + String(lastGNSS.satellites);
    }
    json += "}";

    // Publish via AT+MPUB (Air780EP native MQTT)
    String safeJson = json;
    safeJson.replace("\"", "'");
    String pubCmd = "AT+MPUB=\"" + String(topic_pub) + "\",0,0,\"" + safeJson + "\"";
    sendATCommand(pubCmd, "OK", 5000);
}

// ========== P0: verifyCommandSignature ==========
bool CellularManager::verifyCommandSignature(const String& jsonPayload) {
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
        Logger::warn("[Cellular] cmd ts too far in future, rejected");
        pushDisconnectLog("cmd rejected: ts future " + String(ts));
        Metrics::onCommandReceived(false);
        return false;
    }
    if (ts < now - 86400UL * 7) {
        Logger::warn("[Cellular] cmd ts too old, rejected");
        pushDisconnectLog("cmd rejected: ts expired " + String(ts));
        Metrics::onCommandReceived(false);
        return false;
    }
    if (isNonceReplay(nonce)) {
        Logger::warn("[Cellular] cmd nonce replay detected, rejected");
        pushDisconnectLog("cmd rejected: nonce replay " + nonce);
        Metrics::onCommandReceived(false);
        return false;
    }
    String signBase = "cmd=" + cmd + "&ts=" + tsStr + "&nonce=" + nonce;
    String signExpected = hmacSha256Hex(kHmacSecret, signBase);
    if (signReceived != signExpected) {
        Logger::warn("[Cellular] cmd signature mismatch, rejected");
        Metrics::onCommandReceived(false);
        pushDisconnectLog("cmd rejected: signature mismatch");
        return false;
    }
    recordNonce(nonce);
    Metrics::onCommandReceived(true);
    Logger::info("[Cellular] cmd signature verified: " + cmd);
    return true;
}

bool CellularManager::isNonceReplay(const String& nonce) {
    for (size_t i = 0; i < kNonceCacheSize; i++) {
        if (nonceCache[i] == nonce) return true;
    }
    return false;
}

void CellularManager::recordNonce(const String& nonce) {
    nonceCache[nonceCacheIndex] = nonce;
    nonceCacheIndex = (nonceCacheIndex + 1) % kNonceCacheSize;
}

// ========== P1: getGNSSInfo ==========
GNSSInfo CellularManager::getGNSSInfo() {
    sendATCommand("AT+CGNSPWR=1", "OK", 6000);
    delay(500);
    String raw = sendATCommandRaw("AT+CGNSINF", 5000);
    int fields[20];
    int parsed = sscanf(raw.c_str(),
        "+CGNSINF: %d,%d,%*[^,],%*[^,],%lf,%lf,%*f,%f,%*f,%d",
        &fields[0], &fields[1], &lastGNSS.latitude, &lastGNSS.longitude,
        &lastGNSS.speed, &fields[2]);
    if (parsed >= 6 && fields[1] == 1) {
        lastGNSS.valid = true;
        lastGNSS.satellites = (uint8_t)fields[2];
        lastGNSS.updatedAt = millis();
        Logger::info("[Cellular] GNSS fix lat=" + String(lastGNSS.latitude, 6)
                     + " lng=" + String(lastGNSS.longitude, 6)
                     + " sat=" + String(lastGNSS.satellites));
        Metrics::onGpsFix(true);
    } else {
        lastGNSS.valid = false;
        Metrics::onGpsFix(false);
        Logger::debug("[Cellular] GNSS no fix yet");
    }
    return lastGNSS;
}

void CellularManager::publishLocation() {
    if (mqttState != 1) return;
    GNSSInfo gps = getGNSSInfo();
    if (!gps.valid) return;
    String json = "{";
    json += "\"type\":\"location\",";
    json += "\"lat\":" + String(gps.latitude, 6) + ",";
    json += "\"lng\":" + String(gps.longitude, 6) + ",";
    json += "\"speed\":" + String(gps.speed, 1) + ",";
    json += "\"sat\":" + String(gps.satellites);
    json += "}";
    if (mqttState == 1) {
        String safeJson = json;
        safeJson.replace("\"", "'");
        String pubCmd = "AT+MPUB=\\\"" + String(topic_pub) + "\\\",0,0,\\\"" + safeJson + "\\\"";
        sendATCommand(pubCmd, "OK", 5000);
    }
    // Logger::info("[Cellular] location published");  // suppressed
}

void CellularManager::abortAsyncConnect(const char* reason) {
    Logger::warn(String("[Cellular] conn abort: ") + reason);
    _connStep = MqttConnStep::WAIT_RETRY;
    _connReply = "";
    Metrics::onMqttReconnectResult(false, reason);
    pushDisconnectLog(String("conn abort: ") + reason);
    scheduleNextReconnect();
}

void CellularManager::processLine(String line) {
    if (line.indexOf("CLOSED") != -1 || line.indexOf("DISCONNECT") != -1 || line.indexOf("+CMQTTDISCONNECT") != -1) {
        mqttState = 0;
        if (offlineSinceMs == 0) offlineSinceMs = millis();
        scheduleNextReconnect();
        pushDisconnectLog("connection closed by remote");
        Logger::warn("[Cellular] connection closed, reconnect scheduled");
        return;
    }
    if (!line.startsWith("+CMQTTSUBRECV:")) return;
    // +MQTTSUBRECV: 0,data_len,"data"
    const int lastQuote = line.lastIndexOf('\"');
    const int prevQuote = line.lastIndexOf('\"', lastQuote - 1);
    if (lastQuote == -1 || prevQuote == -1) return;
    String payload = line.substring(prevQuote + 1, lastQuote);
    Logger::info("[Cellular] cmd raw: " + payload);
    if (payload.startsWith("{") && payload.endsWith("}")) {
        if (!verifyCommandSignature(payload)) {
            Logger::error("[Cellular] command signature verification failed");
            publishStatus("ERROR_SIGNATURE");
            return;
        }
        int cs = payload.indexOf("\"cmd\":\"");
        if (cs == -1) return;
        cs += 7;
        int ce = payload.indexOf("\"", cs);
        payload = payload.substring(cs, ce);
    }
    if (!payload.startsWith("START")) return;
    String pwd = payload.substring(6);
    pwd.trim();
    if (pwd != start_pwd) {
        Logger::error("[Cellular] wrong start password");
        publishStatus("ERROR_WRONG_PWD");
        pushDisconnectLog("wrong start password");
        return;
    }
    if (batteryVoltage.isLowForRemoteStart()) {
        Logger::error("[Cellular] start denied low battery (" + String(batteryVoltage.getVoltage(), 2) + "V)");
        publishStatus("ERROR_LOW_BATTERY");
        return;
    }
    if (TaskManager::sendVehicleCommand(VehicleCommandType::VEHICLE_CMD_START_FROM_MQTT, 20)) {
        publishStatus("ENGINE_START_ACCEPTED");
    } else {
        publishStatus("ERROR_QUEUE_BUSY");
    }
}

// ========== publishStatus ==========
void CellularManager::publishStatus(String msg) {
    if (mqttState != 1) return;
    // Publish via AT+MPUB (Air780EP native MQTT)
    String safeMsg = msg;
    safeMsg.replace("\"", "'");
    String pubCmd = "AT+MPUB=\"" + String(topic_pub) + "\",0,0,\"" + safeMsg + "\"";
    sendATCommand(pubCmd, "OK", 5000);
}

// ========== update ==========
void CellularManager::update() {
    if (_updating) return;
    _updating = true;
    
    // === ALWAYS drain Serial2 first, one place only ===
    // === Read all available Serial2 data and dispatch ===
    while (Serial2.available()) {
        const char c = static_cast<char>(Serial2.read());
        if (c == '\n') {
            buffer.trim();
            if (buffer.length() > 0) {
                if (!modemReady) {
                    // Before modem is ready, feed init state machine via _connReply
                    _connReply = buffer;
                } else {
                    bool fed = false;
                    if (!_sendingSyncCmd && _connStep != MqttConnStep::IDLE && _connStep != MqttConnStep::PUBSUB_ONLINE && _connStep != MqttConnStep::WAIT_RETRY) {
                        _connReply = buffer;
                        if (processConnReply()) fed = true;
                    }
                    if (!fed) processLine(buffer);
                }
            }
            buffer = "";
        } else {
            if (c != '\r') buffer += c;
        }
    }
    
    // Power-on state machine handled entirely by updateModemInit() using _connStep
    if (!modemReady) {
        updateModemInit();
        _updating = false;
        return;
    }
    // MQTT ????????????????????????
    const unsigned long now = millis();
    if (now - lastMqttCheck > 1000) {
        if (mqttState == 0) {
            if (offlineSinceMs == 0) offlineSinceMs = now;
            const unsigned long offlineDuration = now - offlineSinceMs;
            if (_connStep == MqttConnStep::IDLE || _connStep == MqttConnStep::PUBSUB_ONLINE) {
                if (offlineDuration >= 600000UL) {
                    recoverModem();
                } else if (offlineDuration >= 120000UL) {
                    if (now >= nextReconnectAt) {
                        recoverConnection(CellularRecoveryLevel::PDP_RESTART);
                        if (_connStep == MqttConnStep::IDLE) startAsyncConnect();
                    }
                } else {
                    if (now >= nextReconnectAt) startAsyncConnect();
                }
            } else if (_connStep == MqttConnStep::WAIT_RETRY) {
                if (offlineDuration >= 120000UL && now >= nextReconnectAt) {
                    recoverConnection(CellularRecoveryLevel::PDP_RESTART);
                    if (_connStep == MqttConnStep::IDLE || _connStep == MqttConnStep::WAIT_RETRY) startAsyncConnect();
                } else if (now >= nextReconnectAt) {
                    startAsyncConnect();
                }
            } else {
                updateConnection();
            }
        } else {
            offlineSinceMs = 0;
            if (_connStep == MqttConnStep::PUBSUB_ONLINE) {
                // AT native MQTT: read any incoming URC messages
                while (Serial2.available()) {
                    String urcLine;
                    while (Serial2.available()) {
                        char c = Serial2.read();
                        if (c == '\n' || c == '\r') {
                            if (urcLine.length() > 0) break;
                            continue;
                        }
                        urcLine += c;
                    }
                    urcLine.trim();
                    if (urcLine.length() > 0) processLine(urcLine);
                }
            }
            if (_connStep != MqttConnStep::IDLE && _connStep != MqttConnStep::PUBSUB_ONLINE) _connStep = MqttConnStep::IDLE;
        }
        lastMqttCheck = now;
    }
    // ????????????
    if (now - lastCsqCheck >= 60000) { getSignalQuality(); lastCsqCheck = now; }
    // ????????????
    if (now - lastStatusPublish >= kStatusPublishIntervalMs && mqttState == 1) { publishStatusJson(); lastStatusPublish = now; }
    // ????????????
    if (now - lastLocationPublish >= kLocationPublishIntervalMs) { publishLocation(); lastLocationPublish = now; }
    // ????????????
    if (!vehicleStatus.isAccOn() && !lowPowerMode) enterLowPowerMode();
    else if (vehicleStatus.isAccOn() && lowPowerMode) exitLowPowerMode();
    _updating = false;
}


// ========== CellularClientBridge implementation ==========
// Bridges between Arduino Client interface and CellularManager AT MQTT.
// For Air780EP native AT mode, TCP/MQTT is managed by async state machine.
// The Client interface here is a thin wrapper that reports connection status
// and provides write/read hooks for potential transparent-mode use.

int CellularManager::CellularClientBridge::connect(const char* host, uint16_t port) {
    if (_mgr) {
        _mgr->startAsyncConnect();
        // Wait briefly for connection to establish (async)
        unsigned long deadline = millis() + 15000;
        while (millis() < deadline) {
            if (_mgr->isMqttConnected()) {
                _connected = true;
                return 1;
            }
            delay(100);
        }
    }
    return 0;
}

int CellularManager::CellularClientBridge::connect(IPAddress ip, uint16_t port) {
    return connect(ip.toString().c_str(), port);
}

size_t CellularManager::CellularClientBridge::write(uint8_t b) {
    return write(&b, 1);
}

size_t CellularManager::CellularClientBridge::write(const uint8_t* buf, size_t size) {
    if (!_connected || !_mgr) return 0;
    // In AT native MQTT mode, publishing is done via AT+MPUB
    // This write path is used for transparent mode only
    // For now, return 0 to indicate "use AT+MPUB instead"
    return 0;
}

int CellularManager::CellularClientBridge::available() {
    if (!_mgr) return 0;
    // URC messages from AT native MQTT come through processLine()
    return 0;
}

int CellularManager::CellularClientBridge::read() {
    return -1;
}

int CellularManager::CellularClientBridge::read(uint8_t* buf, size_t size) {
    return 0;
}

int CellularManager::CellularClientBridge::peek() {
    return -1;
}

void CellularManager::CellularClientBridge::flush() {
}

void CellularManager::CellularClientBridge::stop() {
    if (_mgr) {
        _mgr->abortAsyncConnect("client stop");
    }
    _connected = false;
}

uint8_t CellularManager::CellularClientBridge::connected() {
    if (!_mgr) return 0;
    return _mgr->isMqttConnected() ? 1 : 0;
}

#endif
