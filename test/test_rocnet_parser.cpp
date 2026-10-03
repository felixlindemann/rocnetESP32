/*
 * ==============================================================================
 * QUELLENNACHWEIS / SOURCE ATTRIBUTION:
 * ==============================================================================
 * Unit-Tests für den RocNet-Telegrammparser
 *
 * Test-Telegramme und Paketstrukturen basieren auf der Spezifikation von Rocrail:
 *   - Dokumentation: https://www.rocrail-wiki.net/doku.php?id=rocnet:rocnet-prot-en
 *   - Urheber:       Copyright (c) 2002-2026 Robert Jan Versluis, Rocrail.net
 *   - Header:        lib/rocnet-const.h (Copied from Rocrail)
 * ==============================================================================
 */

#include <cassert>
#include <iostream>
#include <vector>
#include "RocNetParser.h"
#include "AppLog.h"

void testCompactSignalAspect() {
    RocNetParser parser(204);

    // Compact Aspect Frame:
    // Byte 0: NetID (1)
    // Byte 1..2: Node-ID (204 -> 0x00, 0xCC)
    // Byte 3: Group (0x08)
    // Byte 4: Action/Unused (0x00)
    // Byte 5: Port (1-based, e.g. Port 3)
    // Byte 6..7: Aspect Mask (e.g. 0x0005 -> Bits 0 and 2 set)
    uint8_t packet[] = {0x01, 0x00, 0xCC, 0x08, 0x00, 0x03, 0x00, 0x05};

    RocNetCommand cmd;
    bool success = parser.parse(packet, sizeof(packet), cmd);

    assert(success == true);
    assert(cmd.type == RocNetCommandType::SIGNAL_ASPECT);
    assert(cmd.recipientNodeId == 204);
    assert(cmd.port == 3);
    assert(cmd.aspectMask == 0x0005);
    std::cout << "[PASS] testCompactSignalAspect\n";
}

void testCompactTurnout() {
    RocNetParser parser(204);

    // Turnout Thrown (State = 1) on Port 5
    uint8_t packetThrown[] = {0x01, 0x00, 0xCC, 0x02, 0x00, 0x05, 0x01, 0x00};
    RocNetCommand cmd;
    bool success = parser.parse(packetThrown, sizeof(packetThrown), cmd);

    assert(success == true);
    assert(cmd.type == RocNetCommandType::TURNOUT_SWITCH);
    assert(cmd.port == 5);
    assert(cmd.turnoutState == true);

    // Turnout Straight (State = 0) on Port 5
    uint8_t packetStraight[] = {0x01, 0x00, 0xCC, 0x02, 0x00, 0x05, 0x00, 0x00};
    success = parser.parse(packetStraight, sizeof(packetStraight), cmd);
    assert(success == true);
    assert(cmd.turnoutState == false);

    std::cout << "[PASS] testCompactTurnout\n";
}

void testCompactServoDirect() {
    RocNetParser parser(204);

    // Servo Direct 90 degrees on Port 2
    uint8_t packet[] = {0x01, 0x00, 0xCC, 0x03, 0x00, 0x02, 0x00, 0x5A}; // 0x5A = 90
    RocNetCommand cmd;
    bool success = parser.parse(packet, sizeof(packet), cmd);

    assert(success == true);
    assert(cmd.type == RocNetCommandType::SERVO_DIRECT);
    assert(cmd.port == 2);
    assert(cmd.servoValue == 90);
    std::cout << "[PASS] testCompactServoDirect\n";
}

void testNodeFilter() {
    RocNetParser parser(204);

    // Packet for Node 100 -> Must be ignored!
    uint8_t packetOtherNode[] = {0x01, 0x00, 0x64, 0x08, 0x00, 0x01, 0x00, 0x01};
    RocNetCommand cmd;
    bool success = parser.parse(packetOtherNode, sizeof(packetOtherNode), cmd);
    assert(success == false);

    // Broadcast Packet (Node 0) -> Must be accepted!
    uint8_t packetBroadcast[] = {0x01, 0x00, 0x00, 0x08, 0x00, 0x01, 0x00, 0x01};
    success = parser.parse(packetBroadcast, sizeof(packetBroadcast), cmd);
    assert(success == true);
    assert(cmd.port == 1);
    assert(cmd.aspectMask == 1);

    std::cout << "[PASS] testNodeFilter\n";
}

