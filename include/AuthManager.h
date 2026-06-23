#pragma once
#include <Arduino.h>

class AuthManager {
public:
    static bool verify(uint8_t* uid, uint8_t len);
};

