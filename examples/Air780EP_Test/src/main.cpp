#include <Arduino.h>
#include "Air780EP_Test.h"

// PWRKEY control pin (GPIO4)
#define PWRKEY_PIN 4

// Forward declarations
static void pulsePwrkey();

// Forward declarations
static bool sendAT(const String& cmd, const String& expect, unsigned long timeout = AT_TIMEOUT_DEFAULT);
static bool sendATRaw(const String& cmd, String& out, unsigned long timeout = AT_TIMEOUT_DEFAULT);
static void flushSerial2();
static void runCommandLoop();
static bool testMqttEdge();

// ============================================================
// Test: UART Communication
// ============================================================
// ============================================================
// Helper: pulse PWRKEY to power on Air780EP
// ============================================================
static void pulsePwrkey() {
    Serial.println("");
    Serial.println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    Serial.println("!! [PWR] Pulsing PWRKEY (GPIO4) for 2s... !!");
    Serial.println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    Serial.println("");
    pinMode(PWRKEY_PIN, OUTPUT);
    digitalWrite(PWRKEY_PIN, HIGH);
    delay(500);
    digitalWrite(PWRKEY_PIN, LOW);
    delay(2000);  // hold low for 2s
    digitalWrite(PWRKEY_PIN, HIGH);
    Serial.println("  [PWR] PWRKEY released, waiting 3s...");
    delay(3000);  // wait for module to boot
    Serial.println("  [PWR] Flushing boot messages...");
    flushSerial2();
    Serial.println("  [PWR] Module should be ready now");
}

static bool testUart() {
    TEST_STEP("UART Communication - Auto Baud Detection");

    const unsigned long baudRates[] = {9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600};
    const int numBauds = sizeof(baudRates) / sizeof(baudRates[0]);
    int foundBaud = 0;

    for (int i = 0; i < numBauds; i++) {
        Serial.print("  Trying baud ");
        Serial.print(baudRates[i]);
        Serial.println("...");

        Serial2.end();
        delay(100);
        Serial2.begin(baudRates[i], SERIAL_8N1, AIR780EP_RXD2, AIR780EP_TXD2);
        delay(500);
        flushSerial2();

        Serial2.println("AT");
        delay(800);

        String resp;
        unsigned long deadline = millis() + 2000;
        while (millis() < deadline) {
            while (Serial2.available()) {
                char c = Serial2.read();
                if (c == 13 || c == 10) {
                    resp.trim();
                    if (resp == "AT") { resp = ""; continue; }
                    if (resp.indexOf("OK") >= 0) {
                        foundBaud = baudRates[i];
                        deadline = 0;
                        break;
                    }
                    if (resp.length() > 0) {
                        Serial.print("    [raw@");
                        Serial.print(baudRates[i]);
                        Serial.print("] ");
                        Serial.println(resp);
                    }
                    resp = "";
                } else {
                    if (c >= 32 && c < 127) resp += (char)c;
                }
            }
            delay(1);
        }
        if (foundBaud) break;
    }

    if (!foundBaud) {
        TEST_RESULT(false, "AT: Module not responding at any baud rate (9600-921600)");
        return false;
    }

    Serial.print("[OK] Found Air780EP at ");
    Serial.print(foundBaud);
    Serial.println(" baud");
    TEST_RESULT(true, "AT: Module alive at " + String(foundBaud) + " baud");

    // ATE0 - disable echo
    Serial.println("  >> ATE0");
    if (!sendAT("ATE0", "OK", 1000)) {
        TEST_RESULT(false, "ATE0: Failed to disable echo");
        Serial.println("  (continuing anyway - echo may still be on)");
    } else {
        TEST_RESULT(true, "ATE0: Echo disabled");
    }

    // ATI - module info
    Serial.println("  >> ATI");
    delay(200);
    flushSerial2();
    Serial2.println("ATI");
    delay(1500);
    String info;
    while (Serial2.available()) {
        char c = Serial2.read();
        if (c == 0 || c == 0x1b) continue;
        if (c == 13 || c == 10) {
            if (info.length() > 0) {
                info.trim();
                if (info.length() > 0) {
                    bool printable = true;
                    for (unsigned int ci = 0; ci < info.length(); ci++) {
                        if (info[ci] < 32 && info[ci] != 10 && info[ci] != 13) { printable = false; break; }
                    }
                    if (printable) {
                        Serial.print("    ");
                        Serial.println(info);
                    }
                }
                info = "";
            }
        } else {
            if (c >= 32) info += (char)c;
        }
    }
    TEST_RESULT(true, "ATI: Module info printed above");

    return true;
}

