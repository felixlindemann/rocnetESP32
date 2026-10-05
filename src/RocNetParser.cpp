/*
 * ==============================================================================
 * QUELLENNACHWEIS & LIZENZ / SOURCE ATTRIBUTION & LICENSE:
 * ==============================================================================
 * DE: Dekodierung der RocNet-Binärtelegramme gemäß Spezifikation:
 * EN: Decoding of RocNet binary packets according to official specification:
 *   - Dokumentation / Documentation: https://www.rocrail-wiki.net/doku.php?id=rocnet:rocnet-prot-en
 *   - Urheber / Author:             Copyright (c) 2002-2026 Robert Jan Versluis, Rocrail.net
 *   - Spezifikation / Specification: literature/ROCNET_SPECIFICATION.md
 *   - Felddefinitionen / Field Defs: lib/rocnet-const.h (Copied from Rocrail)
 * ==============================================================================
 */

#include "RocNetParser.h"

RocNetParser::RocNetParser(uint16_t myNodeId)
    : _myNodeId(myNodeId) {
}

void RocNetParser::setNodeId(uint16_t nodeId) {
    _myNodeId = nodeId;
}

uint16_t RocNetParser::getNodeId() const {
    return _myNodeId;
}

const char* RocNetParser::commandTypeToString(RocNetCommandType type) {
    switch (type) {
        case RocNetCommandType::SIGNAL_ASPECT:        return "SIGNAL_ASPECT";
        case RocNetCommandType::TURNOUT_SWITCH:       return "TURNOUT_SWITCH";
        case RocNetCommandType::SERVO_DIRECT:         return "SERVO_DIRECT";
        case RocNetCommandType::CHANNEL_CONFIG_WRITE: return "CHANNEL_CONFIG_WRITE";
        case RocNetCommandType::CHANNEL_CONFIG_READ:  return "CHANNEL_CONFIG_READ";
        case RocNetCommandType::SHOW_IDENTIFY:        return "SHOW_IDENTIFY";
        case RocNetCommandType::IDENTIFY_REQUEST:     return "IDENTIFY_REQUEST";
        case RocNetCommandType::HOST_PING:            return "HOST_PING";
        case RocNetCommandType::PORT_QUERY:           return "PORT_QUERY";
        case RocNetCommandType::NODE_ID_WRITE:        return "NODE_ID_WRITE";
        case RocNetCommandType::OPTIONS_READ:         return "OPTIONS_READ";
        case RocNetCommandType::SOFTWARE_RESET:       return "SOFTWARE_RESET";
        default:                                      return "NONE";
    }
}

bool RocNetParser::parse(const uint8_t* buffer, size_t length, RocNetCommand& outCmd) {
    outCmd = RocNetCommand();

    // DE: Mindestpaketlänge prüfen (Header ist 8 Bytes) / EN: Minimum packet length check (Header is 8 bytes)
    if (buffer == nullptr || length < 8) {
        return false;
    }

    // DE: Byte 1..2 ist die Empfänger-Node-ID in Big-Endian / EN: Byte 1..2 is recipient Node ID in big-endian
    uint16_t recipientId = ((uint16_t)buffer[RN_PACKET_RCPTH] << 8) | buffer[RN_PACKET_RCPTL];

    // DE: Eingehende Pakete filtern: eigene Node-ID oder Broadcast (0 oder 0xFFFF)
    // EN: Filter incoming packets: accept matching Node ID or broadcast (0 or 0xFFFF)
    if (recipientId != _myNodeId && recipientId != 0 && recipientId != 0xFFFF) {
        return false;
    }

    outCmd.recipientNodeId = recipientId;

    // DE: Prüfen auf kompaktes Frame-Format (Gruppe an Byte 3) oder Standard-RocNet (Gruppe an Byte 5)
    // EN: Check whether this is compact frame format (Group at Byte 3) or standard RocNet (Group at Byte 5)
    uint8_t byte3 = buffer[3];
    if (byte3 == ROCNET_GROUP_ASPECT || byte3 == ROCNET_GROUP_TURNOUT || byte3 == ROCNET_GROUP_SERVO_DIRECT) {
        return parseCompactFrame(buffer, length, recipientId, outCmd);
    }

    return parseStandardFrame(buffer, length, recipientId, outCmd);
}

