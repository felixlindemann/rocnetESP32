/*
 * ==============================================================================
 * QUELLENNACHWEIS / SOURCE ATTRIBUTION:
 * ==============================================================================
 * RocNet ESP32 Controller Hauptprogramm
 *
 * Protokoll & Netzwerk:
 *   - Implementiert die RocNet-Multicast-Kommunikation gemäß Spezifikation:
 *     https://www.rocrail-wiki.net/doku.php?id=rocnet:rocnet-prot-en
 *   - Urheber: Copyright (c) 2002-2026 Robert Jan Versluis, Rocrail.net
 *   - Konstanten & Telegrammstrukturen: lib/rocnet-const.h (Copied from Rocrail)
 *   - Offline-Dokumentation: literature/rocnet_rocnet-prot-en [Innovative Model Railroad Control System].html
 *
 * Externe Bibliotheken & Treiber:
 *   - Adafruit PWM Servo Driver Library (BSD-Lizenz, Adafruit Industries)
 *   - NeoPixelBus by Makuna (LGPLv3)
 *   - ESP32 Arduino Core (Espressif Systems / Arduino)
 * ==============================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESPmDNS.h>
#include "Config.h"
#include "RocNetParser.h"
#include "SignalOutput.h"
#include "ServoController.h"
#include "WebServerManager.h"
#include "SystemConfig.h"
#include "AppLog.h"

// ==============================================================================
// Global Objects
// ==============================================================================
WiFiUDP udp;
RocNetParser rocNetParser(DEFAULT_ROCNET_NODE_ID);
SignalOutput signalOutput(WS2811_DATA_PIN);
ServoController servoController;
WebServerManager webServerManager;

// Non-blocking WiFi reconnect state
static uint32_t lastWiFiCheck = 0;
constexpr uint32_t WIFI_RECONNECT_INTERVAL_MS = 5000;

// Non-blocking status heartbeat
static uint32_t lastHeartbeat = 0;
constexpr uint32_t HEARTBEAT_INTERVAL_MS = 10000;

// WiFi & Multicast state
static bool isUdpListening = false;

// ==============================================================================
// Function Prototypes
// ==============================================================================
void connectWiFi();
void checkWiFiConnection();
void setupMulticast();
void processUdpPackets();
void dispatchCommand(const RocNetCommand& cmd);
void sendRocNetPacket(const uint8_t* packet, size_t len);
void sendShowEvent();
void sendIdentifyReply(uint16_t recipientId = 0, uint8_t netId = 0);
void sendChannelConfigReply(uint16_t recipientId, uint8_t netId, uint8_t action, uint8_t channel);
void sendHostPong(uint16_t recipientId, uint8_t netId);
void sendPortReport(uint16_t recipientId, uint8_t netId, uint16_t port, bool thrown);
void sendOptionsReply(uint16_t recipientId, uint8_t netId);
void printBanner();

// ==============================================================================
// Setup
// ==============================================================================
void setup() {
    Serial.begin(115200);
    delay(500);

    // 0. Initialize persistent system configuration
    SystemConfig::getInstance().begin();
    const auto& sys = SystemConfig::getInstance().get();

    // Initialize 3-tier logger with configured level (Default: 1 = ERROR)
    AppLogger::getInstance().setLevel(static_cast<AppLogLevel>(sys.logLevel));

    rocNetParser.setNodeId(sys.nodeId);
    servoController.setPairedPorts(sys.pairedPorts);

    // Register turnout completion callback for sending RocNet Report events (Group 3 Action 7)
    servoController.setTurnoutCompletionCallback([](uint8_t channel, bool thrown) {
        uint16_t port = channel + 1; // 1-based port
        sendPortReport(0, 0, port, thrown);
        LOG_D("TX TURNOUT REPORT: Port %u reached end position (%s)", port, thrown ? "ABZWEIG" : "GERADE");
    });

    printBanner();

    // 1. Initialize PCA9685 Servo Controller on configured I2C pins
    LOG_I("Starting ServoController (PCA9685)...");
    if (!servoController.begin(DEFAULT_PCA9685_BASE_ADDR, sys.sdaPin, sys.sclPin)) {
        LOG_E("Failed to initialize PCA9685 on SDA=%d, SCL=%d!", sys.sdaPin, sys.sclPin);
    }

    // 2. Initialize WS2811 Signal Controller on configured GPIO via RMT
    LOG_I("Starting SignalOutput (WS2811)...");
    if (!signalOutput.begin(sys.ws2811Pin, sys.ws2811Chips, sys.signalBrightness)) {
        LOG_E("Failed to initialize WS2811 Signal Output on GPIO %d!", sys.ws2811Pin);
    } else {
        // Optional quick hardware self-test pattern
        signalOutput.testPattern();
    }

    // 3. Connect to WiFi
    connectWiFi();

    // 4. Start mDNS and WebServer for offline servo finetuning & system config
    if (WiFi.status() == WL_CONNECTED) {
        if (MDNS.begin(sys.mdnsName)) {
            MDNS.addService("http", "tcp", 80);
            LOG_I("mDNS responder started: http://%s.local/", sys.mdnsName);
        }
        webServerManager.setIdentifyCallback([]() {
            LOG_D("WebUI triggered Rocrail Identify & Show event");
            sendIdentifyReply(0);
            sendShowEvent();
        });
        webServerManager.begin(&servoController, &signalOutput, &rocNetParser);
    }

    // 5. Join Multicast Group
    setupMulticast();

    LOG_D("RocNet Node ready! ID: %u (\"%s\"), Multicast: %s:%u (LogLevel: %s)",
          sys.nodeId, sys.nodeName, ROCNET_MULTICAST_IP.toString().c_str(),
          ROCNET_MULTICAST_PORT, AppLogger::getInstance().getLevelName());
}

// ==============================================================================
// Loop
// ==============================================================================
void loop() {
    // 0. Handle Web-UI HTTP requests
    webServerManager.handleClient();

    // 1. Update Servos (Smooth non-blocking interpolation step)
    servoController.update();

    // 2. Update WS2811 Signal LEDs (Non-blocking Show())
    signalOutput.update();

    // 3. Poll and process incoming RocNet UDP packets
    processUdpPackets();

    // 4. WiFi connection monitoring
    checkWiFiConnection();

    // 5. Periodic heartbeat & RocNet announcement
    uint32_t now = millis();
    if (now - lastHeartbeat >= HEARTBEAT_INTERVAL_MS) {
        lastHeartbeat = now;
        LOG_I("Heartbeat: Node %u active, IP: %s, RSSI: %d dBm, Heap: %u",
              rocNetParser.getNodeId(), WiFi.localIP().toString().c_str(), WiFi.RSSI(), ESP.getFreeHeap());
        if (isUdpListening) {
            sendIdentifyReply(0);
        }
    }
}

// ==============================================================================
// Helper Functions
// ==============================================================================
void printBanner() {
    const auto& sys = SystemConfig::getInstance().get();
    Serial.println("\n========================================================");
    Serial.println("           RocNet ESP32 Controller Node");
    Serial.println("           Board: Wemos Lolin32 Lite");
    Serial.printf("           Node ID: %u (\"%s\")\n", sys.nodeId, sys.nodeName);
    Serial.printf("           Vendor ID: %u\n", DEFAULT_ROCNET_VENDOR_ID);
    Serial.printf("           Multicast: %s:%u\n", ROCNET_MULTICAST_IP.toString().c_str(), ROCNET_MULTICAST_PORT);
    Serial.printf("           Signals: WS2811 on GPIO %d (%u chips / %u LEDs)\n", sys.ws2811Pin, sys.ws2811Chips, sys.ws2811Chips * 3);
    Serial.printf("           Turnouts: PCA9685 (SDA=%d, SCL=%d, 50Hz, Paired=%d)\n", sys.sdaPin, sys.sclPin, sys.pairedPorts);
    Serial.printf("           LogLevel: %s (%u)\n", AppLogger::getInstance().getLevelName(), sys.logLevel);
    Serial.println("========================================================\n");
}

void connectWiFi() {
    const auto& sys = SystemConfig::getInstance().get();
    LOG_I("Connecting to WiFi SSID '%s'...", sys.wifiSsid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(sys.wifiSsid, sys.wifiPassword);

    uint32_t startAttemptTime = millis();
    while ((WiFi.status() != WL_CONNECTED || WiFi.localIP() == IPAddress(0, 0, 0, 0)) && 
           millis() - startAttemptTime < 10000) {
        delay(300);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED && WiFi.localIP() != IPAddress(0, 0, 0, 0)) {
        LOG_D("WiFi connected! IP address: %s", WiFi.localIP().toString().c_str());
    } else {
        LOG_E("Initial WiFi connection not yet established. Will reconnect in background...");
    }
}

void checkWiFiConnection() {
    uint32_t now = millis();
    if (now - lastWiFiCheck < WIFI_RECONNECT_INTERVAL_MS) return;
    lastWiFiCheck = now;

    if (WiFi.status() == WL_CONNECTED && WiFi.localIP() != IPAddress(0, 0, 0, 0)) {
        if (!isUdpListening) {
            setupMulticast();
        }
    } else {
        if (isUdpListening) {
            LOG_E("WiFi connection lost! Reconnecting...");
        }
        isUdpListening = false;
        WiFi.reconnect();
    }
}

void setupMulticast() {
    if (WiFi.status() == WL_CONNECTED && WiFi.localIP() != IPAddress(0, 0, 0, 0)) {
        udp.stop();

        bool success = false;
        // 224.0.0.1 is the RFC 1112 All-Hosts multicast address.
        // In lwIP/ESP32, 224.0.0.1 is permanently joined at the link layer.
        // Calling setsockopt(IP_ADD_MEMBERSHIP) on 224.0.0.1 returns EADDRNOTAVAIL (125).
        // Therefore, binding directly with begin(port) receives all 224.0.0.1 traffic cleanly!
        if (ROCNET_MULTICAST_IP == IPAddress(224, 0, 0, 1)) {
            success = (udp.begin(ROCNET_MULTICAST_PORT) == 1);
            if (success) {
                isUdpListening = true;
                LOG_I("UDP listening on 0.0.0.0:%u (RocNet 224.0.0.1 All-Hosts group)", ROCNET_MULTICAST_PORT);
                sendIdentifyReply(0); // Announce node to Rocrail on join!
            }
        } else {
            success = (udp.beginMulticast(ROCNET_MULTICAST_IP, ROCNET_MULTICAST_PORT) == 1);
            if (success) {
                isUdpListening = true;
                LOG_I("Joined multicast group %s on port %u",
                      ROCNET_MULTICAST_IP.toString().c_str(), ROCNET_MULTICAST_PORT);
                sendIdentifyReply(0); // Announce node to Rocrail on join!
            }
        }

        if (!success) {
            isUdpListening = false;
            LOG_E("Failed to initialize UDP listener on port %u!", ROCNET_MULTICAST_PORT);
        }
    } else {
        isUdpListening = false;
    }
}

// Track the IP and port of the computer running Rocrail for direct unicast responses
static IPAddress lastRocrailIp = IPAddress(255, 255, 255, 255);
static uint16_t lastRocrailPort = ROCNET_MULTICAST_PORT;

void sendRocNetPacket(const uint8_t* packet, size_t len) {
    if (!isUdpListening || WiFi.status() != WL_CONNECTED || len == 0) return;

    // 1. Multicast group 224.0.0.1:4321
    udp.beginPacket(ROCNET_MULTICAST_IP, ROCNET_MULTICAST_PORT);
    udp.write(packet, len);
    udp.endPacket();

    // 2. Global UDP Broadcast 255.255.255.255:4321
    udp.beginPacket(IPAddress(255, 255, 255, 255), ROCNET_MULTICAST_PORT);
    udp.write(packet, len);
    udp.endPacket();

    // 3. Subnet Broadcast (e.g. 10.10.78.255:4321)
    IPAddress bcast = WiFi.broadcastIP();
    if (bcast != IPAddress(255, 255, 255, 255) && bcast != IPAddress(0, 0, 0, 0)) {
        udp.beginPacket(bcast, ROCNET_MULTICAST_PORT);
        udp.write(packet, len);
        udp.endPacket();
    }

    // 4. Direct Unicast to Rocrail host IP (bypasses any router multicast filter)
    if (lastRocrailIp != IPAddress(0, 0, 0, 0) &&
        lastRocrailIp != IPAddress(255, 255, 255, 255) &&
        lastRocrailIp != ROCNET_MULTICAST_IP) {
        udp.beginPacket(lastRocrailIp, ROCNET_MULTICAST_PORT);
        udp.write(packet, len);
        udp.endPacket();
        if (lastRocrailPort != 0 && lastRocrailPort != ROCNET_MULTICAST_PORT) {
            udp.beginPacket(lastRocrailIp, lastRocrailPort);
            udp.write(packet, len);
            udp.endPacket();
        }
    }
}

void sendShowEvent() {
    if (!isUdpListening || WiFi.status() != WL_CONNECTED) return;
    uint8_t packet[8];
    packet[0] = 0; // NetID
    packet[1] = 0; // Broadcast recipient
    packet[2] = 0;
    packet[3] = (uint8_t)(rocNetParser.getNodeId() >> 8);
    packet[4] = (uint8_t)(rocNetParser.getNodeId() & 0xFF);
    packet[5] = RN_GROUP_STATIONARY; // 3
    packet[6] = (uint8_t)((RN_ACTIONTYPE_EVENT << 5) | RN_STATIONARY_SHOW); // 0x2B = 43
    packet[7] = 0; // Length = 0
    sendRocNetPacket(packet, 8);
    log_i("TX SHOW EVENT (Action 11, code 0x2B) -> Rocrail (popups/selects node in RocNetNode dialog)");
}

void sendIdentifyReply(uint16_t recipientId, uint8_t netId) {
    if (!isUdpListening || WiFi.status() != WL_CONNECTED) return;

    const auto& sys = SystemConfig::getInstance().get();
    const char* nodeName = sys.nodeName;
    uint8_t nameLen = (uint8_t)strlen(nodeName);

    // Standard 15-byte RocNet Identify packet according to official protocol specification
    // Header (8 bytes) + Data (7 bytes: class, manuID, versionH, versionL, nr I/O, subipH, subipL)
    uint8_t packet[32];
    memset(packet, 0, sizeof(packet));

    packet[0] = netId; // NetID (0)
    packet[1] = (uint8_t)(recipientId >> 8);
    packet[2] = (uint8_t)(recipientId & 0xFF);
    packet[3] = (uint8_t)(rocNetParser.getNodeId() >> 8);
    packet[4] = (uint8_t)(rocNetParser.getNodeId() & 0xFF);
    packet[5] = RN_GROUP_STATIONARY; // 3
    packet[6] = (uint8_t)((RN_ACTIONTYPE_REPLY << 5) | RN_STATIONARY_IDENTIFY); // 0x48 = 72
    packet[7] = 7; // Exactly 7 bytes data per RocNet specification!

    // Data: class, manuID, versionH, versionL, nr I/O, subipH, subipL
    packet[8]  = RN_CLASS_IO;                  // 1 = RasPi I/O
    packet[9]  = DEFAULT_ROCNET_VENDOR_ID;     // 0 = Rocrail
    packet[10] = DEFAULT_ROCNET_VERSION_MAJOR; // 1
    packet[11] = DEFAULT_ROCNET_VERSION_MINOR; // 0
    packet[12] = (uint8_t)(sys.ws2811Chips * 3); // nr I/O (e.g. 48)
    packet[13] = WiFi.localIP()[2];            // Subnet IP H (e.g. 78)
    packet[14] = WiFi.localIP()[3];            // Node IP L (e.g. 68)

    // 1. Send as REPLY (0x48) addressed to recipientId
    sendRocNetPacket(packet, 15);

    // 2. Also send as REPLY (0x48) broadcast (recipient = 0)
    packet[1] = 0;
    packet[2] = 0;
    sendRocNetPacket(packet, 15);

    // 3. Also send as EVENT (0x28 = 40) broadcast (recipient = 0)
    packet[6] = (uint8_t)((RN_ACTIONTYPE_EVENT << 5) | RN_STATIONARY_IDENTIFY);
    sendRocNetPacket(packet, 15);

    // 4. If recipient was specific, also send as EVENT to recipient
    if (recipientId != 0) {
        packet[1] = (uint8_t)(recipientId >> 8);
        packet[2] = (uint8_t)(recipientId & 0xFF);
        sendRocNetPacket(packet, 15);
    }

    // 5. Also send extended packet with nodeName (for versions of Rocrail supporting name)
    if (nameLen > 0) {
        uint8_t extPacket[8 + 7 + 32];
        memset(extPacket, 0, sizeof(extPacket));
        memcpy(extPacket, packet, 15);
        extPacket[1] = (uint8_t)(recipientId >> 8);
        extPacket[2] = (uint8_t)(recipientId & 0xFF);
        extPacket[6] = (uint8_t)((RN_ACTIONTYPE_REPLY << 5) | RN_STATIONARY_IDENTIFY);
        extPacket[7] = 7 + nameLen;
        memcpy(&extPacket[15], nodeName, nameLen);
        sendRocNetPacket(extPacket, 8 + 7 + nameLen);
    }

    LOG_D("TX IDENTIFY (7-byte standard + broadcast) -> NetID %u, Node %u, IP: %s, Name: %s",
          netId, rocNetParser.getNodeId(), WiFi.localIP().toString().c_str(), nodeName);
}

void processUdpPackets() {
    if (!isUdpListening || WiFi.status() != WL_CONNECTED) return;

    int packetSize = udp.parsePacket();
    if (packetSize <= 0) return;

    IPAddress senderIp = udp.remoteIP();
    uint16_t senderPort = udp.remotePort();
    if (senderIp != IPAddress(0, 0, 0, 0)) {
        lastRocrailIp = senderIp;
        lastRocrailPort = senderPort;
    }

    uint8_t packetBuffer[256];
    int bytesRead = udp.read(packetBuffer, sizeof(packetBuffer));

    if (bytesRead > 0) {
        uint16_t senderId = (bytesRead >= 5) ? (((uint16_t)packetBuffer[3] << 8) | packetBuffer[4]) : 0;
        if (senderId != rocNetParser.getNodeId()) {
            char hexBuf[128] = {0};
            int pos = 0;
            for (int i = 0; i < bytesRead && pos < (int)sizeof(hexBuf) - 4; i++) {
                pos += snprintf(hexBuf + pos, sizeof(hexBuf) - pos, "%02X ", packetBuffer[i]);
            }
            LOG_I("UDP RX (%d bytes from %s:%u): [ %s]", bytesRead, senderIp.toString().c_str(), senderPort, hexBuf);
        }

        RocNetCommand cmd;
        if (rocNetParser.parse(packetBuffer, bytesRead, cmd)) {
            dispatchCommand(cmd);
        }
    }
}

void dispatchCommand(const RocNetCommand& cmd) {
    switch (cmd.type) {
        case RocNetCommandType::SIGNAL_ASPECT: {
            auto& sys = SystemConfig::getInstance().getMutable();
            // Smart Auto-Scale: Auto-expand WS2811 chip count if incoming signal port exceeds current configured chips
            if (cmd.port > sys.ws2811Chips && cmd.port <= 128) {
                uint16_t oldChips = sys.ws2811Chips;
                sys.ws2811Chips = cmd.port;
                signalOutput.setChipCount(cmd.port);
                SystemConfig::getInstance().save();
                LOG_D("⚡ Smart Auto-Scale: WS2811 count expanded from %u to %u chips for Signal Port %u",
                      oldChips, cmd.port, cmd.port);
            }

            LOG_D("RX SIGNAL_ASPECT: Port=%u | AspectNum=%u (Val=%u) | Mask=0x%04X | Blink=0x%04X",
                  cmd.port, cmd.rawAspect, cmd.aspectValue, cmd.aspectMask, cmd.blinkMask);
            signalOutput.setAspect(cmd.port, cmd.aspectMask, cmd.blinkMask);
            break;
        }

        case RocNetCommandType::TURNOUT_SWITCH:
            LOG_D("RX TURNOUT_SWITCH: Port=%u -> %s (Delay: %u ms)",
                  cmd.port, cmd.turnoutState ? "ABZWEIG (THROWN)" : "GERADE (STRAIGHT)", cmd.delayMs);
            servoController.setTurnoutPort(cmd.port, cmd.turnoutState ? 1 : 0, cmd.delayMs);
            break;

        case RocNetCommandType::SERVO_DIRECT:
            LOG_D("RX SERVO_DIRECT: Port=%u, Target Value=%u", cmd.port, cmd.servoValue);
            servoController.setServoDirect(cmd.port, cmd.servoValue);
            break;

        case RocNetCommandType::CHANNEL_CONFIG_WRITE: {
            LOG_D("RX CHANNEL_CONFIG_WRITE: Channel=%u, Left=%u, Right=%u, OffSteps=%u, OnSteps=%u",
                  cmd.port, cmd.offPos, cmd.onPos, cmd.offSteps, cmd.onSteps);
            servoController.programChannel(cmd.port - 1, cmd.offPos, cmd.onPos, cmd.offSteps, cmd.onSteps, cmd.channelOptions);
            sendChannelConfigReply(cmd.senderNodeId, cmd.netId, RN_PROGRAMMING_WCHANNEL, cmd.port);
            break;
        }

        case RocNetCommandType::CHANNEL_CONFIG_READ: {
            uint8_t maxRequested = (cmd.servoValue > cmd.port) ? cmd.servoValue : cmd.port;
            if (maxRequested > servoController.getTotalServos() && maxRequested <= MAX_SERVO_CHANNELS) {
                uint8_t neededModules = ((maxRequested - 1) / SERVOS_PER_MODULE) + 1;
                servoController.setModuleCount(neededModules, true);
            }
            uint8_t total = servoController.getTotalServos();
            uint8_t fromCh = (cmd.port >= 1 && cmd.port <= total) ? cmd.port : 1;
            uint8_t toCh   = (cmd.servoValue >= fromCh && cmd.servoValue <= total) ? cmd.servoValue : fromCh;
            LOG_I("RX CHANNEL_CONFIG_READ: Channels %u to %u (Total %u)", fromCh, toCh, total);
            for (uint8_t ch = fromCh; ch <= toCh; ch++) {
                sendChannelConfigReply(cmd.senderNodeId, cmd.netId, RN_PROGRAMMING_RCHANNEL, ch);
            }
            break;
        }

        case RocNetCommandType::SHOW_IDENTIFY:
            LOG_D("RX SHOW_IDENTIFY: Visual test pattern & reply sent");
            signalOutput.testPattern();
            sendIdentifyReply(cmd.senderNodeId, cmd.netId);
            sendShowEvent();
            break;

        case RocNetCommandType::IDENTIFY_REQUEST:
            LOG_D("RX IDENTIFY_REQUEST: Responding to Node %u (NetID %u)", cmd.senderNodeId, cmd.netId);
            sendIdentifyReply(cmd.senderNodeId, cmd.netId);
            sendShowEvent();
            break;

        case RocNetCommandType::HOST_PING:
            LOG_D("RX HOST_PING: Ping received from Node %u -> TX HOST_PONG sent", cmd.senderNodeId);
            sendHostPong(cmd.senderNodeId, cmd.netId);
            break;

        case RocNetCommandType::PORT_QUERY: {
            uint8_t ch = (cmd.port > 0) ? (cmd.port - 1) : 0;
            bool state = servoController.getTurnoutState(ch);
            LOG_D("RX PORT_QUERY: Port=%u -> TX REPORT (State: %s)", cmd.port, state ? "ABZWEIG" : "GERADE");
            sendPortReport(cmd.senderNodeId, cmd.netId, cmd.port, state);
            break;
        }

        case RocNetCommandType::NODE_ID_WRITE: {
            uint16_t oldId = rocNetParser.getNodeId();
            uint16_t newId = cmd.servoValue;
            if (newId > 0 && newId != oldId) {
                auto& sys = SystemConfig::getInstance().getMutable();
                sys.nodeId = newId;
                SystemConfig::getInstance().save();
                rocNetParser.setNodeId(newId);
                LOG_D("RX NODE_ID_WRITE: Node-ID successfully updated from %u to %u", oldId, newId);
                sendIdentifyReply(0, cmd.netId);
                sendShowEvent();
            }
            break;
        }

        case RocNetCommandType::OPTIONS_READ:
            LOG_D("RX OPTIONS_READ: Sent Hardware Options & I2C Scan (Mask 0x%02X) to Node %u",
                  servoController.getI2cScanMask(), cmd.senderNodeId);
            sendOptionsReply(cmd.senderNodeId, cmd.netId);
            break;

        case RocNetCommandType::SOFTWARE_RESET:
            LOG_E("RX SOFTWARE_RESET: Remote reboot command received via RocNet! Restarting ESP32...");
            delay(200);
            ESP.restart();
            break;

        default:
            break;
    }
}

void sendChannelConfigReply(uint16_t recipientId, uint8_t netId, uint8_t action, uint8_t channel) {
    if (channel < 1 || channel > servoController.getTotalServos()) return;
    ServoChannelConfig cfg = servoController.getChannelConfig(channel - 1);

    uint8_t packet[16];
    packet[0] = netId;
    packet[1] = (uint8_t)(recipientId >> 8);
    packet[2] = (uint8_t)(recipientId & 0xFF);
    packet[3] = (uint8_t)(rocNetParser.getNodeId() >> 8);
    packet[4] = (uint8_t)(rocNetParser.getNodeId() & 0xFF);
    packet[5] = RN_GROUP_PT_STATIONARY;
    packet[6] = (uint8_t)((RN_ACTIONTYPE_REPLY << 5) | action);
    packet[7] = 8; // 8 data bytes

    packet[8]  = channel; // 1-based
    packet[9]  = (uint8_t)(cfg.offPosAngle >> 8);
    packet[10] = (uint8_t)(cfg.offPosAngle & 0xFF);
    packet[11] = (uint8_t)(cfg.onPosAngle >> 8);
    packet[12] = (uint8_t)(cfg.onPosAngle & 0xFF);
    packet[13] = cfg.offSteps;
    packet[14] = cfg.onSteps;
    packet[15] = cfg.options;

    sendRocNetPacket(packet, 16);

    // Also send broadcast copy if recipient was specific
    if (recipientId != 0) {
        packet[1] = 0;
        packet[2] = 0;
        sendRocNetPacket(packet, 16);
    }

    LOG_I("TX CHANNEL_CONFIG_REPLY (Action %u, Channel %u: Left=%u, Right=%u)",
          action, channel, cfg.offPosAngle, cfg.onPosAngle);
}

void sendHostPong(uint16_t recipientId, uint8_t netId) {
    if (!isUdpListening || WiFi.status() != WL_CONNECTED) return;
    uint8_t packet[8];
    packet[0] = netId;
    packet[1] = (uint8_t)(recipientId >> 8);
    packet[2] = (uint8_t)(recipientId & 0xFF);
    packet[3] = (uint8_t)(rocNetParser.getNodeId() >> 8);
    packet[4] = (uint8_t)(rocNetParser.getNodeId() & 0xFF);
    packet[5] = RN_GROUP_HOST;
    packet[6] = (uint8_t)((RN_ACTIONTYPE_REPLY << 5) | RN_HOST_PONG);
    packet[7] = 0;
    sendRocNetPacket(packet, 8);
}

void sendPortReport(uint16_t recipientId, uint8_t netId, uint16_t port, bool thrown) {
    if (!isUdpListening || WiFi.status() != WL_CONNECTED) return;
    uint8_t packet[12];
    packet[0] = netId;
    packet[1] = (uint8_t)(recipientId >> 8);
    packet[2] = (uint8_t)(recipientId & 0xFF);
    packet[3] = (uint8_t)(rocNetParser.getNodeId() >> 8);
    packet[4] = (uint8_t)(rocNetParser.getNodeId() & 0xFF);
    packet[5] = RN_GROUP_STATIONARY;
    // Broadcast events use EVENT (0x27), direct queries use REPLY (0x47)
    uint8_t actionType = (recipientId == 0) ? RN_ACTIONTYPE_EVENT : RN_ACTIONTYPE_REPLY;
    packet[6] = (uint8_t)((actionType << 5) | RN_STATIONARY_REPORT);
    packet[7] = 4; // 4 data bytes: addrH, addrL, status1, status2
    packet[8] = (uint8_t)(port >> 8);
    packet[9] = (uint8_t)(port & 0xFF);
    packet[10] = thrown ? 1 : 0;
    packet[11] = 0;
    sendRocNetPacket(packet, 12);
}

void sendOptionsReply(uint16_t recipientId, uint8_t netId) {
    if (!isUdpListening || WiFi.status() != WL_CONNECTED) return;
    uint8_t packet[8 + 14];
    memset(packet, 0, sizeof(packet));
    packet[0] = netId;
    packet[1] = (uint8_t)(recipientId >> 8);
    packet[2] = (uint8_t)(recipientId & 0xFF);
    packet[3] = (uint8_t)(rocNetParser.getNodeId() >> 8);
    packet[4] = (uint8_t)(rocNetParser.getNodeId() & 0xFF);
    packet[5] = RN_GROUP_PT_STATIONARY;
    packet[6] = (uint8_t)((RN_ACTIONTYPE_REPLY << 5) | RN_PROGRAMMING_RDOPT);
    packet[7] = 14; // 14 data bytes per RocNet specification

    packet[8]  = 1; // iotype: 1 = I2C
    packet[9]  = 0; // flags
    packet[10] = 0; // cstype
    packet[11] = 0; // csdevice
    // I2C scan mask for PCA9685 at 0x40 (Bit 0 = 0x40, Bit 1 = 0x41 etc.)
    packet[17] = servoController.getI2cScanMask();
    sendRocNetPacket(packet, 22);
}
