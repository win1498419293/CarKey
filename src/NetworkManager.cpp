#include "NetworkManager.h"
#include "Logger.h"
#include "Config.h"
#if ENABLE_CELLULAR
#include "CellularManager.h"
#include "CellularClient.h"
#endif
#include <WiFi.h>

NetworkManager networkManager;

void NetworkManager::init() {
    if (_initialized) return;
    _initialized = true;
    Logger::info("[Network] NetworkManager initialized");
    _activeNetwork = Network::NONE;
    _lastReportedNetwork = Network::NONE;
}

void NetworkManager::update() {
    if (!_initialized) return;

    const unsigned long now = millis();
    if (now - _lastNetworkCheck < 1000) return;  // Check every 1s
    _lastNetworkCheck = now;

    // Update WiFi status
    _wifiOnline = (WiFi.status() == WL_CONNECTED);

    // Update Cellular status
#if ENABLE_CELLULAR
    _cellularOnline = cellularManager.isMqttConnected();
#else
    _cellularOnline = false;
#endif

    evaluateNetwork();
}

void NetworkManager::evaluateNetwork() {
    Network best = Network::NONE;
    // Priority: Cellular first, WiFi fallback
    if (_cellularOnline) {
        best = Network::CELLULAR;
    } else if (_wifiOnline) {
        best = Network::WIFI;
    }
    if (best != _activeNetwork) {
        switchToNetwork(best);
    }
    _activeNetwork = best;
}
void NetworkManager::switchToNetwork(Network net) {
    Network old = _activeNetwork;
    _activeNetwork = net;
    Logger::info(String("[Network] switching: ") + getActiveNetworkName() +
                 " (was " + (old == Network::WIFI ? "WiFi" :
                             old == Network::CELLULAR ? "Cellular" : "None") + ")");
    if (_onNetworkChange) {
        _onNetworkChange(net);
    }
}

Client* NetworkManager::getClient() {
    switch (_activeNetwork) {
        case Network::WIFI:
            return &_wifiClient;
        case Network::CELLULAR:
#if ENABLE_CELLULAR
            return getCellularClient();
#else
            return nullptr;
#endif
        default:
            return nullptr;
    }
}

Client* NetworkManager::getCellularClient() {
#if ENABLE_CELLULAR
    return cellularManager.getClient();
#else
    return nullptr;
#endif
}

const char* NetworkManager::getActiveNetworkName() const {
    switch (_activeNetwork) {
        case Network::WIFI:     return "WiFi";
        case Network::CELLULAR: return "4G";
        default:                return "None";
    }
}

void NetworkManager::onNetworkChange(void (*callback)(Network)) {
    _onNetworkChange = callback;
}