bool RocNetParser::parseCompactFrame(const uint8_t* buffer, size_t length, uint16_t recipientId, RocNetCommand& outCmd) {
    uint8_t group = buffer[3];
    outCmd.group = group;
    outCmd.port = buffer[5]; // DE: Byte 5: Port / Signaladresse (1-basiert) / EN: Byte 5: Port (1-based)

    switch (group) {
        case ROCNET_GROUP_ASPECT: { // 0x08: Aspect numbers
            outCmd.type = RocNetCommandType::SIGNAL_ASPECT;
            // DE: Byte 6: Aspect High Byte, Byte 7: Aspect Low Byte / EN: Aspect value bytes
            outCmd.aspectMask = ((uint16_t)buffer[6] << 8) | buffer[7];
            return true;
        }

        case ROCNET_GROUP_TURNOUT: { // 0x02: Classical turnout
            outCmd.type = RocNetCommandType::TURNOUT_SWITCH;
            // DE: Zustand: 0 = Gerade, 1 = Abzweig / EN: State: 0 = straight, 1 = thrown
            outCmd.turnoutState = (buffer[6] != 0 || buffer[7] != 0);
            return true;
        }

        case ROCNET_GROUP_SERVO_DIRECT: { // 0x03: Direct servo angle / pulse
            outCmd.type = RocNetCommandType::SERVO_DIRECT;
            outCmd.servoValue = ((uint16_t)buffer[6] << 8) | buffer[7];
            return true;
        }

        default:
            return false;
    }
}

