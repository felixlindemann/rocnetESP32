#pragma once

/*
 * ==============================================================================
 * QUELLENNACHWEIS / SOURCE ATTRIBUTION:
 * ==============================================================================
 * Das vorliegende Projekt implementiert einen kompatiblen RocNet-Knoten.
 * Die Netzwerk-, Port- und Gruppenparameter basieren auf der offiziellen
 * RocNet-Protokollspezifikation von Rocrail:
 *   - Dokumentation: https://www.rocrail-wiki.net/doku.php?id=rocnet:rocnet-prot-en
 *   - Urheber:       Copyright (c) 2002-2026 Robert Jan Versluis, Rocrail.net
 *   - Offline-Kopie: literature/rocnet_rocnet-prot-en [Innovative Model Railroad Control System].html
 *   - Konstanten:    lib/rocnet-const.h (Copied from Rocrail)
 * ==============================================================================
 */

#include <cstdint>
#include <cstddef>

#ifdef ARDUINO
#include <Arduino.h>
#include <IPAddress.h>
#define ROCNET_MULTICAST_IP IPAddress(224, 0, 0, 1)
#else
#define ROCNET_MULTICAST_IP "224.0.0.1"
#endif

// ==============================================================================
// 1. WiFi Configuration (ausgelagert in secrets.h)
// ==============================================================================
#if __has_include("secrets.h")
#include "secrets.h"
#endif

#ifndef WIFI_SSID
#define WIFI_SSID "YOUR_WIFI_SSID"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#endif

// ==============================================================================
// 2. RocNet Network Configuration
// Quellennachweis: Definiert gemäß RocNet Spezifikation (Rocrail.net)
// https://www.rocrail-wiki.net/doku.php?id=rocnet:rocnet-prot-en#network
// ==============================================================================
// Official RocNet Standard Multicast Port according to Rocrail documentation (Port 4321)
constexpr uint16_t ROCNET_MULTICAST_PORT = 4321;

// Default Node ID of this ESP32 controller (configurable, default: 204)
constexpr uint16_t DEFAULT_ROCNET_NODE_ID = 204;

// Node Name (shown as UID-Name in RocNetNode dialog)
constexpr const char* DEFAULT_ROCNET_NODE_NAME = "ESP32-Signal";

// Vendor / Manufacturer ID (0 = DIY / Unknown, 1 = GCA, 13 = WiRocS)
constexpr uint8_t DEFAULT_ROCNET_VENDOR_ID = 0;

// Firmware Version (VersionH = Major, VersionL = Minor)
constexpr uint8_t DEFAULT_ROCNET_VERSION_MAJOR = 1;
constexpr uint8_t DEFAULT_ROCNET_VERSION_MINOR = 0;

// RocNet Groups (Quellennachweis: lib/rocnet-const.h & RocNet-Spezifikation)
constexpr uint8_t ROCNET_GROUP_TURNOUT        = 0x02; // RN_GROUP_MOBILE / Turnout straight / thrown
constexpr uint8_t ROCNET_GROUP_SERVO_DIRECT   = 0x03; // RN_GROUP_STATIONARY / Direct servo angle / pulse
constexpr uint8_t ROCNET_GROUP_ASPECT         = 0x08; // RN_GROUP_SENSOR / Aspect Extended Output
constexpr uint8_t ROCNET_GROUP_OUTPUT         = 0x09; // RN_GROUP_OUTPUT / Rocrail standard output


// ==============================================================================
// 3. WS2811 Signal Configuration
// ==============================================================================
// Data pin for WS2811 LED driver string (Wemos Lolin32 Lite)
constexpr uint8_t WS2811_DATA_PIN = 32;

// Default number of physical WS2811 chips (each chip controls 3 channels / LEDs)
// 16 chips = 48 LED channels
constexpr uint16_t DEFAULT_WS2811_CHIP_COUNT = 16;

// Default signal brightness (0..255)
constexpr uint8_t DEFAULT_SIGNAL_BRIGHTNESS = 255;

// Signal Aspect Mappings (LED Bits: Bit 0 = LED1/Red, Bit 1 = LED2/Green, Bit 2 = LED3/Yellow/White)
// Aspect 1: Red (statisch)
constexpr uint16_t ASPECT_1_MASK  = 0x0001; 
constexpr uint16_t ASPECT_1_BLINK = 0x0000;

