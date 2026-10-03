/*
 * ==============================================================================
 * QUELLENNACHWEIS / SOURCE ATTRIBUTION:
 * ==============================================================================
 * Dekodierung der RocNet-Binärtelegramme gemäß Spezifikation:
 *   - Dokumentation: https://www.rocrail-wiki.net/doku.php?id=rocnet:rocnet-prot-en
 *   - Urheber:       Copyright (c) 2002-2026 Robert Jan Versluis, Rocrail.net
 *   - Offline-Kopie: literature/rocnet_rocnet-prot-en [Innovative Model Railroad Control System].html
 *   - Felddefinitionen und Offsets: lib/rocnet-const.h (Copied from Rocrail)
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

    // Minimum packet length check (Header is 8 bytes)
    if (buffer == nullptr || length < 8) {
        return false;
    }

    // Byte 1..2 is the recipient Node ID in big-endian format
    uint16_t recipientId = ((uint16_t)buffer[RN_PACKET_RCPTH] << 8) | buffer[RN_PACKET_RCPTL];

    // Filter incoming packets: accept only matching Node-ID or broadcast (0 or 0xFFFF)
    if (recipientId != _myNodeId && recipientId != 0 && recipientId != 0xFFFF) {
        // Not addressed to this node, silently ignore
        return false;
    }

    outCmd.recipientNodeId = recipientId;

    // Check whether this is the compact frame format from prompt01 (Group at Byte 3)
    // or standard RocNet frame format (Group at Byte 5)
    uint8_t byte3 = buffer[3];
    if (byte3 == ROCNET_GROUP_ASPECT || byte3 == ROCNET_GROUP_TURNOUT || byte3 == ROCNET_GROUP_SERVO_DIRECT) {
        return parseCompactFrame(buffer, length, recipientId, outCmd);
    }

    return parseStandardFrame(buffer, length, recipientId, outCmd);
}

bool RocNetParser::parseCompactFrame(const uint8_t* buffer, size_t length, uint16_t recipientId, RocNetCommand& outCmd) {
    uint8_t group = buffer[3];
    outCmd.group = group;
    outCmd.port = buffer[5]; // Byte 5: Port / Signaladresse (1-basiert)

    switch (group) {
        case ROCNET_GROUP_ASPECT: { // 0x08: Aspect numbers
            outCmd.type = RocNetCommandType::SIGNAL_ASPECT;
            // Byte 6: Aspect Value High Byte, Byte 7: Aspect Value Low Byte
            outCmd.aspectMask = ((uint16_t)buffer[6] << 8) | buffer[7];
            return true;
        }

        case ROCNET_GROUP_TURNOUT: { // 0x02: Classical turnout
            outCmd.type = RocNetCommandType::TURNOUT_SWITCH;
            // State: 0 = straight (gerade), 1 = thrown (abzweigend)
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

    // Header fields according to rocnet-const.h and documentation
    outCmd.netId = buffer[RN_PACKET_NETID];
    outCmd.senderNodeId = ((uint16_t)buffer[RN_PACKET_SNDRH] << 8) | buffer[RN_PACKET_SNDRL];
    uint8_t group = buffer[RN_PACKET_GROUP];
    uint8_t actionRaw = buffer[RN_PACKET_ACTION];
    uint8_t actionCode = actionRaw & RN_ACTION_CODE_MASK;
    uint8_t actionType = (actionRaw & RN_ACTION_TYPE_MASK) >> 5;
    uint8_t dataLen = buffer[RN_PACKET_LEN];

    // According to RocNet specification:
    // If the action type is EVENT (1) or REPLY (2), no real I/O action should be taken
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
    // 0. Group 0: Host (RN_GROUP_HOST)
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
    // 1. Group 9: Output (RN_GROUP_OUTPUT)
    // -------------------------------------------------------------------------
    if (group == RN_GROUP_OUTPUT) {
        // Action 14 (0x0E): Rocrail native Signal Aspect command
        if (actionCode == 14 || actionCode == 0x0E) {
            if (dataLen >= 7) {
                outCmd.type = RocNetCommandType::SIGNAL_ASPECT;
                // data[4] is the address/port (1-based)
                outCmd.port = (data[4] > 0) ? data[4] : 1;
                outCmd.rawAspect = data[3]; // data[3] is the aspect index (0, 1, 2...)

                uint8_t blinkByte = data[5];  // High byte: Blink bitmask (e.g. 0x02 for Value 512)
                uint8_t staticByte = data[6]; // Low byte: Static bitmask (e.g. 0x01 for Value 1, 0x02 for Value 2)
                uint16_t combinedValue = ((uint16_t)blinkByte << 8) | staticByte;
                outCmd.aspectValue = combinedValue;

                outCmd.rawDataLen = (dataLen < sizeof(outCmd.rawData)) ? dataLen : sizeof(outCmd.rawData);
                memcpy(outCmd.rawData, data, outCmd.rawDataLen);

                // Rocrail Aspect Values:
                // Low Byte (data[6]) = static LED mask (Bit 0: Red, Bit 1: Green, Bit 2: Yellow)
                // High Byte (data[5]) = blinking LED mask (e.g. 0x02 = Value 512 = Green Blinking!)
                if (combinedValue > 0) {
                    outCmd.aspectMask = staticByte | blinkByte;
                    outCmd.blinkMask  = blinkByte;
                } else {
                    // Fallback to aspect index mapping (data[3]: 0 -> Aspect 1, 1 -> Aspect 2, 2 -> Aspect 3)
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

        // Action 1: On, Action 0: Off
        if (actionCode == RN_OUTPUT_SWITCH || actionCode == 0) {
            // Check for Type 6 (Macro / Signals / Aspects)
            if (dataLen >= 3 && data[0] == 6) {
                outCmd.type = RocNetCommandType::SIGNAL_ASPECT;
                // data[1] = 0, data[2] = aspect, data[3] = macro offset / port
                outCmd.aspectMask = data[2];
                outCmd.port = (dataLen >= 4) ? data[3] : 1;
                return true;
            }

            // Check if packet uses 4-byte layout [state, type, delay, port]
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
                outCmd.delayMs = delayVal; // Switch time byte from Rocrail
                return true;
            }

            // Official RocNet 3-byte layout: [type, value, address/port]
            if (dataLen >= 3) {
                uint8_t outType = data[0];
                uint8_t val = data[1];
                uint8_t port = data[2];

                // Type 0 = Switch, Type 1 = Light, Type 2 = Servo
                outCmd.type = RocNetCommandType::TURNOUT_SWITCH;
                outCmd.port = port;
                outCmd.turnoutState = (actionCode == 0) ? false : (val != 0);
                return true;
            }
        }
    }

    // -------------------------------------------------------------------------
    // 2. Group 3: Stationary decoders (RN_GROUP_STATIONARY)
    // -------------------------------------------------------------------------
    if (group == RN_GROUP_STATIONARY) {
        // Action 1: Single Port, Action 2: Port Pair
        if (actionCode == RN_STATIONARY_SINGLE_PORT || actionCode == RN_STATIONARY_PORT_PAIR) {
            if (dataLen >= 4) {
                outCmd.type = RocNetCommandType::TURNOUT_SWITCH;
                outCmd.turnoutState = (data[0] != 0); // 0 = close/straight, 1 = throw/turnout
                outCmd.port = data[3];                // data 4 is port
                return true;
            }
        }

        // Action 8: Identify Query, Action 12: Start of Day
        if (actionCode == RN_STATIONARY_IDENTIFY || actionCode == RN_STATIONARY_STARTOFDAY) {
            outCmd.type = RocNetCommandType::IDENTIFY_REQUEST;
            return true;
        }

        // Action 11: Show / Identify LED flash
        if (actionCode == RN_STATIONARY_SHOW) {
            outCmd.type = RocNetCommandType::SHOW_IDENTIFY;
            return true;
        }

        // Action 4: Query Single Port, Action 5: Query Port Pair
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
    // 3. Group 5: Programming Stationary (RN_GROUP_PT_STATIONARY)
    // -------------------------------------------------------------------------
    if (group == RN_GROUP_PT_STATIONARY) {
        // Action 6: Set RocNet ID (idH, idL, subipH, subipL)
        if (actionCode == RN_PROGRAMMING_WRNID && dataLen >= 2) {
            outCmd.type = RocNetCommandType::NODE_ID_WRITE;
            outCmd.servoValue = ((uint16_t)data[0] << 8) | data[1]; // target new node ID
            return true;
        }

        // Action 7: Read hardware options & I2C scan report
        if (actionCode == RN_PROGRAMMING_RDOPT) {
            outCmd.type = RocNetCommandType::OPTIONS_READ;
            return true;
        }

        // Action 11: Remote soft reboot
        if (actionCode == RN_PROGRAMMING_UPDATE) {
            outCmd.type = RocNetCommandType::SOFTWARE_RESET;
            return true;
        }

        // Action 18: Set channel (Direct servo position: channel, valueH, valueL)
        if (actionCode == RN_PROGRAMMING_SETCHANNEL && dataLen >= 3) {
            outCmd.type = RocNetCommandType::SERVO_DIRECT;
            outCmd.port = data[0]; // channel# (1..16)
            outCmd.servoValue = ((uint16_t)data[1] << 8) | data[2];
            return true;
        }

        // Action 16: Write channel config (channel#, offposH, offposL, onposH, onposL, offsteps, onsteps, options)
        if (actionCode == RN_PROGRAMMING_WCHANNEL && dataLen >= 8) {
            outCmd.type = RocNetCommandType::CHANNEL_CONFIG_WRITE;
            outCmd.port = data[0]; // channel# (1..16)
            outCmd.offPos = ((uint16_t)data[1] << 8) | data[2];
            outCmd.onPos  = ((uint16_t)data[3] << 8) | data[4];
            outCmd.offSteps = data[5];
            outCmd.onSteps  = data[6];
            outCmd.channelOptions = data[7];
            return true;
        }

        // Action 15: Read channel config (from channel, to channel)
        if (actionCode == RN_PROGRAMMING_RCHANNEL && dataLen >= 1) {
            outCmd.type = RocNetCommandType::CHANNEL_CONFIG_READ;
            outCmd.port = data[0]; // from channel
            outCmd.servoValue = (dataLen >= 2) ? data[1] : data[0]; // to channel
            return true;
        }
    }

    // -------------------------------------------------------------------------
    // 4. Group 1: Command Station (RN_GROUP_CS)
    // -------------------------------------------------------------------------
    if (group == RN_GROUP_CS) {
        // Action 9: Switch (addrH, addrL, port, gate, action)
        if (actionCode == RN_CS_SWITCH && dataLen >= 5) {
            outCmd.type = RocNetCommandType::TURNOUT_SWITCH;
            outCmd.port = data[2];
            outCmd.turnoutState = (data[4] != 0);
            return true;
        }
    }

    // -------------------------------------------------------------------------
    // 5. Fallback for Prompt01 groups in standard frame wrapper
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
