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
#endif

}  // namespace Metrics