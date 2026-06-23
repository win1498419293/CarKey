#pragma once

class RelayManager {
public:
    void init();
    void on();
    void off();
    bool isOn();

    bool startEngine(); 
    void stopEngine();
    void update();
    // Horn control
    bool isEngineRunning(); 
    void triggerAlarm();
    void stopAlarm();

private:
    bool state = false;
    bool engineRunning = false; // Engine running state
    unsigned long startChargeVerifyBeginMs = 0; // 0 = not verifying
};

extern RelayManager relayManager;