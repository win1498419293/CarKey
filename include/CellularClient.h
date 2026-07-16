#pragma once
#include <Arduino.h>
#include <Client.h>

// ========== CellularClient ==========
// Bridges CellularManager's AT-based MQTT to Arduino Client interface.
// Does NOT operate UART directly ? all data flows through CellularManager.
//
// For Air780EP: uses AT native MQTT commands.
// For transparent-mode modules: would use AT+CIPSTART + AT+CIPSEND.
// 
// To add support for a new module type, create a new Client subclass
// that implements connect/write/read/stop using that module's AT commands.

class CellularClient : public Client {
public:
    CellularClient() {}

    // These must be implemented for compatibility, but for Air780EP's
    // native AT MQTT mode, connection is managed by CellularManager state machine.
    int connect(IPAddress ip, uint16_t port) override {
        return connect(ip.toString().c_str(), port);
    }

    int connect(const char* host, uint16_t port) override {
        // Connection is handled by CellularManager's async state machine.
        // This method signals the manager to begin connecting.
        return startConnect(host, port);
    }

    size_t write(uint8_t b) override {
        return write(&b, 1);
    }

    size_t write(const uint8_t* buf, size_t size) override;

    int available() override;
    int read() override;
    int read(uint8_t* buf, size_t size) override;
    int peek() override;
    void flush() override;
    void stop() override;
    uint8_t connected() override;
    operator bool() override { return _connected; }

    bool isConnected() const { return _connected; }

private:
    bool _connected = false;

    int startConnect(const char* host, uint16_t port);
};