bool RocNetParser::parseStandardFrame(const uint8_t* buffer, size_t length, uint16_t recipientId, RocNetCommand& outCmd) {
    if (length < 8) return false;

    // DE: Header-Felder gemäß rocnet-const.h / EN: Header fields according to rocnet-const.h
    outCmd.netId = buffer[RN_PACKET_NETID];
    outCmd.senderNodeId = ((uint16_t)buffer[RN_PACKET_SNDRH] << 8) | buffer[RN_PACKET_SNDRL];
    uint8_t group = buffer[RN_PACKET_GROUP];
    uint8_t actionRaw = buffer[RN_PACKET_ACTION];
    uint8_t actionCode = actionRaw & RN_ACTION_CODE_MASK;
    uint8_t actionType = (actionRaw & RN_ACTION_TYPE_MASK) >> 5;
    uint8_t dataLen = buffer[RN_PACKET_LEN];

    // DE: Gemäß RocNet-Spezifikation: Bei EVENT (1) oder REPLY (2) keine I/O-Aktion ausführen
    // EN: According to RocNet specification: If action type is EVENT (1) or REPLY (2), take no I/O action
    if (actionType != RN_ACTIONTYPE_REQUEST) {
        return false;
    }

    outCmd.group = group;
    outCmd.action = actionCode;

    const uint8_t* data = &buffer[RN_PACKET_DATA];
    size_t availableData = (length >= 8) ? (length - 8) : 0;
    if (availableData < dataLen) {
        dataLen = availableData;
    }

    // -------------------------------------------------------------------------
    // 0. Group 0: Host / Zentralbefehle (RN_GROUP_HOST)
    // -------------------------------------------------------------------------
    if (group == RN_GROUP_HOST) {
        if (actionCode == RN_HOST_PING) {
            outCmd.type = RocNetCommandType::HOST_PING;
            return true;
        }
        if (actionCode == RN_HOST_SHUTDOWN) {
            outCmd.type = RocNetCommandType::SOFTWARE_RESET;
            return true;
        }
    }

    // -------------------------------------------------------------------------
    // 1. Group 9: Output / Ausgänge & Signal-Aspekte (RN_GROUP_OUTPUT)
    // -------------------------------------------------------------------------
    if (group == RN_GROUP_OUTPUT) {
        // DE: Aktion 14 (0x0E): Rocrail natives Signal Aspect Telegramm
        // EN: Action 14 (0x0E): Rocrail native signal aspect command
        if (actionCode == 14 || actionCode == 0x0E) {
            if (dataLen >= 7) {
                outCmd.type = RocNetCommandType::SIGNAL_ASPECT;
                outCmd.port = (data[4] > 0) ? data[4] : 1;
                outCmd.rawAspect = data[3];

                uint8_t blinkByte = data[5];  // DE: Blink-Maske / EN: Blink bitmask
                uint8_t staticByte = data[6]; // DE: Statische Maske / EN: Static bitmask
                uint16_t combinedValue = ((uint16_t)blinkByte << 8) | staticByte;
                outCmd.aspectValue = combinedValue;

                outCmd.rawDataLen = (dataLen < sizeof(outCmd.rawData)) ? dataLen : sizeof(outCmd.rawData);
                memcpy(outCmd.rawData, data, outCmd.rawDataLen);

                if (combinedValue > 0) {
                    outCmd.aspectMask = staticByte | blinkByte;
                    outCmd.blinkMask  = blinkByte;
                } else {
                    uint8_t aspectIdx = data[3] + 1;
                    if (aspectIdx == 1) {
                        outCmd.aspectMask = ASPECT_1_MASK;
                        outCmd.blinkMask  = ASPECT_1_BLINK;
                    } else if (aspectIdx == 2) {
                        outCmd.aspectMask = ASPECT_2_MASK;
                        outCmd.blinkMask  = ASPECT_2_BLINK;
                    } else if (aspectIdx == 3) {
                        outCmd.aspectMask = ASPECT_3_MASK;
                        outCmd.blinkMask  = ASPECT_3_BLINK;
                    } else {
                        outCmd.aspectMask = ASPECT_4_MASK;
                        outCmd.blinkMask  = ASPECT_4_BLINK;
                    }
                }
                return true;
            }
        }

        // DE: Aktion 1: Ein, Aktion 0: Aus / EN: Action 1: On, Action 0: Off
        if (actionCode == RN_OUTPUT_SWITCH || actionCode == 0) {
            // DE: Typ 6 (Makro / Signale / Aspekte) / EN: Type 6 (Macro / Signals / Aspects)
            if (dataLen >= 3 && data[0] == 6) {
                outCmd.type = RocNetCommandType::SIGNAL_ASPECT;
                outCmd.aspectMask = data[2];
                outCmd.port = (dataLen >= 4) ? data[3] : 1;
                return true;
            }

            // DE: 4-Byte-Layout [state, type, delay, port] / EN: 4-byte layout [state, type, delay, port]
            if (dataLen >= 4 && (data[0] <= 1) && (data[1] <= 7)) {
                uint8_t state = data[0];
                uint8_t outType = data[1];
                uint8_t delayVal = data[2];
                uint8_t port = data[3];

                if (outType == 6) {
                    outCmd.type = RocNetCommandType::SIGNAL_ASPECT;
                    outCmd.aspectMask = state;
                    outCmd.port = port;
                    return true;
                }

                outCmd.type = RocNetCommandType::TURNOUT_SWITCH;
                outCmd.port = port;
                outCmd.turnoutState = (actionCode == 0) ? false : (state != 0);
                outCmd.delayMs = delayVal;
                return true;
            }

            // DE: Standard 3-Byte-Layout [type, value, address/port] / EN: Standard 3-byte layout
            if (dataLen >= 3) {
                uint8_t outType = data[0];
                uint8_t val = data[1];
                uint8_t port = data[2];

                outCmd.type = RocNetCommandType::TURNOUT_SWITCH;
                outCmd.port = port;
                outCmd.turnoutState = (actionCode == 0) ? false : (val != 0);
                return true;
            }
        }
    }

    // -------------------------------------------------------------------------
    // 2. Group 3: Stationary / Stationäre Decoder (RN_GROUP_STATIONARY)
    // -------------------------------------------------------------------------
    if (group == RN_GROUP_STATIONARY) {
        // DE: Aktion 1: Einzelport, Aktion 2: Port-Paar / EN: Action 1: Single port, Action 2: Port pair
        if (actionCode == RN_STATIONARY_SINGLE_PORT || actionCode == RN_STATIONARY_PORT_PAIR) {
            if (dataLen >= 4) {
                outCmd.type = RocNetCommandType::TURNOUT_SWITCH;
                outCmd.turnoutState = (data[0] != 0); // 0 = Gerade / straight, 1 = Abzweig / thrown
                outCmd.port = data[3];
                return true;
            }
        }

        // DE: Aktion 8: Identify, Aktion 12: Start of Day / EN: Action 8: Identify, Action 12: Start of Day
        if (actionCode == RN_STATIONARY_IDENTIFY || actionCode == RN_STATIONARY_STARTOFDAY) {
            outCmd.type = RocNetCommandType::IDENTIFY_REQUEST;
            return true;
        }

        // DE: Aktion 11: Show (LED-Blinken) / EN: Action 11: Show / Identify LED flash
        if (actionCode == RN_STATIONARY_SHOW) {
            outCmd.type = RocNetCommandType::SHOW_IDENTIFY;
            return true;
        }

        // DE: Aktion 4 & 5: Port-Zustandsabfrage / EN: Action 4 & 5: Query Port State
        if (actionCode == RN_STATIONARY_QUERY_SINGLE_PORT || actionCode == RN_STATIONARY_QUERY_PORT_PAIR) {
            outCmd.type = RocNetCommandType::PORT_QUERY;
            uint8_t p = 1;
            if (dataLen >= 4 && data[0] == 0 && data[3] != 0) {
                p = data[3];
            } else if (dataLen >= 1 && data[0] != 0) {
                p = data[0];
            }
            outCmd.port = p;
            return true;
        }
    }

    // -------------------------------------------------------------------------
    // 3. Group 5: Programming Stationary / Programmierung (RN_GROUP_PT_STATIONARY)
    // -------------------------------------------------------------------------
    if (group == RN_GROUP_PT_STATIONARY) {
        // DE: Aktion 6: Node-ID schreiben / EN: Action 6: Set RocNet Node ID
        if (actionCode == RN_PROGRAMMING_WRNID && dataLen >= 2) {
            outCmd.type = RocNetCommandType::NODE_ID_WRITE;
            outCmd.servoValue = ((uint16_t)data[0] << 8) | data[1];
            return true;
        }

        // DE: Aktion 7: Hardware-Optionen & I2C-Scan lesen / EN: Action 7: Read hardware options & I2C scan
        if (actionCode == RN_PROGRAMMING_RDOPT) {
            outCmd.type = RocNetCommandType::OPTIONS_READ;
            return true;
        }

        // DE: Aktion 11: Remote-Neustart / EN: Action 11: Remote soft reboot
        if (actionCode == RN_PROGRAMMING_UPDATE) {
            outCmd.type = RocNetCommandType::SOFTWARE_RESET;
            return true;
        }

        // DE: Aktion 18: Kanal direkt stellen / EN: Action 18: Set channel direct servo position
        if (actionCode == RN_PROGRAMMING_SETCHANNEL && dataLen >= 3) {
            outCmd.type = RocNetCommandType::SERVO_DIRECT;
            outCmd.port = data[0];
            outCmd.servoValue = ((uint16_t)data[1] << 8) | data[2];
            return true;
        }

        // DE: Aktion 16: Kanalkonfiguration schreiben / EN: Action 16: Write channel config
        if (actionCode == RN_PROGRAMMING_WCHANNEL && dataLen >= 8) {
            outCmd.type = RocNetCommandType::CHANNEL_CONFIG_WRITE;
            outCmd.port = data[0];
            outCmd.offPos = ((uint16_t)data[1] << 8) | data[2];
            outCmd.onPos  = ((uint16_t)data[3] << 8) | data[4];
            outCmd.offSteps = data[5];
            outCmd.onSteps  = data[6];
            outCmd.channelOptions = data[7];
            return true;
        }

        // DE: Aktion 15: Kanalkonfiguration lesen / EN: Action 15: Read channel config
        if (actionCode == RN_PROGRAMMING_RCHANNEL && dataLen >= 1) {
            outCmd.type = RocNetCommandType::CHANNEL_CONFIG_READ;
            outCmd.port = data[0];
            outCmd.servoValue = (dataLen >= 2) ? data[1] : data[0];
            return true;
        }
    }

    // -------------------------------------------------------------------------
    // 4. Group 1: Command Station / Zentrale (RN_GROUP_CS)
    // -------------------------------------------------------------------------
    if (group == RN_GROUP_CS) {
        if (actionCode == RN_CS_SWITCH && dataLen >= 5) {
            outCmd.type = RocNetCommandType::TURNOUT_SWITCH;
            outCmd.port = data[2];
            outCmd.turnoutState = (data[4] != 0);
            return true;
        }
    }

    // -------------------------------------------------------------------------
    // 5. Fallback für Legacy-Gruppen / Fallback for legacy group formats
    // -------------------------------------------------------------------------
    if (group == ROCNET_GROUP_ASPECT) {
        outCmd.type = RocNetCommandType::SIGNAL_ASPECT;
        if (dataLen >= 4) {
            outCmd.aspectMask = ((uint16_t)data[0] << 8) | data[1];
            outCmd.port = data[3];
        } else if (dataLen >= 2) {
            outCmd.port = data[0];
            outCmd.aspectMask = data[1];
        } else {
            outCmd.port = 1;
            outCmd.aspectMask = (dataLen >= 1) ? data[0] : 0;
        }
        return true;
    }

    if (group == ROCNET_GROUP_TURNOUT) {
        outCmd.type = RocNetCommandType::TURNOUT_SWITCH;
        outCmd.port = (dataLen >= 4) ? data[3] : ((dataLen >= 1) ? data[0] : 1);
        outCmd.turnoutState = (dataLen >= 1 && data[0] != 0);
        return true;
    }

    return false;
}
