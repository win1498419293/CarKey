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



#if ENABLE_CELLULAR
    Logger::info("[METRIC][MQTT] reconnect_ok=" + String(g_mqttReconnect.ok) + " reconnect_fail=" + String(g_mqttReconnect.fail)
                 + " reconnect_avg_ms=" + avgToString(g_mqttLatencySamples, g_mqttLatencyTotalMs)
                 + " reconnect_max_ms=" + String(g_mqttLatencyMaxMs));
#endif

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
#endif

}  // namespace Metrics