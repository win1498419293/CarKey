#include "Config.h"
#if ENABLE_CELLULAR
#include "CellularManager.h"

#include "BatteryVoltage.h"
#include "Config.h"
#include "Logger.h"
#include "Metrics.h"
#include "TaskManager.h"

extern BatteryVoltage batteryVoltage;
CellularManager cellularManager;

#define RXD2 16
#define TXD2 17

const char* mqtt_broker_primary = "broker.emqx.io";
const char* mqtt_broker_backup = "test.mosquitto.org";
const int mqtt_port = 1883;
const char* mqtt_client_id = "CarKeyV5_Pro_888X";
const char* topic_sub = "carkey/v5/cmd";
const char* topic_pub = "carkey/v5/status";

bool CellularManager::sendATCommand(String cmd, String expectedResponse, unsigned long timeout) {
    while (Serial2.available()) {
        Serial2.read();
        delay(2);
    }

    Logger::debug("[AT->] " + cmd);
    Serial2.println(cmd);

    const unsigned long t = millis();
    String reply = "";
    while (millis() - t < timeout) {
        while (Serial2.available()) {
            reply += static_cast<char>(Serial2.read());
        }

        if (reply.indexOf(expectedResponse) != -1) {
            reply.replace("\r", "");
            reply.replace("\n", " ");
            Logger::debug("[AT<- OK] " + reply);
            return true;
        }
        if (reply.indexOf("ERROR") != -1 && expectedResponse != "ERROR") {
            reply.replace("\r", "");
            reply.replace("\n", " ");
            Logger::debug("[AT<- ERR] " + reply);
            return false;
        }
    }

    Logger::warn("[AT<- TIMEOUT] " + reply);
    return false;
}

void CellularManager::init() {
    Logger::info("[Cellular] init 4G MQTT engine");
    Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);

    mqttState = 0;
    useBackupBroker = false;
    reconnectDelayMs = 5000;
    nextReconnectAt = millis();
    offlineSinceMs = millis();
    modemReady = false;
    modemInitAttempts = 0;
    nextModemProbeAt = millis() + 1000;
}

void CellularManager::updateModemInit() {
    if (modemReady || millis() < nextModemProbeAt) {
        return;
    }

    if (sendATCommand("AT", "OK", 1500)) {
        sendATCommand("ATE0", "OK", 800);
        modemReady = true;
        mqttState = 0;
        reconnectDelayMs = 5000;
        nextReconnectAt = millis();
        offlineSinceMs = millis();
        Logger::info("[Cellular] modem ready");
        return;
    }

    modemInitAttempts++;
    Logger::warn("[Cellular] modem not ready, retry #" + String(modemInitAttempts));
    const unsigned long backoffMs = min(8000UL, 1000UL + static_cast<unsigned long>(modemInitAttempts) * 500UL);
    nextModemProbeAt = millis() + backoffMs;
}

bool CellularManager::connectMQTT() {
    Metrics::onMqttReconnectAttempt();
    const char* activeBroker = useBackupBroker ? mqtt_broker_backup : mqtt_broker_primary;
    Logger::info(String("[Cellular] connect MQTT via ") + (useBackupBroker ? "backup: " : "primary: ") + activeBroker);

    if (!sendATCommand("AT+CGATT?", "+CGATT: 1", 3000)) {
        Logger::warn("[Cellular] network not attached yet");
        Metrics::onMqttReconnectResult(false, "cgatt");
        return false;
    }

    sendATCommand("AT+MDISCONNECT", "OK", 2000);
    delay(1000);

    String mconfig = "AT+MCONFIG=\"";
    mconfig += mqtt_client_id;
    mconfig += "\",\"\",\"\"";
    sendATCommand(mconfig, "OK", 2000);
    delay(500);

    String mipstart = "AT+MIPSTART=\"";
    mipstart += activeBroker;
    mipstart += "\",";
    mipstart += mqtt_port;
    if (!sendATCommand(mipstart, "CONNECT OK", 15000)) {
        Logger::error("[Cellular] TCP tunnel failed");
        Metrics::onMqttReconnectResult(false, "tcp");
        return false;
    }

    delay(500);
    if (!sendATCommand("AT+MCONNECT=1,60", "CONNACK OK", 5000)) {
        Logger::error("[Cellular] MQTT connect rejected");
        Metrics::onMqttReconnectResult(false, "mconnect");
        return false;
    }

    delay(500);
    String msub = "AT+MSUB=\"";
    msub += topic_sub;
    msub += "\",0";
    if (!sendATCommand(msub, "SUBACK", 5000)) {
        Logger::error("[Cellular] subscribe failed");
        Metrics::onMqttReconnectResult(false, "msub");
        return false;
    }

    mqttState = 1;
    reconnectDelayMs = 5000;
    nextReconnectAt = millis() + reconnectDelayMs;
    offlineSinceMs = 0;
    Logger::info("[Cellular] MQTT online");
    Metrics::onMqttReconnectResult(true, "ok");
    return true;
}