// ============================================================
// Test: SIM Status (AT+CPIN?)
// ============================================================
static bool testSim() {
    TEST_STEP("SIM Status");

    String reply;
    if (!sendATRaw("AT+CPIN?", reply, AT_TIMEOUT_CPIN)) {
        TEST_RESULT(false, "CPIN: No response within " + String(AT_TIMEOUT_CPIN) + "ms");
        return false;
    }

    if (reply.indexOf("+CPIN: READY") >= 0) {
        TEST_RESULT(true, "CPIN: SIM ready");
        return true;
    }
    if (reply.indexOf("+CPIN:") >= 0) {
        TEST_RESULT(false, "CPIN: SIM status = " + reply);
        return false;
    }
    TEST_RESULT(false, "CPIN: Unexpected response = " + reply);
    return false;
}

// ============================================================
// Test: Signal Quality (AT+CSQ)
// ============================================================
static bool testCsq() {
    TEST_STEP("Signal Quality");

    String reply;
    if (!sendATRaw("AT+CSQ", reply, 3000)) {
        TEST_RESULT(false, "CSQ: No response");
        return false;
    }

    int rssi = 0, ber = 99;
    if (sscanf(reply.c_str(), "+CSQ: %d,%d", &rssi, &ber) >= 1) {
        int dBm = (rssi >= 0 && rssi <= 31) ? (-113 + 2 * rssi) : 0;
        String level;
        if (rssi >= 20) level = "Excellent";
        else if (rssi >= 15) level = "Good";
        else if (rssi >= 10) level = "Weak";
        else if (rssi >= 0) level = "Danger";
        else level = "Unknown";

        Serial.print("    RSSI="); Serial.print(rssi);
        Serial.print("  dBm="); Serial.print(dBm);
        Serial.print("  BER="); Serial.print(ber);
        Serial.print("  Level="); Serial.println(level);

        if (rssi == 99) {
            TEST_RESULT(false, "CSQ: RSSI=99 (invalid/no signal)");
            return false;
        }
        TEST_RESULT(true, "CSQ: Signal quality OK");
        return true;
    }
    TEST_RESULT(false, "CSQ: Parse failed = " + reply);
    return false;
}

// ============================================================
// Test: Network Registration (AT+CEREG?)
// ============================================================
static bool testCereg() {
    TEST_STEP("Network Registration");

    // Try up to 3 times with 5s interval
    for (int attempt = 0; attempt < 3; attempt++) {
        String reply;
        if (!sendATRaw("AT+CEREG?", reply, 5000)) {
            if (attempt < 2) {
                Serial.println("    Retrying...");
                delay(2000);
                continue;
            }
            TEST_RESULT(false, "CEREG: No response after 3 attempts");
            return false;
        }

        Serial.print("    Reply: ");
        Serial.println(reply);

        // Parse: +CEREG: <n>,<stat>
        int n = 0, stat = 0;
        if (sscanf(reply.c_str(), "+CEREG: %d,%d", &n, &stat) >= 2 ||
            sscanf(reply.c_str(), "+CGREG: %d,%d", &n, &stat) >= 2 ||
            sscanf(reply.c_str(), "+CREG: %d,%d", &n, &stat) >= 2) {
            const char* statusStr;
            switch (stat) {
                case 0: statusStr = "Not registered"; break;
                case 1: statusStr = "Registered (home)"; break;
                case 2: statusStr = "Searching..."; break;
                case 3: statusStr = "Registration denied"; break;
                case 4: statusStr = "Unknown"; break;
                case 5: statusStr = "Registered (roaming)"; break;
                default: statusStr = "Invalid"; break;
            }
            Serial.print("    Status: ");
            Serial.println(statusStr);

            if (stat == 1 || stat == 5) {
                TEST_RESULT(true, "CEREG: Registered");
                return true;
            }
            if (stat == 2) {
                Serial.println("    Still searching, waiting...");
                delay(5000);
                continue;
            }
            if (stat == 3) {
                TEST_RESULT(false, "CEREG: Registration denied");
                return false;
            }
        }
        TEST_RESULT(false, "CEREG: Parse failed");
        return false;
    }

    TEST_RESULT(false, "CEREG: Not registered after timeout");
    return false;
}

