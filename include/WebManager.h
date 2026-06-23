#pragma once
#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>

class WebManager {
public:
    void init();
    void handle();
    void unlock();
    void lock();
    String scanWiFi();
private:
    void setupRoutes();
    void serveStaticFile(const char* path, const char* contentType);
};

extern WebManager webManager; // Global singleton instance