void testStandardRocNetFrame() {
    RocNetParser parser(204);

    // Standard RocNet Packet (Header: NetID, Recipient, Sender, Group, Action, Len)
    // Recipient: 204 (0x00, 0xCC)
    // Sender: 1 (0x00, 0x01)
    // Group: 0x09 (Output), Action: 0x01, Len: 4
    // Data: state=1, portType=2 (servo), delay=0, port=7
    uint8_t packet[] = {
        0x01,                   // NetID
        0x00, 0xCC,             // Recipient (204)
        0x00, 0x01,             // Sender (1)
        0x09,                   // Group (Output)
        0x01,                   // Action (Switch)
        0x04,                   // Data Len
        0x01, 0x02, 0x00, 0x07  // Data: state=1, type=servo, delay=0, port=7
    };

    RocNetCommand cmd;
    bool success = parser.parse(packet, sizeof(packet), cmd);
    assert(success == true);
    assert(cmd.type == RocNetCommandType::TURNOUT_SWITCH);
    assert(cmd.port == 7);
    assert(cmd.turnoutState == true);
    std::cout << "[PASS] testStandardRocNetFrame\n";
}

void testStandardOutputMacroAspect() {
    RocNetParser parser(204);

    // Group 9 (RN_GROUP_OUTPUT), Action 1 (RN_OUTPUT_SWITCH), Len 4
    // Data: [6 (macro), 0, aspect=7, macro offset / port=4]
    uint8_t packet[] = {
        0x01,                   // NetID
        0x00, 0xCC,             // Recipient (204)
        0x00, 0x01,             // Sender (1)
        RN_GROUP_OUTPUT,        // Group 9
        RN_OUTPUT_SWITCH,       // Action 1
        0x04,                   // Len 4
        0x06, 0x00, 0x07, 0x04  // Macro type 6, 0, aspect 7, port 4
    };

    RocNetCommand cmd;
    bool success = parser.parse(packet, sizeof(packet), cmd);
    assert(success == true);
    assert(cmd.type == RocNetCommandType::SIGNAL_ASPECT);
    assert(cmd.aspectMask == 7);
    assert(cmd.port == 4);

    std::cout << "[PASS] testStandardOutputMacroAspect\n";
}

void testStationaryPortPair() {
    RocNetParser parser(204);

    // Group 3 (RN_GROUP_STATIONARY), Action 2 (RN_STATIONARY_PORT_PAIR), Len 4
    // Data: [1 (throw), 0 (type), 0 (delay), 5 (port)]
    uint8_t packetThrow[] = {
        0x01, 0x00, 0xCC, 0x00, 0x01,
        RN_GROUP_STATIONARY,
        RN_STATIONARY_PORT_PAIR,
        0x04,
        0x01, 0x00, 0x00, 0x05
    };

    RocNetCommand cmd;
    bool success = parser.parse(packetThrow, sizeof(packetThrow), cmd);
    assert(success == true);
    assert(cmd.type == RocNetCommandType::TURNOUT_SWITCH);
    assert(cmd.turnoutState == true);
    assert(cmd.port == 5);

    // Close / Straight
    packetThrow[8] = 0x00;
    success = parser.parse(packetThrow, sizeof(packetThrow), cmd);
    assert(success == true);
    assert(cmd.turnoutState == false);

    std::cout << "[PASS] testStationaryPortPair\n";
}