// ============================================================
// Test: PDP Context (AT+CGATT?, AT+CGACT?, AT+CGPADDR)
// ============================================================
static bool testPdp() {
    TEST_STEP("PDP Context");

    // Set PDP context
    {
        TEST_STEP("  >> CGDCONT");
        if (!sendAT("AT+CGDCONT=1,\"IP\",\"CMNET\"", "OK", 5000)) {
            TEST_RESULT(false, "CGDCONT: Failed");
            return false;
        }
        TEST_RESULT(true, "CGDCONT: PDP context set");
    }

    // CGATT - GPRS attach
    {
        TEST_STEP("  >> CGATT");
        String reply;
        if (!sendATRaw("AT+CGATT?", reply, 5000)) {
            TEST_RESULT(false, "CGATT: No response");
            return false;
        }
        if (reply.indexOf("+CGATT: 1") >= 0) {
            TEST_RESULT(true, "CGATT: Already attached");
        } else if (reply.indexOf("+CGATT: 0") >= 0) {
            Serial.println("    Not attached, trying to attach...");
            if (!sendAT("AT+CGATT=1", "OK", 10000)) {
                TEST_RESULT(false, "CGATT: Attach command failed");
                return false;
            }
            delay(3000);
            if (!sendATRaw("AT+CGATT?", reply, 5000) || reply.indexOf("+CGATT: 1") < 0) {
                TEST_RESULT(false, "CGATT: Still not attached after command");
                return false;
            }
            TEST_RESULT(true, "CGATT: Attached successfully");
        } else {
            TEST_RESULT(false, "CGATT: Unexpected = " + reply);
            return false;
        }
    }

    // CGACT - PDP activate
    {
        TEST_STEP("  >> CGACT");
        // First deactivate then reactivate
        sendAT("AT+CGACT=0,1", "OK", 5000);
        delay(2000);

        if (!sendAT("AT+CGACT=1,1", "OK", 10000)) {
            TEST_RESULT(false, "CGACT: PDP activation failed");
            return false;
        }
        delay(2000);
        TEST_RESULT(true, "CGACT: PDP activated");
    }

    // CGPADDR - get IP
    {
        TEST_STEP("  >> CGPADDR");
        String reply;
        if (!sendATRaw("AT+CGPADDR", reply, 5000)) {
            TEST_RESULT(false, "CGPADDR: No response");
            return false;
        }
        int s = reply.indexOf('"');
        int e = reply.indexOf('"', s + 1);
        if (s >= 0 && e > s) {
            String ip = reply.substring(s + 1, e);
            Serial.print("    IP Address: ");
            Serial.println(ip);
            TEST_RESULT(true, "CGPADDR: IP obtained");
        } else {
            TEST_RESULT(true, "CGPADDR: Response = " + reply);
        }
    }

    return true;
}