void CellularManager::scheduleNextReconnect() {
    nextReconnectAt = millis() + reconnectDelayMs;
    reconnectDelayMs = min(reconnectDelayMs * 2UL, 120000UL);
    useBackupBroker = !useBackupBroker;
}

void CellularManager::recoverModem() {
    Logger::warn("[Cellular] offline 10min, recovering modem");
    sendATCommand("AT+CFUN=1,1", "OK", 2000);
    delay(6000);
    while (Serial2.available()) {
        Serial2.read();
    }
    mqttState = 0;
    reconnectDelayMs = 5000;
    nextReconnectAt = millis() + reconnectDelayMs;
    useBackupBroker = false;
    offlineSinceMs = millis();
}

void CellularManager::update() {
    if (!modemReady) {
        updateModemInit();
        return;
    }

    while (Serial2.available()) {
        const char c = static_cast<char>(Serial2.read());
        if (c == '\n') {
            buffer.trim();
            if (buffer.length() > 0) {
                processLine(buffer);
            }
            buffer = "";
        } else {
            buffer += c;
        }
    }

    if (millis() - lastMqttCheck > 1000) {
        if (mqttState == 0) {
            if (offlineSinceMs == 0) {
                offlineSinceMs = millis();
            }
            if (millis() - offlineSinceMs >= 600000UL) {
                recoverModem();
            } else if (millis() >= nextReconnectAt) {
                if (!connectMQTT()) {
                    scheduleNextReconnect();
                }
            }
        } else {
            offlineSinceMs = 0;
        }
        lastMqttCheck = millis();
    }
}

void CellularManager::processLine(String line) {
    if (line.indexOf("CLOSED") != -1 || line.indexOf("DISCONNECT") != -1) {
        mqttState = 0;
        if (offlineSinceMs == 0) {
            offlineSinceMs = millis();
        }
        scheduleNextReconnect();
        Logger::warn("[Cellular] connection closed, reconnect scheduled");
        return;
    }

    if (!line.startsWith("+MSUB:")) {
        return;
    }

    const int firstQuote = line.lastIndexOf('"');
    const int secondQuote = line.lastIndexOf('"', firstQuote - 1);
    if (firstQuote == -1 || secondQuote == -1) {
        return;
    }

    String payload = line.substring(secondQuote + 1, firstQuote);
    Logger::info("[Cellular] cmd: " + payload);

    if (!payload.startsWith("START")) {
        return;
    }

    String pwd = payload.substring(6);
    pwd.trim();
    if (pwd != start_pwd) {
        Logger::error("[Cellular] wrong start password");
        publishStatus("ERROR_WRONG_PWD");
        return;
    }

    if (batteryVoltage.isLowForRemoteStart()) {
        Logger::error("[Cellular] start denied low battery (" + String(batteryVoltage.getVoltage(), 2) + "V)");
        publishStatus("ERROR_LOW_BATTERY");
        return;
    }

    if (TaskManager::sendVehicleCommand(VehicleCommandType::VEHICLE_CMD_START_FROM_MQTT, 20)) {
        publishStatus("ENGINE_START_ACCEPTED");
    } else {
        publishStatus("ERROR_QUEUE_BUSY");
    }
}

void CellularManager::publishStatus(String msg) {
    if (mqttState != 1) {
        return;
    }
    Serial2.printf("AT+MPUB=\"%s\",0,0,\"%s\"\r\n", topic_pub, msg.c_str());
}

#endif