// Aspect 2: Green (statisch)
constexpr uint16_t ASPECT_2_MASK  = 0x0002; 
constexpr uint16_t ASPECT_2_BLINK = 0x0000;

// Aspect 3: Gelb / Blinkend (wie Grün, aber blinkend)
constexpr uint16_t ASPECT_3_MASK  = 0x0002; 
constexpr uint16_t ASPECT_3_BLINK = 0x0002; 

// Aspect 4: Gelb / Weiß (statisch)
constexpr uint16_t ASPECT_4_MASK  = 0x0004; 
constexpr uint16_t ASPECT_4_BLINK = 0x0000;

// Signal Blink Interval in milliseconds (500ms = 1 Hz)
constexpr uint32_t SIGNAL_BLINK_INTERVAL_MS = 500;

// ==============================================================================
// 4. PCA9685 Servo / Turnout Configuration
// ==============================================================================
// Hardware I2C pins on Wemos Lolin32 Lite (GPIO 19 = SDA, GPIO 18 = SCL)
constexpr int I2C_SDA_PIN = 19;
constexpr int I2C_SCL_PIN = 18;

// PCA9685 I2C Base Address (Module 1 = 0x40, Module 2 = 0x41, Module 3 = 0x42, Module 4 = 0x43)
constexpr uint8_t DEFAULT_PCA9685_BASE_ADDR = 0x40;
constexpr uint8_t DEFAULT_PCA9685_I2C_ADDR = DEFAULT_PCA9685_BASE_ADDR;

// Multi-PCA9685 Module limits (1..4 modules, 16..64 servos)
constexpr uint8_t DEFAULT_PCA9685_MODULE_COUNT = 1;
constexpr uint8_t MAX_PCA9685_MODULES = 4;
constexpr uint8_t SERVOS_PER_MODULE = 16;
constexpr uint8_t MAX_SERVO_CHANNELS = MAX_PCA9685_MODULES * SERVOS_PER_MODULE; // 64 total servos
constexpr uint8_t SERVO_CHANNEL_COUNT = MAX_SERVO_CHANNELS;

// Servo PWM frequency (50 Hz = 20 ms period)
constexpr float PCA9685_PWM_FREQ = 50.0f;

// Helper function to convert degrees (0..180°) to standard RC servo pulse (500..2500 µs)
// Standard RC Servos: 0° = 500 µs, 90° = 1500 µs, 180° = 2500 µs
constexpr uint16_t degreesToUs(float deg) {
    return (uint16_t)(500.0f + (deg * 2000.0f / 180.0f));
}

// Default initial angle limits before user runtime calibration
constexpr uint16_t SERVO_DEFAULT_LEFT_ANGLE_DEG = 30;
constexpr uint16_t SERVO_DEFAULT_RIGHT_ANGLE_DEG = 150;

// Pulse width limits in microseconds for servos (Full 0° .. 180° Range)
// 0° = 500 µs (Left / Straight), 180° = 2500 µs (Right / Thrown)
constexpr uint16_t SERVO_DEFAULT_MIN_PULSE_US = degreesToUs(0.0f);   // 500 us
constexpr uint16_t SERVO_DEFAULT_MAX_PULSE_US = degreesToUs(180.0f); // 2500 us

// Turnout travel time in milliseconds (interpolated movement)
// Set to 400 ms for smooth visible motion, or dynamic from Rocrail
constexpr uint32_t SERVO_DEFAULT_TRAVEL_TIME_MS = 400;

// Turnout Port Addressing Mode:
// true = Paired Ports (Rocrail default: Port 1=Straight, Port 2=Thrown -> Servo 1; Port 3/4 -> Servo 2, etc.)
// false = 1:1 Direct Channel (Port 1 -> Servo 1, Port 2 -> Servo 2, etc.)
constexpr bool TURNOUT_PAIRED_PORTS = true;

// Interval in ms between software timer interpolation steps (20ms matches 50Hz)
constexpr uint32_t SERVO_STEP_INTERVAL_MS = 20;

// Turn off PWM pulse after reaching position (idle) to avoid servo humming & save power
constexpr bool SERVO_DEFAULT_SLEEP_ON_IDLE = true;
constexpr uint32_t SERVO_IDLE_TIMEOUT_MS = 500;
