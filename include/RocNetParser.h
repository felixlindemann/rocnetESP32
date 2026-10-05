#pragma once

/*
 * ==============================================================================
 * QUELLENNACHWEIS & LIZENZ / SOURCE ATTRIBUTION & LICENSE:
 * ==============================================================================
 * DE: Dieser Parser implementiert die Dekodierung von RocNet-Binärtelegrammen
 *     basierend auf der offiziellen Spezifikation von Rocrail:
 * EN: This parser implements decoding of RocNet binary packets based on the
 *     official Rocrail specification:
 *   - Dokumentation / Documentation: https://www.rocrail-wiki.net/doku.php?id=rocnet:rocnet-prot-en
 *   - Urheber / Author:             Copyright (c) 2002-2026 Robert Jan Versluis, Rocrail.net
 *   - Spezifikation / Specification: literature/ROCNET_SPECIFICATION.md
 *   - Header-Quelle / Header Source: lib/rocnet-const.h (Copied from Rocrail)
 * 
 * DE: Implementierte RocNet-Gruppen (gemäß Protokollspezifikation):
 * EN: Implemented RocNet groups (according to protocol specification):
 *   - Group 0: Host (Ping / Pong / Shutdown)
 *   - Group 1: Command Station (RN_CS_SWITCH Weichenbefehle / turnout commands)
 *   - Group 3: Stationary (RN_STATIONARY_SINGLE_PORT, IDENTIFY, SHOW, REPORT)
 *   - Group 5: Programming Stationary (SETCHANNEL, WCHANNEL, RCHANNEL, WRNID, RDOPT)
 *   - Group 8: Aspect Numbers / Bitmasks (WS2811 LED-Signale / signal aspects)
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

// DE: Vom Controller unterstützte Befehlstypen / EN: Command types supported by the controller
enum class RocNetCommandType {
    NONE,
    SIGNAL_ASPECT,         // DE: Signal-Bitmaske für WS2811 / EN: Aspect bitmask for WS2811
    TURNOUT_SWITCH,        // DE: Weichenumlauf Gerade/Abzweig / EN: Turnout straight/thrown
    SERVO_DIRECT,          // DE: Direkte Servo-Winkelsteuerung / EN: Direct servo angle/pulse control
    CHANNEL_CONFIG_WRITE,  // DE: Servo-Endlagen schreiben / EN: Write channel config
    CHANNEL_CONFIG_READ,   // DE: Servo-Endlagen lesen / EN: Read channel config
    SHOW_IDENTIFY,         // DE: Optische Node-Identifikation (LED) / EN: Visual identification flash
    IDENTIFY_REQUEST,      // DE: Identifikationsabfrage durch Zentrale / EN: Host queries node identity
    HOST_PING,             // DE: Verbindungsprüfung / EN: Host ping request
    PORT_QUERY,            // DE: Ist-Zustand Port/Weiche abfragen / EN: Query turnout/port status
    NODE_ID_WRITE,         // DE: Node-ID ferngesteuert ändern / EN: Remote set RocNet Node ID
    OPTIONS_READ,          // DE: Optionen und I2C-Scan lesen / EN: Read node options and I2C scan report
    SOFTWARE_RESET         // DE: Geordneter Software-Neustart / EN: Remote soft reboot request
};

// DE: Struktur für dekodierte Befehle / EN: Parsed command structure
struct RocNetCommand {
    RocNetCommandType type = RocNetCommandType::NONE;
    uint8_t netId = 0;
    uint16_t recipientNodeId = 0;
    uint16_t senderNodeId = 0;
    uint8_t group = 0;
    uint8_t action = 0;
    uint16_t port = 0;           // DE: 1-basierter Port (1..16) / EN: 1-based port (1..16)
    uint8_t rawAspect = 0;       // DE: Roh-Aspect-Index / EN: Raw aspect index from Rocrail
    uint16_t aspectValue = 0;    // DE: Roh-16-Bit-Aspectwert / EN: Raw 16-bit aspect value
    uint16_t aspectMask = 0;     // DE: 16-Bit-Signal-Bitmaske / EN: 16-bit aspect mask for signals
    uint16_t blinkMask = 0;      // DE: Maske blinkender LEDs / EN: 16-bit mask of blinking LEDs
    bool turnoutState = false;   // DE: false = Gerade, true = Abzweig / EN: false = straight, true = thrown
    uint32_t delayMs = 0;        // DE: Konfigurierte Stellzeit in ms / EN: Configured travel delay in ms
    uint16_t servoValue = 0;     // DE: Zielwinkel (0..180°) oder Impuls in µs / EN: Target angle or pulse
    uint16_t offPos = 0;         // DE: Linker Grenzwinkel / EN: Left limit angle
    uint16_t onPos = 0;          // DE: Rechter Grenzwinkel / EN: Right limit angle
    uint8_t offSteps = 0;
    uint8_t onSteps = 0;
    uint8_t channelOptions = 0;
    uint8_t rawData[16] = {0};   // DE: Nutzdaten-Puffer für Debugging / EN: Raw payload bytes for debugging
    uint8_t rawDataLen = 0;
};

class RocNetParser {
public:
    explicit RocNetParser(uint16_t myNodeId = DEFAULT_ROCNET_NODE_ID);

    // DE: Aktive Node-ID setzen/abrufen / EN: Set or get active Node ID
    void setNodeId(uint16_t nodeId);
    uint16_t getNodeId() const;

    // DE: Eintreffendes UDP-Paket parsen / EN: Parse incoming UDP packet
    bool parse(const uint8_t* buffer, size_t length, RocNetCommand& outCmd);

    // DE: Typ als lesbaren Text ausgeben / EN: Helper for debugging string output
    static const char* commandTypeToString(RocNetCommandType type);

private:
    uint16_t _myNodeId;

    bool parseCompactFrame(const uint8_t* buffer, size_t length, uint16_t recipientId, RocNetCommand& outCmd);
    bool parseStandardFrame(const uint8_t* buffer, size_t length, uint16_t recipientId, RocNetCommand& outCmd);
};