void testStationaryShow() {
    RocNetParser parser(204);

    // Group 3 (RN_GROUP_STATIONARY), Action 11 (RN_STATIONARY_SHOW)
    uint8_t packetShow[] = {
        0x01, 0x00, 0xCC, 0x00, 0x01,
        RN_GROUP_STATIONARY,
        RN_STATIONARY_SHOW,
        0x00
    };

    RocNetCommand cmd;
    bool success = parser.parse(packetShow, sizeof(packetShow), cmd);
    assert(success == true);
    assert(cmd.type == RocNetCommandType::SHOW_IDENTIFY);

    std::cout << "[PASS] testStationaryShow\n";
}

void testProgrammingSetChannel() {
    RocNetParser parser(204);

    // Group 5 (RN_GROUP_PT_STATIONARY), Action 18 (RN_PROGRAMMING_SETCHANNEL), Len 3
    // Data: [channel=2, valH=0x01, valL=0x5E (350us / angle)]
    uint8_t packetSetChannel[] = {
        0x01, 0x00, 0xCC, 0x00, 0x01,
        RN_GROUP_PT_STATIONARY,
        RN_PROGRAMMING_SETCHANNEL,
        0x03,
        0x02, 0x01, 0x5E
    };

    RocNetCommand cmd;
    bool success = parser.parse(packetSetChannel, sizeof(packetSetChannel), cmd);
    assert(success == true);
    assert(cmd.type == RocNetCommandType::SERVO_DIRECT);
    assert(cmd.port == 2);
    assert(cmd.servoValue == 350);

    std::cout << "[PASS] testProgrammingSetChannel\n";
}

void testEventReplyFilter() {
    RocNetParser parser(204);

    // Action with Type = 1 (Event) or Type = 2 (Reply) must be ignored for I/O
    // Action code = 1, type = 1 -> byte 6 = (1 << 5) | 1 = 0x21
    uint8_t packetEvent[] = {
        0x01, 0x00, 0xCC, 0x00, 0x01,
        RN_GROUP_OUTPUT,
        0x21, // Event
        0x04,
        0x01, 0x00, 0x07, 0x04
    };

    RocNetCommand cmd;
    bool success = parser.parse(packetEvent, sizeof(packetEvent), cmd);
    assert(success == false); // Must be ignored!

    std::cout << "[PASS] testEventReplyFilter\n";
}

void testRocrailNativeSignalAspect() {
    RocNetParser parser(204);

    // Exact packet captured from Rocrail Linux server for Signal sg11:
    // NetID: 0, Recipient: 204, Sender: 1, Group: 9, Action: 14 (0x0E), Len: 8
    // Data: [0x01, 0x08, 0x03, 0x00, 0x01, 0x00, 0x02, 0x0A] -> Aspect 2 (Green) on Port 1
    uint8_t packetGreen[] = {
        0x00, 0x00, 0xCC, 0x00, 0x01, 0x09, 0x0E, 0x08,
        0x01, 0x08, 0x03, 0x00, 0x01, 0x00, 0x02, 0x0A
    };

    RocNetCommand cmd;
    bool success = parser.parse(packetGreen, sizeof(packetGreen), cmd);
    assert(success == true);
    assert(cmd.type == RocNetCommandType::SIGNAL_ASPECT);
    assert(cmd.port == 1);
    assert(cmd.aspectMask == 0x0002); // Green

    // Aspect 1 (Red)
    uint8_t packetRed[] = {
        0x00, 0x00, 0xCC, 0x00, 0x01, 0x09, 0x0E, 0x08,
        0x01, 0x08, 0x03, 0x01, 0x01, 0x00, 0x01, 0x0A
    };
    success = parser.parse(packetRed, sizeof(packetRed), cmd);
    assert(success == true);
    assert(cmd.type == RocNetCommandType::SIGNAL_ASPECT);
    assert(cmd.port == 1);
    assert(cmd.aspectMask == 0x0001); // Red

    std::cout << "[PASS] testRocrailNativeSignalAspect\n";
}

