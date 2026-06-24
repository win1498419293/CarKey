#pragma once

class RelayManager {
public:
    void init();
    void on();
    void off();
    bool isOn();

    // Engine relay sequence (does not set engine running state — VehicleStatusManager handles detection)
    bool startEngine();
    void stopEngine();
    void update();

    // Horn control
    void triggerAlarm();
    void stopAlarm();

private:
    bool state = false;
    bool engineRelayActive = false;    // Track if start relay sequence was executed
    unsigned long startChargeVerifyBeginMs = 0;
};

extern RelayManager relayManager;
