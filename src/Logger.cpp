#include "Logger.h"
#include "Config.h"
#include "HistoryManager.h"

void Logger::info(const String& msg) {
    sysLog(msg, "info");
    historyManager.recordLog("INFO", msg);
}

void Logger::warn(const String& msg) {
    sysLog(msg, "warn");
    historyManager.recordLog("WARN", msg);
}

void Logger::error(const String& msg) {
    sysLog(msg, "err");
    historyManager.recordLog("ERR", msg);
}

void Logger::debug(const String& msg) {
#if LOGGER_DEBUG_ENABLED
    sysLog(msg, "debug");
    historyManager.recordLog("DEBUG", msg);
#else
    (void)msg;
#endif
}
