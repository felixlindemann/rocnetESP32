#pragma once

/*
 * ==============================================================================
 * QUELLENNACHWEIS / SOURCE ATTRIBUTION:
 * ==============================================================================
 * Dieser Parser implementiert die Dekodierung von RocNet-Binärtelegrammen
 * basierend auf der offiziellen Spezifikation von Rocrail:
 *   - Dokumentation: https://www.rocrail-wiki.net/doku.php?id=rocnet:rocnet-prot-en
 *   - Urheber:       Copyright (c) 2002-2026 Robert Jan Versluis, Rocrail.net
 *   - Offline-Kopie: literature/rocnet_rocnet-prot-en [Innovative Model Railroad Control System].html
 *   - Header-Quelle: lib/rocnet-const.h (Copied from Rocrail)
 * 
 * Implementierte RocNet-Gruppen (gemäß Protokollspezifikation):
 *   - Group 0: Host (Ping / Pong / Shutdown)
 *   - Group 1: Command Station (RN_CS_SWITCH Weichenbefehle)
 *   - Group 3: Stationary (RN_STATIONARY_SINGLE_PORT, IDENTIFY, SHOW)
 *   - Group 5: Programming Stationary (RN_PROGRAMMING_SETCHANNEL, WCHANNEL, RCHANNEL)
 *   - Group 8: Aspect Numbers / Bitmasks (WS2811 LED-Signale)
 *   - Group 9: Output (RN_GROUP_OUTPUT Action 14 Aspect / Action 1 Switch)
 * ==============================================================================
 */

#include <cstdint>
#include <cstddef>
#include <cstring>
#ifdef ARDUINO
#include <Arduino.h>
#endif
#include "Config.h"
#include "rocnet-const.h"

// Command types supported by the controller
enum class RocNetCommandType {
    NONE,
    SIGNAL_ASPECT,         // Aspect Bitmask for WS2811 (Group 0x08 or Group 9 Macro)
    TURNOUT_SWITCH,        // Turnout Straight / Thrown (Group 3, Group 9, Group 1, or Group 2)
    SERVO_DIRECT,          // Direct Servo Control (Group 5 Action 18 or Group 3 Direct)
    CHANNEL_CONFIG_WRITE,  // Write Channel Config (Group 5 Action 16)
    CHANNEL_CONFIG_READ,   // Read Channel Config (Group 5 Action 15)
    SHOW_IDENTIFY,         // Visual identification flash (Group 3 Action 11 SHOW)
    IDENTIFY_REQUEST,      // Host queries node identification (Group 3 Action 8 IDENTIFY or 12 STARTOFDAY)
    HOST_PING,             // Host ping request (Group 0 Action 2 PING)
    PORT_QUERY,            // Query turnout/port status (Group 3 Action 4 or 5)
    NODE_ID_WRITE,         // Remote set RocNet Node ID (Group 5 Action 6 WRNID)
    OPTIONS_READ,          // Read node options and I2C scan report (Group 5 Action 7 RDOPT)
    SOFTWARE_RESET         // Remote soft reboot request (Group 5 Action 11 UPDATE)
};

// Parsed command structure
struct RocNetCommand {
    RocNetCommandType type = RocNetCommandType::NONE;
    uint8_t netId = 0;
    uint16_t recipientNodeId = 0;
    uint16_t senderNodeId = 0;
    uint8_t group = 0;
    uint8_t action = 0;
    uint16_t port = 0;           // 1-based port (signals or turnouts 1..16)
    uint8_t rawAspect = 0;       // Raw aspect index from Rocrail (0, 1, 2...)
    uint16_t aspectValue = 0;    // Raw 16-bit aspect value from Rocrail (e.g. 1, 2, 512...)
    uint16_t aspectMask = 0;     // 16-bit aspect mask for signals
    uint16_t blinkMask = 0;      // 16-bit mask of LEDs that should blink
    bool turnoutState = false;   // false = straight (Gerade), true = thrown (Abzweig)
    uint32_t delayMs = 0;        // Configured switch delay / time from Rocrail
    uint16_t servoValue = 0;     // Target angle (0..180) or pulse in us
    uint16_t offPos = 0;         // Left limit (e.g. 39)
    uint16_t onPos = 0;          // Right limit (e.g. 50)
    uint8_t offSteps = 0;
    uint8_t onSteps = 0;
    uint8_t channelOptions = 0;
    uint8_t rawData[16] = {0};   // Raw payload bytes for debugging
    uint8_t rawDataLen = 0;
};

class RocNetParser {
public:
    explicit RocNetParser(uint16_t myNodeId = DEFAULT_ROCNET_NODE_ID);

    // Set or get active Node ID
    void setNodeId(uint16_t nodeId);
    uint16_t getNodeId() const;

    // Parse incoming UDP packet. Returns true if valid command intended for this node.
    bool parse(const uint8_t* buffer, size_t length, RocNetCommand& outCmd);

    // Helper for debugging / string output
    static const char* commandTypeToString(RocNetCommandType type);

private:
    uint16_t _myNodeId;

    bool parseCompactFrame(const uint8_t* buffer, size_t length, uint16_t recipientId, RocNetCommand& outCmd);
    bool parseStandardFrame(const uint8_t* buffer, size_t length, uint16_t recipientId, RocNetCommand& outCmd);
};
