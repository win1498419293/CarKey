#pragma once
#include <Arduino.h>
#include <vector>
#include <Client.h>

// ========== Recovery Level ==========
enum class CellularRecoveryLevel : uint8_t {
    MQTT_ONLY = 0,
    PDP_RESTART,
    CFUN_RESTART,
};

// ========== Signal Quality ==========
struct CellularSignal {
    int rssi = 0;
    int ber = 99;
    int rssiDbm = 0;
    const char* level;
};

// ========== GNSS Info ==========
struct GNSSInfo {
    bool valid = false;
    double latitude = 0.0;
    double longitude = 0.0;
    float speed = 0.0f;
    uint8_t satellites = 0;
    unsigned long updatedAt = 0;
};

// ========== Disconnect Record ==========
struct DisconnectRecord {
    unsigned long atMs = 0;
    String reason;
};

// ========== MQTT Connection Step (Air780EP AT native) ==========
enum class MqttConnStep : uint8_t {
    IDLE = 0,
    POWER_ON,           // Power on module (PWRKEY pulse)
    POWER_WAIT,         // Wait for module to boot
    AT_PROBE,           // AT ? OK
    ATE0_SET,           // ATE0 ? OK
    CPIN_CHECK,         // AT+CPIN? ? +CPIN: READY
    CGDCONT_SET,        // AT+CGDCONT=1,"IP","CMNET" ? OK
    CEREG_CHECK,        // AT+CEREG? ? registered
    CGATT_CHECK,        // AT+CGATT? ? 1
    CGACT_CHECK,        // AT+CGACT? ? active
    MCONFIG_SET,        // AT+MCONFIG="clientid" ? OK
    MIPSTART_CONNECT,   // AT+MIPSTART="ip",port ? CONNECT OK
    MCONNECT_SESSION,   // AT+MCONNECT=1,120 ? CONNACK OK
    MSUBSCRIBE,         // AT+MSUB="topic",0 ? SUBACK
    PUBSUB_ONLINE,      // MQTT fully online
    WAIT_RETRY,
};

// ========== CellularManager ==========
// Pure AT communication engine for cellular module.
// - Manages UART (Serial2) exclusively
// - Provides Client interface for upper layers via getClient()
// - Handles module power-on, AT init, network registration
// - 4G MQTT via Air780EP native AT commands (MCONFIG/MIPSTART/MCONNECT/MSUB/MPUB)
// - Signal quality, GNSS, power management
//
// To support a different module (e.g., EC200U, SIM7600):
//   Replace the AT command sequences and MqttConnStep transitions
//   Upper layers (MqttManager / NetworkManager) don't change.

class CellularManager {
public:
    void init();
    void update();
    void publishStatus(String payload);

    // --- Client interface (for NetworkManager) ---
    // Returns a Client pointer that bridges AT-based MQTT data
    Client* getClient() { return &_client; }

    // --- Connection management ---
    void startAsyncConnect();
    void updateConnection();

    // --- P0: Graded recovery ---
    void recoverConnection(CellularRecoveryLevel level);

    // --- P0: Signal quality ---
    CellularSignal getSignalQuality();

    // --- P0: Status JSON publish ---
    void publishStatusJson();

    // --- P0: Command signature verification ---
    bool verifyCommandSignature(const String& jsonPayload);

    // --- AT line-level communication ---
    String readATLine(unsigned long timeoutMs, bool logRaw = true);
    bool sendATCommandLine(String cmd, String expectedResponse, unsigned long timeout, bool logRaw = true);
    bool sendATCommand(String cmd, String expectedResponse, unsigned long timeout);
    String sendATCommandRaw(String cmd, unsigned long timeout);

    // --- P1: GNSS ---
    GNSSInfo getGNSSInfo();
    const GNSSInfo& getLastGNSSInfo() const { return lastGNSS; }
    void publishLocation();

    // --- P1: Disconnect log ---
    const std::vector<DisconnectRecord>& getDisconnectLog() const { return disconnectLog; }

    // --- P1: Power management ---
    void enterLowPowerMode();
    void exitLowPowerMode();