void testProgrammingChannelConfig() {
    RocNetParser parser(204);

    // Group 5 Action 16: Write channel config (Port 1, Left=39, Right=50, offsteps=0, onsteps=0, options=0)
    uint8_t packetWrite[] = {
        0x00, 0x00, 0xCC, 0x00, 0x01, 0x05, 0x10, 0x08,
        0x01, 0x00, 0x27, 0x00, 0x32, 0x00, 0x00, 0x00 // 0x27 = 39, 0x32 = 50
    };
    RocNetCommand cmd;
    bool success = parser.parse(packetWrite, sizeof(packetWrite), cmd);
    assert(success == true);
    assert(cmd.type == RocNetCommandType::CHANNEL_CONFIG_WRITE);
    assert(cmd.port == 1);
    assert(cmd.offPos == 39);
    assert(cmd.onPos == 50);

    // Group 5 Action 15: Read channel config (from channel 1 to channel 16)
    uint8_t packetRead[] = {
        0x00, 0x00, 0xCC, 0x00, 0x01, 0x05, 0x0F, 0x02,
        0x01, 0x10
    };
    success = parser.parse(packetRead, sizeof(packetRead), cmd);
    assert(success == true);
    assert(cmd.type == RocNetCommandType::CHANNEL_CONFIG_READ);
    assert(cmd.port == 1);
    assert(cmd.servoValue == 16);

    std::cout << "[PASS] testProgrammingChannelConfig\n";
}

void testHostPing() {
    RocNetParser parser(204);

    // Group 0 (RN_GROUP_HOST), Action 2 (RN_HOST_PING), Broadcast (recipient = 0)
    uint8_t packetPing[] = {
        0x01, 0x00, 0x00, 0x00, 0x01,
        RN_GROUP_HOST,
        RN_HOST_PING,
        0x00
    };

    RocNetCommand cmd;
    bool success = parser.parse(packetPing, sizeof(packetPing), cmd);
    assert(success == true);
    assert(cmd.type == RocNetCommandType::HOST_PING);

    std::cout << "[PASS] testHostPing\n";
}

void testStationaryPortQuery() {
    RocNetParser parser(204);

    // Group 3 (RN_GROUP_STATIONARY), Action 4 (RN_STATIONARY_QUERY_SINGLE_PORT), Port 6
    uint8_t packetQuery[] = {
        0x01, 0x00, 0xCC, 0x00, 0x01,
        RN_GROUP_STATIONARY,
        RN_STATIONARY_QUERY_SINGLE_PORT,
        0x04,
        0x00, 0x00, 0x00, 0x06
    };

    RocNetCommand cmd;
    bool success = parser.parse(packetQuery, sizeof(packetQuery), cmd);
    assert(success == true);
    assert(cmd.type == RocNetCommandType::PORT_QUERY);
    assert(cmd.port == 6);

    // Action 5 (RN_STATIONARY_QUERY_PORT_PAIR)
    packetQuery[6] = RN_STATIONARY_QUERY_PORT_PAIR;
    packetQuery[11] = 0x08;
    success = parser.parse(packetQuery, sizeof(packetQuery), cmd);
    assert(success == true);
    assert(cmd.type == RocNetCommandType::PORT_QUERY);
    assert(cmd.port == 8);

    std::cout << "[PASS] testStationaryPortQuery\n";
}

void testNodeIdWrite() {
    RocNetParser parser(204);

    // Group 5 (RN_GROUP_PT_STATIONARY), Action 6 (RN_PROGRAMMING_WRNID)
    // New Node-ID: 208 (0x00, 0xD0)
    uint8_t packetWrnid[] = {
        0x01, 0x00, 0xCC, 0x00, 0x01,
        RN_GROUP_PT_STATIONARY,
        RN_PROGRAMMING_WRNID,
        0x02,
        0x00, 0xD0
    };

    RocNetCommand cmd;
    bool success = parser.parse(packetWrnid, sizeof(packetWrnid), cmd);
    assert(success == true);
    assert(cmd.type == RocNetCommandType::NODE_ID_WRITE);
    assert(cmd.servoValue == 208);

    std::cout << "[PASS] testNodeIdWrite\n";
}

