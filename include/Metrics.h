#pragma once

#include <Arduino.h>

namespace Metrics {

void begin();
void tick();

#if ENABLE_BLE
void onBleScanStart();
void onBleAuthorizedDetected();
#endif

void onRemoteStartQueued();
void onRemoteStartResult(bool success, const char* reason);

#if ENABLE_CELLULAR
void onMqttReconnectAttempt();
void onMqttReconnectResult(bool success, const char* reason);
void onMqttHeartbeat(unsigned long rttMs);
void onCellularRecovery(const char* level);
void onSignalQuality(int rssi, int dBm);
void onGpsFix(bool success);
#endif
void onCommandReceived(bool verified);

// --- ?????? ---
int getMqttRssi();
String getMqttStatusJson();

}  // namespace Metrics