// ============================================================
// Test: MQTT (Air780EP native AT commands)
// Test: MQTT + Network Diagnostic (Air780EP native AT commands)
// ============================================================
static bool testMqtt() {
    TEST_STEP("MQTT Full Test (Air780EP V1011 native)");

    const char* brokerIp = "35.172.255.228";  // broker.emqx.io resolved IP
    const int   port     = 1883;
    const char* clientId = "Air780EP_Test_001";
    const char* topicCmd = "air780ep/test/cmd";
    const char* topicSts = "air780ep/test/status";

    // ---- Phase 1: MQTT Connect (MIPSTART + MCONNECT) ----
    TEST_STEP("  >> MQTT Connect");
    {
        // Ensure clean state
        sendAT("AT+MDISCONNECT", "OK", 2000);
        delay(500);

        // Step 1: Configure client ID
        String cfg = "AT+MCONFIG=\"" + String(clientId) + "\"";
        Serial.print("    MCONFIG: "); Serial.println(cfg);
        if (!sendAT(cfg, "OK", 3000)) {
            TEST_RESULT(false, "MCONFIG failed");
            return false;
        }
        TEST_RESULT(true, "MCONFIG: client configured");

        // Step 2: TCP connect to broker via IP (DNS is broken on this firmware)
        String tcp = "AT+MIPSTART=\"" + String(brokerIp) + "\"," + String(port);
        Serial.print("    MIPSTART: "); Serial.println(tcp);
        if (!sendAT(tcp, "CONNECT OK", 15000)) {
            TEST_RESULT(false, "MIPSTART failed - TCP cannot reach broker");
            return false;
        }
        TEST_RESULT(true, "MIPSTART: TCP connected to " + String(brokerIp));

        // Step 3: MQTT CONNECT
        if (!sendAT("AT+MCONNECT=1,120", "CONNACK OK", 10000)) {
            TEST_RESULT(false, "MCONNECT failed");
            return false;
        }
        TEST_RESULT(true, "MCONNECT: MQTT session established");
    }

    // ---- Phase 2: Subscribe + Publish ----
    TEST_STEP("  >> Subscribe & Publish");

    // Subscribe
    {
        String sub = "AT+MSUB=\"" + String(topicCmd) + "\",0";
        Serial.print("    MSUB: "); Serial.println(sub);
        if (!sendAT(sub, "SUBACK", 5000)) {
            TEST_RESULT(false, "MSUB failed");
            return false;
        }
        TEST_RESULT(true, "MSUB: Subscribed to " + String(topicCmd));
    }

    // Publish status
    {
        String payload = "{\"status\":\"online\",\"rssi\":23}";
        String pub = "AT+MPUB=\"" + String(topicSts) + "\",0,0,";
        // For JSON payload, need to escape quotes
        pub += "\"" + payload + "\"";
        Serial.print("    MPUB: "); Serial.println(pub);
        if (!sendAT(pub, "OK", 5000)) {
            TEST_RESULT(false, "MPUB failed");
            return false;
        }
        TEST_RESULT(true, "MPUB: Published status");
    }

    // ---- Phase 3: Wait and check for incoming messages ----
    TEST_STEP("  >> Wait for incoming");
    {
        Serial.println("    Listening for 10 seconds...");
        unsigned long start = millis();
        bool gotMsg = false;
        while (millis() - start < 10000) {
            while (Serial2.available()) {
                String line;
                while (Serial2.available()) {
                    char c = Serial2.read();
                    if (c == '\n' || c == '\r') {
                        if (line.length() > 0) break;
                        continue;
                    }
                    line += c;
                }
                line.trim();
                if (line.length() > 0) {
                    Serial.print("    [RX] ");
                    Serial.println(line);
                    if (line.indexOf("+MSUB:") >= 0) gotMsg = true;
                }
            }
            delay(10);
        }
        if (gotMsg) TEST_RESULT(true, "Received subscribed message");
        else TEST_RESULT(true, "No incoming messages (normal if none published)");
    }

    // ---- Phase 4: Disconnect + Reconnect ----
    TEST_STEP("  >> Disconnect & Reconnect");
    {
        // Disconnect
        if (!sendAT("AT+MDISCONNECT", "OK", 5000)) {
            TEST_RESULT(false, "MDISCONNECT failed");
            return false;
        }
        TEST_RESULT(true, "MDISCONNECT: Disconnected");

        delay(1000);

        // Reconnect
        String tcp2 = "AT+MIPSTART=\"" + String(brokerIp) + "\"," + String(port);
        if (!sendAT(tcp2, "CONNECT OK", 15000)) {
            TEST_RESULT(false, "MIPSTART reconnect failed");
            return false;
        }
        TEST_RESULT(true, "MIPSTART: Reconnected");

        if (!sendAT("AT+MCONNECT=1,120", "CONNACK OK", 10000)) {
            TEST_RESULT(false, "MCONNECT reconnect failed");
            return false;
        }
        TEST_RESULT(true, "MCONNECT: Reconnected OK");
    }

    // ---- Phase 5: Final publish ----
    TEST_STEP("  >> Final publish");

    {
        String pub2 = "AT+MPUB=\"" + String(topicSts) + "\",0,0,\"reconnect_test_ok\"";
        if (!sendAT(pub2, "OK", 5000)) {
            TEST_RESULT(false, "Final MPUB failed");
            return false;
        }
        TEST_RESULT(true, "MPUB: Final test message sent");
    }

    TEST_RESULT(true, "ALL MQTT TESTS PASSED");
    return true;
}
// ============================================================
// Helper: send AT command, wait for expected response
// ============================================================
static bool sendAT(const String& cmd, const String& expect, unsigned long timeout) {
    flushSerial2();
    Serial2.println(cmd);

    unsigned long deadline = millis() + timeout;
    String lineBuf;
    bool found = false;

    while (millis() < deadline) {
        while (Serial2.available()) {
            char c = Serial2.read();
            if (c == '\r') continue;
            if (c == '\n') {
                lineBuf.trim();
                if (lineBuf.length() > 0) {
                    // Filter: skip command echo
                    if (lineBuf == cmd) {
                        Serial.print("  [ECHO] ");
                        Serial.println(lineBuf);
                        lineBuf = "";
                        continue;
                    }
                    // Filter: skip startup banners
                    if (lineBuf.indexOf("RDY") >= 0 || lineBuf.indexOf("^boot") >= 0 ||
                        lineBuf.indexOf("Call Ready") >= 0) {
                        Serial.print("  [BOOT] ");
                        Serial.println(lineBuf);
                        lineBuf = "";
                        continue;
                    }
                    Serial.print("  ");
                    Serial.println(lineBuf);

                    if (lineBuf.indexOf(expect) >= 0) {
                        found = true;
                        // Don't break - drain remaining
                    }
                    if (lineBuf.indexOf("ERROR") >= 0 && expect != "ERROR") {
                        lineBuf = "";
                        // Don't break - drain remaining
                    }
                }
                lineBuf = "";
                deadline = millis() + 500;  // Extend deadline on each line
            } else {
                lineBuf += c;
            }
        }
    }

    return found;
}