void testOptionsRead() {
    RocNetParser parser(204);

    // Group 5 (RN_GROUP_PT_STATIONARY), Action 7 (RN_PROGRAMMING_RDOPT)
    uint8_t packetRdopt[] = {
        0x01, 0x00, 0xCC, 0x00, 0x01,
        RN_GROUP_PT_STATIONARY,
        RN_PROGRAMMING_RDOPT,
        0x00
    };

    RocNetCommand cmd;
    bool success = parser.parse(packetRdopt, sizeof(packetRdopt), cmd);
    assert(success == true);
    assert(cmd.type == RocNetCommandType::OPTIONS_READ);

    std::cout << "[PASS] testOptionsRead\n";
}

void testSoftwareReset() {
    RocNetParser parser(204);

    // Group 5 (RN_GROUP_PT_STATIONARY), Action 11 (RN_PROGRAMMING_UPDATE)
    uint8_t packetReset[] = {
        0x01, 0x00, 0xCC, 0x00, 0x01,
        RN_GROUP_PT_STATIONARY,
        RN_PROGRAMMING_UPDATE,
        0x00
    };

    RocNetCommand cmd;
    bool success = parser.parse(packetReset, sizeof(packetReset), cmd);
    assert(success == true);
    assert(cmd.type == RocNetCommandType::SOFTWARE_RESET);

    std::cout << "[PASS] testSoftwareReset\n";
}

void testAppLoggingLevelsAndBuffer() {
    AppLogger& logger = AppLogger::getInstance();
    logger.clear();

    // Default Level must be LOG_ERROR (Stufe 1)
    assert(logger.getLevel() == AppLogLevel::LOG_ERROR);
    assert(std::string(logger.getLevelName()) == "ERROR");

    // In LOG_ERROR level: only ERROR is logged
    LOG_E("Test Error Msg %d", 42);
    LOG_D("Test Debug Msg should be ignored");
    LOG_I("Test Info Msg should be ignored");

    assert(logger.getCount() == 1);
    String json = logger.getLogsJson(0);
    assert(json.find("Test Error Msg 42") != std::string::npos);
    assert(json.find("Test Debug Msg") == std::string::npos);

    // Switch to LOG_DEBUG (Stufe 2): ERROR + DEBUG logged
    logger.setLevel(AppLogLevel::LOG_DEBUG);
    assert(logger.getLevel() == AppLogLevel::LOG_DEBUG);
    LOG_D("Test Debug Msg %s", "active");
    LOG_I("Test Info Msg still ignored");
    assert(logger.getCount() == 2);

    // Switch to LOG_INFO (Stufe 3): ALL logged
    logger.setLevel(AppLogLevel::LOG_INFO);
    LOG_I("Test Info Msg %s", "visible");
    assert(logger.getCount() == 3);

    json = logger.getLogsJson(0);
    assert(json.find("visible") != std::string::npos);

    // Reset to default for clean state
    logger.setLevel(AppLogLevel::LOG_ERROR);
    logger.clear();
    assert(logger.getCount() == 0);

    std::cout << "[PASS] testAppLoggingLevelsAndBuffer\n";
}

int main() {
    std::cout << "Running RocNetParser Unit Tests...\n";
    testCompactSignalAspect();
    testCompactTurnout();
    testCompactServoDirect();
    testNodeFilter();
    testStandardRocNetFrame();
    testStandardOutputMacroAspect();
    testStationaryPortPair();
    testStationaryShow();
    testProgrammingSetChannel();
    testProgrammingChannelConfig();
    testEventReplyFilter();
    testRocrailNativeSignalAspect();
    testHostPing();
    testStationaryPortQuery();
    testNodeIdWrite();
    testOptionsRead();
    testSoftwareReset();
    testAppLoggingLevelsAndBuffer();
    std::cout << "\nAll Unit Tests Passed Successfully!\n";
    return 0;
}
