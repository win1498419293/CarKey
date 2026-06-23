#pragma once

#include "RelayManager.h"

enum CarState {
    LOCKED,
    UNLOCKED,
    ENGINE_ON
};

class StateMachine {
public:
    explicit StateMachine(RelayManager* relay);
    void update();
    void requireSecondaryAuth();
    void clearPendingSecondaryAuth();
    void onAuthorizedNfc();
    void onUnauthorizedNfc();

private:
    void scheduleAlarmPulse(unsigned long durationMs);

    CarState state = LOCKED;
    RelayManager* relay = nullptr;
    bool pendingSecondaryAuth = false;
    bool alarmPulseActive = false;
    unsigned long alarmPulseUntilMs = 0;
};

extern StateMachine stateMachine;
