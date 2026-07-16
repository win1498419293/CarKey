#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <Client.h>

// ========== NetworkManager ==========
// Abstract network layer that provides a unified Client interface.
// Upper layers (MqttManager, OTA) use getClient() without caring
// whether the underlying transport is WiFi or Cellular.
//
// Architecture:
//   Application Layer (MqttManager / OTA / HTTP)
//          |
//   NetworkManager::getClient()
// Network selection priority: Cellular (4G) first, WiFi fallback.
//   ???????????????
//   WiFiClient  CellularClient
//          |
//   CellularManager (AT UART engine)
//
// Network selection priority: WiFi first, Cellular fallback.

class NetworkManager {
public:
    enum class Network : uint8_t {
        NONE = 0,
        WIFI,
        CELLULAR
    };

    void init();
    void update();

    // Get the active Client instance for the current best network
    // Returns nullptr if no network is available
    Client* getClient();

    // Get the active WiFiClient (always available, even when WiFi disconnected)
    Client* getWiFiClient() { return &_wifiClient; }

    // Get the active CellularClient (may be null if cellular not ready)
    Client* getCellularClient();

    // Check if any network is online
    bool isOnline() const { return _activeNetwork != Network::NONE; }
    bool isWifiOnline() const { return _wifiOnline; }
    bool isCellularOnline() const { return _cellularOnline; }

    // Get current active network type
    Network getActiveNetwork() const { return _activeNetwork; }
    const char* getActiveNetworkName() const;

    // Force network preference (e.g., force cellular when WiFi is weak)
    void setPreferCellular(bool prefer) { _preferCellular = prefer; }
    bool getPreferCellular() const { return _preferCellular; }

    // Set network switching callback
    void onNetworkChange(void (*callback)(Network newNetwork));

private:
    WiFiClient _wifiClient;
    Network _activeNetwork = Network::NONE;
    bool _wifiOnline = false;
    bool _cellularOnline = false;
    bool _preferCellular = false;
    bool _initialized = false;
    unsigned long _lastNetworkCheck = 0;
    Network _lastReportedNetwork = Network::NONE;
    void (*_onNetworkChange)(Network) = nullptr;

    void evaluateNetwork();
    void switchToNetwork(Network net);
};

extern NetworkManager networkManager;

