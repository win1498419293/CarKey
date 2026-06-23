#pragma once
#include <Arduino.h>

// Unified logging interface with levels: INFO / WARN / ERROR / DEBUG
#ifndef LOGGER_DEBUG_ENABLED
#define LOGGER_DEBUG_ENABLED 1
#endif

class Logger {
public:
    static void info(const String& msg);
    static void warn(const String& msg);
    static void error(const String& msg);
    static void debug(const String& msg);
};