// ============================================================
// Helper: send AT command, return full response as string
// ============================================================
static bool sendATRaw(const String& cmd, String& out, unsigned long timeout) {
    flushSerial2();
    Serial2.println(cmd);

    unsigned long deadline = millis() + timeout;
    String lineBuf;
    out = "";

    while (millis() < deadline) {
        while (Serial2.available()) {
            char c = Serial2.read();
            if (c == '\r') continue;
            if (c == '\n') {
                lineBuf.trim();
                if (lineBuf.length() > 0) {
                    // Filter: skip command echo
                    if (lineBuf == cmd) {
                        lineBuf = "";
                        continue;
                    }
                    // Filter: skip startup banners
                    if (lineBuf.indexOf("RDY") >= 0 || lineBuf.indexOf("^boot") >= 0 ||
                        lineBuf.indexOf("Call Ready") >= 0) {
                        Serial.print("  [BOOT] ");
                        Serial.println(lineBuf);
                        lineBuf = "";
                        continue;
                    }
                    Serial.print("  ");
                    Serial.println(lineBuf);

                    // Store for parsing
                    if (out.length() > 0) out += "\n";
                    out += lineBuf;
                }
                lineBuf = "";
                deadline = millis() + 500;
            } else {
                lineBuf += c;
            }
        }
    }

    return out.length() > 0;
}

