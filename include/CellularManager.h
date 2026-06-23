#pragma once
#include <Arduino.h>

class CellularManager {
public:
    void init();
    void update();
    void publishStatus(String payload);

private:
    String buffer = "";
    unsigned long lastMqttCheck = 0;
    int mqttState = 0;
    bool useBackupBroker = false;
    unsigned long nextReconnectAt = 0;
    unsigned long reconnectDelayMs = 5000;
    unsigned long offlineSinceMs = 0;
    bool modemReady = false;
    uint8_t modemInitAttempts = 0;
    unsigned long nextModemProbeAt = 0;

    void processLine(String line);
    bool connectMQTT();
    void scheduleNextReconnect();
    void recoverModem();
    void updateModemInit();

    bool sendATCommand(String cmd, String expectedResponse, unsigned long timeout);
};

#if ENABLE_CELLULAR
extern CellularManager cellularManager;
#endif
