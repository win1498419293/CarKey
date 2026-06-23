#pragma once
#include <Arduino.h>

// StatusLight - unified status LED ring (16x WS2812 on GPIO48) + buzzer (GPIO47)

class StatusLight {
public:
    enum class State : uint8_t {
        OFF = 0,
        IDLE,
        NFC_WAITING,
        NFC_AUTHENTICATING,
        NFC_SUCCESS,
        NFC_FAIL,
        BLE_SCANNING,
        BLE_CONNECTED,
        BLE_PAIRING,
        OTA,
        ERROR,
        LOW_BATTERY,
    };

    static void init();
    static void update();
    static void setStatus(State s);

    static void setIdle()           { setStatus(State::IDLE); }
    static void setWaitingNFC()     { setStatus(State::NFC_WAITING); }
    static void setAuthenticating() { setStatus(State::NFC_AUTHENTICATING); }
    static void setNFCSuccess()     { setStatus(State::NFC_SUCCESS); }
    static void setNFCError()       { setStatus(State::NFC_FAIL); }
    static void setBLEScanning()    { setStatus(State::BLE_SCANNING); }
    static void setBLEConnected()   { setStatus(State::BLE_CONNECTED); }
    static void setBLEPairing()     { setStatus(State::BLE_PAIRING); }
    static void setOTA()            { setStatus(State::OTA); }
    static void setError()          { setStatus(State::ERROR); }
    static void setLowBattery()     { setStatus(State::LOW_BATTERY); }

    static State currentState() { return _state; }
    static bool isTransient();

    static const uint8_t LED_COUNT = 16;

private:
    // Core
    static void _fillColor(uint8_t r, uint8_t g, uint8_t b);
    static void _off();

    // Animations
    static void _breathe(uint8_t r, uint8_t g, uint8_t b, uint16_t periodMs);
    static void _blink(uint8_t r, uint8_t g, uint8_t b, uint16_t onMs, uint16_t offMs);
    static void _blinkAlt(uint8_t r1, uint8_t g1, uint8_t b1, uint8_t r2, uint8_t g2, uint8_t b2, uint16_t eachMs);
    static void _chase(uint8_t r, uint8_t g, uint8_t b, uint16_t speedMs);
    static void _rotate(uint8_t r, uint8_t g, uint8_t b, uint16_t speedMs);
    static void _shake(uint8_t r, uint8_t g, uint8_t b);
    static void _rainbowBreathe(uint16_t periodMs, uint16_t huePeriodMs);

    // Transient
    static void _beginTransient(uint8_t blinkTarget, unsigned long durationMs);

    // Buzzer
    static void _beep(uint16_t ms);
    static void _successBeep();
    static void _failBeep();

    static State _state;
    static State _prevState;
    static unsigned long _stateStartMs;
    static unsigned long _lastUpdateMs;

    static uint8_t  _transientBlinkCount;
    static uint8_t  _transientBlinkTarget;
    static unsigned long _transientDurationMs;
    static bool     _transientDone;

    static const uint8_t  LED_PIN = 48;
    static const uint8_t  BRIGHTNESS = 40;
    static const uint8_t  BUZZER_PIN = 47;
};