    // --- Getters ---
    bool isMqttConnected() const { return mqttState == 1; }
    bool isModemReady() const { return modemReady; }
    bool isNetworkRegistered() const { return _networkRegistered; }
    const CellularSignal& getCurrentSignal() const { return currentSignal; }

    // --- Process async AT response ---
    bool processConnReply();
    void abortAsyncConnect(const char* reason);

    // --- Power control ---
    void powerOn();    // Pulse PWRKEY to turn on module
    void powerOff();   // Turn off module

private:
    friend class CellularClientBridge;

    // --- Client bridge implementation ---
    class CellularClientBridge : public Client {
    public:
        CellularClientBridge(CellularManager* mgr) : _mgr(mgr) {}
        int connect(IPAddress ip, uint16_t port) override;
        int connect(const char* host, uint16_t port) override;
        size_t write(uint8_t b) override;
        size_t write(const uint8_t* buf, size_t size) override;
        int available() override;
        int read() override;
        int read(uint8_t* buf, size_t size) override;
        int peek() override;
        void flush() override;
        void stop() override;
        uint8_t connected() override;
        operator bool() override { return _connected; }
    private:
        CellularManager* _mgr;
        bool _connected = false;
    };

    CellularClientBridge _client{this};

    // --- State ---
    String buffer = "";
    unsigned long lastMqttCheck = 0;
    int mqttState = 0;          // 0=disconnected, 1=connected
    bool useBackupBroker = false;
    unsigned long nextReconnectAt = 0;
    unsigned long reconnectDelayMs = 5000;
    unsigned long offlineSinceMs = 0;
    bool modemReady = false;
    bool _networkRegistered = false;
    uint8_t modemInitAttempts = 0;
    unsigned long nextModemProbeAt = 0;
    bool lowPowerMode = false;
    bool _initialized = false;
    bool _updating = false;

    // --- Signal quality ---
    unsigned long lastCsqCheck = 0;
    CellularSignal currentSignal;

    // --- Status publish ---
    unsigned long lastStatusPublish = 0;
    static constexpr unsigned long kStatusPublishIntervalMs = 60000;

    // --- Nonce cache ---
    static constexpr size_t kNonceCacheSize = 32;
    String nonceCache[kNonceCacheSize];
    uint8_t nonceCacheIndex = 0;

    // --- GNSS ---
    unsigned long lastLocationPublish = 0;
    static constexpr unsigned long kLocationPublishIntervalMs = 300000;
    GNSSInfo lastGNSS;

    // --- Disconnect log ---
    static constexpr size_t kDisconnectLogMax = 10;
    std::vector<DisconnectRecord> disconnectLog;

    // --- Heartbeat ---
    unsigned long lastHeartbeat = 0;
    unsigned long heartbeatIntervalMs = 60000;

    // --- MQTT connection state machine ---
    MqttConnStep _connStep = MqttConnStep::IDLE;
    unsigned long _connStepStartMs = 0;
    unsigned long _connTimeoutMs = 0;
    String _connActiveBroker = "";
    int _connMqttPort = 1883;
    bool _connUseBackup = false;
    String _connReply = "";
    bool _sendingSyncCmd = false;
    static constexpr unsigned long kConnStepTimeoutMs = 3000;

    // --- MQTT AT command params ---
    String _mqttClientId;
    String _mqttBroker;
    String _mqttBrokerIp;

    // --- TCP async connect state ---
    String _tcpReply = "";
    String _atLineBuf = "";
    bool _atLineInProgress = false;

    // --- PWRKEY pin ---
    static constexpr uint8_t PWRKEY_PIN = 4;  // GPIO4 controls module PWRKEY

    // --- Internal methods ---
    void processLine(String line);
    void scheduleNextReconnect();
    void recoverModem();
    void updateModemInit();
    void pushDisconnectLog(const String& reason);
    bool isNonceReplay(const String& nonce);
    void recordNonce(const String& nonce);
    void pulsePwrkey();
};

#if ENABLE_CELLULAR
extern CellularManager cellularManager;
#endif
