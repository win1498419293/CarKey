#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Wire.h>

// DisplayManager - SH1106 128x64 OLED via I2C (SDA=8, SCL=21)
// Shows status messages driven by the same StatusLight::State
// Call DisplayManager::update() from the sensorTask alongside StatusLight::update()

class DisplayManager {
public:
    static void init();
    static void update();

    // Show two-line message
    static void show(const String& line1, const String& line2);

private:
    static Adafruit_SH1106G* _display;
    static bool _ready;
    static String _lastL1, _lastL2;
    static unsigned long _lastUpdateMs;

    static const uint8_t SDA = 8;
    static const uint8_t SCL = 21;
    static const uint8_t ADDR = 0x3C;
};

extern DisplayManager displayManager;
