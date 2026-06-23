#include "Logger.h"
#include "Config.h"

void Logger::info(const String& msg) {
    sysLog(msg, "info");
}

void Logger::warn(const String& msg) {
    sysLog(msg, "warn");
}

void Logger::error(const String& msg) {
    sysLog(msg, "err");
}

void Logger::debug(const String& msg) {
#if LOGGER_DEBUG_ENABLED
    sysLog(msg, "debug");
#else
    (void)msg;
#endif
}