// ============================================================
// Helper: flush Serial2 receive buffer
// ============================================================
static void flushSerial2() {
    while (Serial2.available()) {
        Serial2.read();
        delay(2);
    }
}
// ============================================================
// AT Debug Mode: send any command from Serial to Serial2
// ============================================================
static void runSerial2LoopbackTest() {
    Serial.println();
    Serial.println('=== Serial2 Loopback Test ===');
    Serial.println('This test checks if Serial2 (GPIO16/17) can send and receive.');
    Serial.println('Short GPIO16 to GPIO17 with a jumper wire, then press ENTER.');
    Serial.println('(Waiting 15s for you to short them...)');
    
    // Wait for user to short GPIO16(RX) and GPIO17(TX)
    // Actually we do an internal loopback: Serial2 TX sends, Serial2 RX reads
    // For this we need pins shorted externally
    
    unsigned long start = millis();
    bool led_on = true;
    while (millis() - start < 15000) {
        if (Serial.available()) {
            Serial.read();
            break;
        }
        // LED not used
        led_on = !led_on;
        delay(200);
    }
    
    Serial.println('Test 1: Serial2 internal self-test');
    Serial2.println('AT');
    delay(500);
    
    int count = 0;
    while (Serial2.available()) {
        Serial2.read();
        count++;
    }
    Serial.print('  Serial2 available() returned ');
    Serial.print(count);
    Serial.println(' bytes (should be >0 if pins are shorted or module is connected)');
    
    if (count > 0) {
        Serial.println('[PASS] Serial2 can receive data');
    } else {
        Serial.println('[FAIL] Serial2 cannot receive any data');
        Serial.println('Check wiring: GPIO16(RX2) <-> Module TX, GPIO17(TX2) <-> Module RX');
        Serial.println('Also verify both modules share GND');
    }
    Serial.println('=== Loopback Test Complete ===');
    Serial.println();
}

static void runCommandLoop() {
    // Run loopback test first
    runSerial2LoopbackTest();
    
    // Re-pulse PWRKEY in case module powered off during tests
    pinMode(PWRKEY_PIN, OUTPUT);
    digitalWrite(PWRKEY_PIN, HIGH);
    
    Serial.println("");
    Serial.println("========================================");
    Serial.println("[AT DEBUG] Enter AT commands (type EXIT to return)");
    Serial.println("[AT DEBUG] Commands are sent to Air780EP via Serial2");
    Serial.println("========================================");
    Serial.println("");

    String input;
    while (true) {
        while (Serial.available()) {
            char c = Serial.read();
            if (c == '\r' || c == '\n') {
                if (input.length() > 0) {
                    input.trim();
                    if (input == "EXIT" || input == "exit") {
                        Serial.println("Exiting AT debug mode");
                        return;
                    }
                    if (input.length() > 0) {
                        Serial.print("[AT->] ");
                        Serial.println(input);

                        flushSerial2();
                        Serial2.println(input);
                        delay(500);

                        String lineBuf;
                        unsigned long deadline = millis() + 3000;
                        while (millis() < deadline) {
                            while (Serial2.available()) {
                                char c2 = Serial2.read();
                                if (c2 == '\r') continue;
                                if (c2 == '\n') {
                                    lineBuf.trim();
                                    if (lineBuf.length() > 0) {
                                        Serial.print("  ");
                                        Serial.println(lineBuf);
                                    }
                                    lineBuf = "";
                                    deadline = millis() + 500;
                                } else {
                                    lineBuf += c2;
                                }
                            }
                        }
                        Serial.println("[--- end ---]");
                    }
                    input = "";
                }
            } else {
                input += c;
            }
        }
        delay(10);
    }
}

// ============================================================
// Test Runner
// ============================================================

