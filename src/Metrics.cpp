#include "Config.h"
#include "Metrics.h"

#include "Logger.h"

namespace Metrics {
namespace {

struct CounterPair {
    uint32_t ok = 0;
    uint32_t fail = 0;
};

CounterPair g_remoteStart;
#if ENABLE_CELLULAR
CounterPair g_mqttReconnect;
#endif

uint32_t g_startFailLowBattery = 0;
uint32_t g_startFailQueueBusy = 0;
uint32_t g_startFailRelayRejected = 0;
uint32_t g_startFailOther = 0;

#if ENABLE_BLE
uint32_t g_bleHits = 0;
unsigned long g_bleScanStartMs = 0;
uint32_t g_bleLatencySamples = 0;
unsigned long g_bleLatencyTotalMs = 0;
unsigned long g_bleLatencyMaxMs = 0;
#endif

uint32_t g_startLatencySamples = 0;
unsigned long g_startLatencyTotalMs = 0;
unsigned long g_startLatencyMaxMs = 0;
unsigned long g_startQueuedAtMs = 0;

#if ENABLE_CELLULAR
uint32_t g_mqttLatencySamples = 0;
unsigned long g_mqttLatencyTotalMs = 0;
unsigned long g_mqttLatencyMaxMs = 0;
unsigned long g_mqttAttemptAtMs = 0;

// ??????
uint32_t g_heartbeatSamples = 0;
unsigned long g_heartbeatTotalMs = 0;
unsigned long g_heartbeatMaxMs = 0;

// ??????
uint32_t g_recoveryMqtt = 0;
uint32_t g_recoveryPdp = 0;
uint32_t g_recoveryCfun = 0;

// ????
uint32_t g_cmdVerified = 0;
uint32_t g_cmdRejected = 0;

// ????
int g_lastRssi = 99;
int g_lastDbm = 0;

// GPS
uint32_t g_gpsFixOk = 0;
uint32_t g_gpsFixFail = 0;
#endif

unsigned long g_nextReportAtMs = 0;
constexpr unsigned long kReportIntervalMs = 60000;

void addLatency(unsigned long valueMs, uint32_t& samples, unsigned long& totalMs, unsigned long& maxMs) {
    samples++;
    totalMs += valueMs;
    if (valueMs > maxMs) {
        maxMs = valueMs;
    }
}

String avgToString(uint32_t samples, unsigned long totalMs) {
    if (samples == 0) {
        return "n/a";
    }
    return String(totalMs / samples);
}

}  // namespace

void begin() {
    g_nextReportAtMs = millis() + kReportIntervalMs;
}

void tick() {
    const unsigned long now = millis();
    if (static_cast<long>(now - g_nextReportAtMs) < 0) {
        return;
    }

    String line = "[METRIC]";

    // ??????
    line += " start_ok=" + String(g_remoteStart.ok);
    line += " start_fail=" + String(g_remoteStart.fail);
    line += " start_avg_ms=" + avgToString(g_startLatencySamples, g_startLatencyTotalMs);
    line += " start_max_ms=" + String(g_startLatencyMaxMs);

#if ENABLE_CELLULAR
    line += " | MQTT reconnect_ok=" + String(g_mqttReconnect.ok);
    line += " reconnect_fail=" + String(g_mqttReconnect.fail);
    line += " reconnect_avg_ms=" + avgToString(g_mqttLatencySamples, g_mqttLatencyTotalMs);
    line += " reconnect_max_ms=" + String(g_mqttLatencyMaxMs);

    line += " heartbeat_avg_ms=" + avgToString(g_heartbeatSamples, g_heartbeatTotalMs);
    line += " heartbeat_max_ms=" + String(g_heartbeatMaxMs);

    line += " | recovery MQTT=" + String(g_recoveryMqtt);
    line += " PDP=" + String(g_recoveryPdp);
    line += " CFUN=" + String(g_recoveryCfun);

    line += " cmd_ok=" + String(g_cmdVerified);
    line += " cmd_reject=" + String(g_cmdRejected);

    line += " rssi=" + String(g_lastRssi) + "(" + String(g_lastDbm) + "dBm)";
    line += " gps_ok=" + String(g_gpsFixOk) + " gps_fail=" + String(g_gpsFixFail);
#endif

    Logger::info(line);

    g_nextReportAtMs = now + kReportIntervalMs;
}

#if ENABLE_BLE
void onBleScanStart() {
    g_bleScanStartMs = millis();
}

void onBleAuthorizedDetected() {
    g_bleHits++;
    if (g_bleScanStartMs != 0) {
        const unsigned long latency = millis() - g_bleScanStartMs;
        addLatency(latency, g_bleLatencySamples, g_bleLatencyTotalMs, g_bleLatencyMaxMs);
    }
}
#endif

void onRemoteStartQueued() {
    g_startQueuedAtMs = millis();
}

void onRemoteStartResult(bool success, const char* reason) {
    if (success) {
        g_remoteStart.ok++;
    } else {
        g_remoteStart.fail++;
        String r = reason == nullptr ? "" : String(reason);
        if (r == "low_battery") {
            g_startFailLowBattery++;
        } else if (r == "queue_busy") {
            g_startFailQueueBusy++;
        } else if (r == "relay_reject") {
            g_startFailRelayRejected++;
        } else {
            g_startFailOther++;
        }
    }

    if (g_startQueuedAtMs != 0) {
        const unsigned long latency = millis() - g_startQueuedAtMs;
        addLatency(latency, g_startLatencySamples, g_startLatencyTotalMs, g_startLatencyMaxMs);
        g_startQueuedAtMs = 0;
    }
}

#if ENABLE_CELLULAR
void onMqttReconnectAttempt() {
    g_mqttAttemptAtMs = millis();
}

void onMqttReconnectResult(bool success, const char* reason) {
    (void)reason;
    if (success) {
        g_mqttReconnect.ok++;
    } else {
        g_mqttReconnect.fail++;
    }

    if (g_mqttAttemptAtMs != 0) {
        const unsigned long latency = millis() - g_mqttAttemptAtMs;
        addLatency(latency, g_mqttLatencySamples, g_mqttLatencyTotalMs, g_mqttLatencyMaxMs);
        g_mqttAttemptAtMs = 0;
    }
}

void onMqttHeartbeat(unsigned long rttMs) {
    addLatency(rttMs, g_heartbeatSamples, g_heartbeatTotalMs, g_heartbeatMaxMs);
}

void onCellularRecovery(const char* level) {
    String lvl = level == nullptr ? "" : String(level);
    if (lvl == "mqtt") {
        g_recoveryMqtt++;
    } else if (lvl == "pdp") {
        g_recoveryPdp++;
    } else if (lvl == "cfun") {
        g_recoveryCfun++;
    }
}

void onCommandReceived(bool verified) {
    if (verified) {
        g_cmdVerified++;
    } else {
        g_cmdRejected++;
    }
}

void onSignalQuality(int rssi, int dBm) {
    g_lastRssi = rssi;
    g_lastDbm = dBm;
}

void onGpsFix(bool success) {
    if (success) {
        g_gpsFixOk++;
    } else {
        g_gpsFixFail++;
    }
}

int getMqttRssi() {
    return g_lastRssi;
}

String getMqttStatusJson() {
    String json = "{";
    json += "\"reconnect_ok\":" + String(g_mqttReconnect.ok) + ",";
    json += "\"reconnect_fail\":" + String(g_mqttReconnect.fail) + ",";
    json += "\"reconnect_avg_ms\":" + avgToString(g_mqttLatencySamples, g_mqttLatencyTotalMs) + ",";
    json += "\"heartbeat_avg_ms\":" + avgToString(g_heartbeatSamples, g_heartbeatTotalMs) + ",";
    json += "\"rssi\":" + String(g_lastRssi) + ",";
    json += "\"rssi_dbm\":" + String(g_lastDbm);
    json += "}";
    return json;
}
#endif

}  // namespace Metrics
