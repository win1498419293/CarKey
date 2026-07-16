#pragma once
#include <Arduino.h>

// ESP32 UART2 pins for Air780EP
#define AIR780EP_RXD2 16
#define AIR780EP_TXD2 17
#define AIR780EP_BAUD 115200

// AT command timeout defaults (ms)
#define AT_TIMEOUT_DEFAULT 3000
#define AT_TIMEOUT_LONG    10000
#define AT_TIMEOUT_CPIN    15000

// Test step result
#define TEST_PASS  true
#define TEST_FAIL  false

// Test step macro
#define TEST_STEP(name) do { \
    Serial.println(""); \
    Serial.println("========================================"); \
    Serial.print("[TEST] "); \
    Serial.println(name); \
    Serial.println("========================================"); \
} while(0)

#define TEST_RESULT(pass, msg) do { \
    Serial.print(pass ? "[PASS] " : "[FAIL] "); \
    Serial.println(msg); \
} while(0)