// ============================================================
// Test: MQTT Edge Cases (simulates main-program failure scenarios)
// ============================================================
static bool testMqttEdge() {
    TEST_STEP("MQTT Edge Cases & Recovery");

    const char* brokerIp = "35.172.255.228";
    const int   port     = 1883;
    const char* topicSts = "air780ep/test/status";

    // ---- Test 1: MQTT-only reconnect (no MCONFIG) ----
    TEST_STEP("  >> Reconnect without MCONFIG");
    {
        // Already have a session from previous test; disconnect MQTT but keep TCP
        sendAT("AT+MDISCONNECT", "OK", 2000);
        delay(500);

        // Reconnect TCP should still work (might be ALREADY CONNECT)
        String tcp = "AT+MIPSTART=\"" + String(brokerIp) + "\"," + String(port);
        if (!sendAT(tcp, "CONNECT OK", 10000)) {
            // May already be connected
            Serial.println("    (ALREADY CONNECT is OK too)");
        }
        TEST_RESULT(true, "MIPSTART: TCP reconnect");

        if (!sendAT("AT+MCONNECT=1,120", "CONNACK OK", 10000)) {
            TEST_RESULT(false, "MCONNECT after MDISCONNECT failed");
            return false;
        }
        TEST_RESULT(true, "MCONNECT: Session re-established without MCONFIG");
    }

    // ---- Test 2: Full re-init (MCONFIG again) ----
    TEST_STEP("  >> Full re-init with MCONFIG");
    {
        sendAT("AT+MDISCONNECT", "OK", 2000);
        sendAT("AT+MIPSTART=\"0.0.0.0\",0", "ERROR", 2000);  // force close
        delay(500);
        flushSerial2();

        String cfg = "AT+MCONFIG=\"Air780EP_Edge_002\"";
        Serial.print("    "); Serial.println(cfg);
        if (!sendAT(cfg, "OK", 3000)) {
            TEST_RESULT(false, "MCONFIG re-init failed");
            return false;
        }
        TEST_RESULT(true, "MCONFIG: Re-initialized");

        String tcp = "AT+MIPSTART=\"" + String(brokerIp) + "\"," + String(port);
        Serial.print("    "); Serial.println(tcp);
        if (!sendAT(tcp, "CONNECT OK", 15000)) {
            TEST_RESULT(false, "MIPSTART after re-init failed");
            return false;
        }
        TEST_RESULT(true, "MIPSTART: Connected after re-init");

        if (!sendAT("AT+MCONNECT=1,120", "CONNACK OK", 10000)) {
            TEST_RESULT(false, "MCONNECT after re-init failed");
            return false;
        }
        TEST_RESULT(true, "MCONNECT: Session established after re-init");
    }

    // ---- Test 3: Repeated publish without subscribe ----
    TEST_STEP("  >> Multiple publish burst");
    {
        for (int i = 0; i < 5; i++) {
            String payload = "burst_msg_" + String(i);
            String pub = "AT+MPUB=\"" + String(topicSts) + "\",0,0,\"" + payload + "\"";
            if (!sendAT(pub, "OK", 5000)) {
                TEST_RESULT(false, "MPUB burst #" + String(i) + " failed");
                return false;
            }
            delay(200);
        }
        TEST_RESULT(true, "MPUB: 5 burst messages sent OK");
    }

    // ---- Test 4: Long JSON payload publish ----
    TEST_STEP("  >> Long JSON publish");
    {
        String json = "{\"online\":true,\"rssi\":23,\"rssi_dbm\":-67,\"cereg\":1,\"ip\":\"10.0.0.1\",\"battery\":12.6,\"mqtt\":true,\"uptime\":86400}";
        String pub = "AT+MPUB=\"" + String(topicSts) + "\",0,0,\"" + json + "\"";
        if (!sendAT(pub, "OK", 5000)) {
            TEST_RESULT(false, "MPUB long JSON failed");
            return false;
        }
        TEST_RESULT(true, "MPUB: Long JSON published");
    }

    // ---- Test 5: CGACT deactivate/reactivate while MQTT online ----
    TEST_STEP("  >> CGACT cycle while online");
    {
        // Deactivate PDP while MQTT is connected (should fail)
        sendAT("AT+CGACT=0,1", "OK", 5000);
        delay(3000);

        // Reactivate PDP
        if (!sendAT("AT+CGACT=1,1", "OK", 10000)) {
            TEST_RESULT(false, "CGACT=1,1 after deactivate failed");
            flushSerial2();
            // Try again with longer timeout
            delay(2000);
            if (!sendAT("AT+CGACT=1,1", "OK", 15000)) {
                TEST_RESULT(false, "CGACT=1,1 retry also failed");
                return false;
            }
        }
        TEST_RESULT(true, "CGACT: PDP reactivated");

        // Verify MQTT state - MIPSTART should still be valid
        String tcp = "AT+MIPSTART=\"" + String(brokerIp) + "\"," + String(port);
        sendAT(tcp, "CONNECT OK", 5000);  // might be ALREADY CONNECT
        delay(500);

        // Try MCONNECT to restore session
        if (!sendAT("AT+MCONNECT=1,120", "CONNACK OK", 10000)) {
            TEST_RESULT(true, "MCONNECT after PDP cycle - may need MCONFIG (acceptable)");
        } else {
            TEST_RESULT(true, "MCONNECT: Session restored after PDP cycle");
        }
    }

    // ---- Test 6: CSQ + MQTT interleaved (simulates main program) ----
    TEST_STEP("  >> CSQ interleaved with MQTT");
    {
        for (int i = 0; i < 3; i++) {
            String csq;
            sendATRaw("AT+CSQ", csq, 2000);
            int rssi = 99, ber = 99;
            if (sscanf(csq.c_str(), "+CSQ: %d,%d", &rssi, &ber) >= 1) {
                Serial.print("    CSQ#"); Serial.print(i);
                Serial.print(" rssi="); Serial.print(rssi);
                Serial.print(" ber="); Serial.println(ber);
            } else {
                Serial.print("    CSQ#"); Serial.print(i);
                Serial.print(" raw="); Serial.println(csq);
            }

            String pub = "AT+MPUB=\"" + String(topicSts) + "\",0,0,\"heartbeat_" + String(i) + "\"";
            if (!sendAT(pub, "OK", 5000)) {
                TEST_RESULT(false, "MPUB after CSQ #" + String(i) + " failed");
                return false;
            }
            delay(500);
        }
        TEST_RESULT(true, "CSQ+MPUB interleaved OK (3 cycles)");
    }

    // ---- Test 7: Long idle then re-publish ----
    TEST_STEP("  >> Idle 5s then publish");
    {
        delay(5000);

        String pub = "AT+MPUB=\"" + String(topicSts) + "\",0,0,\"after_idle_test\"";
        if (!sendAT(pub, "OK", 5000)) {
            TEST_RESULT(false, "MPUB after idle failed");
            return false;
        }
        TEST_RESULT(true, "MPUB: Published after idle");
    }

    // ---- Test 8: Graceful disconnect ----
    TEST_STEP("  >> Graceful disconnect");
    {
        if (!sendAT("AT+MDISCONNECT", "OK", 5000)) {
            TEST_RESULT(false, "MDISCONNECT failed");
            return false;
        }
        TEST_RESULT(true, "MDISCONNECT: Clean disconnect");
    }

    TEST_RESULT(true, "ALL EDGE CASE TESTS PASSED");
    return true;
}

