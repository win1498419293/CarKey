#pragma once
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <SPIFFS.h>

class WebManager {
public:
    void init();
    void handle();
    void unlock();
    void lock();
    String scanWiFi();

    // V6: WebSocket real-time status push
    void broadcastStatus(const String& json);
    void wsLoop();

private:
    void setupRoutes();
    void serveStaticFile(const char* path, const char* contentType);
    void handleWebSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length);
};

extern WebManager webManager;
extern WebSocketsServer webSocket;