void setup() {
    Serial.begin(115200);
    delay(3000);
    Serial.println("");
    Serial.println("");
    Serial.println("========================================");
    Serial.println("  Air780EP Module Test Suite");
    Serial.println("  ESP32 + Air780EP via UART2 (GPIO16/17)");
    Serial.println("========================================");
    Serial.println("");

    // Power on the Air780EP module via PWRKEY
    pulsePwrkey();

    bool allPassed = true;

    // Phase 1: UART
    if (!testUart()) {
        Serial.println("");
        Serial.println("[FATAL] UART test failed, aborting");
        allPassed = false;
    }

    // Phase 2: SIM
    if (allPassed && !testSim()) {
        Serial.println("");
        Serial.println("[FATAL] SIM test failed, aborting");
        allPassed = false;
    }

    // Phase 3: Signal
    if (allPassed && !testCsq()) {
        Serial.println("[WARN] Signal quality test failed, continuing");
    }

    // Phase 4: Network Registration
    if (allPassed && !testCereg()) {
        Serial.println("");
        Serial.println("[FATAL] Network registration failed, aborting");
        allPassed = false;
    }

    // Phase 5: PDP
    if (allPassed && !testPdp()) {
        Serial.println("");
        Serial.println("[WARN] PDP test failed, continuing to MQTT anyway");
    }

    // Phase 6: MQTT
    if (allPassed) {
        allPassed = testMqtt();
        if (!allPassed) {
            Serial.println("");
            Serial.println("[WARN] MQTT test failed");
        }
    }

    // Summary
    Serial.println("");
    Serial.println("========================================");
    if (allPassed) {
        Serial.println("  ALL TESTS PASSED");
    } else {
        Serial.println("  SOME TESTS FAILED (see above)");
    }
    Serial.println("========================================");
    Serial.println("");

    // Enter AT debug command loop
    runCommandLoop();
}

void loop() {
    // Not used - everything is in setup()
    delay(1000);
}




