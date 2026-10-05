#pragma once

/*
 * ==============================================================================
 * QUELLENNACHWEIS & LIZENZ / SOURCE ATTRIBUTION & LICENSE:
 * ==============================================================================
 * DE: Das vorliegende Projekt implementiert einen kompatiblen RocNet-Knoten.
 *     Die Netzwerk-, Port- und Gruppenparameter basieren auf der offiziellen
 *     RocNet-Protokollspezifikation von Rocrail:
 * EN: This project implements a compatible RocNet node. Network, port, and group
 *     parameters are based on the official RocNet specification by Rocrail:
 *   - Dokumentation / Documentation: https://www.rocrail-wiki.net/doku.php?id=rocnet:rocnet-prot-en
 *   - Urheber / Author:             Copyright (c) 2002-2026 Robert Jan Versluis, Rocrail.net
 *   - Spezifikation / Specification: literature/ROCNET_SPECIFICATION.md
 *   - C-Konstanten / C Constants:    lib/rocnet-const.h (Copied from Rocrail)
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
// 1. WiFi Configuration / WLAN-Konfiguration
// DE: Ausgelagert in secrets.h (nicht in Git) / EN: Stored in secrets.h (not in Git)
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
// 2. RocNet Network Configuration / RocNet-Netzwerkkonfiguration
// DE: Definiert gemäß RocNet-Spezifikation (Rocrail.net)
// EN: Defined according to RocNet specification (Rocrail.net)
// ==============================================================================
// DE: Offizieller RocNet Multicast-Port (4321) / EN: Official RocNet multicast port (4321)
constexpr uint16_t ROCNET_MULTICAST_PORT = 4321;

// DE: Standard Node-ID dieses ESP32 Controllers / EN: Default Node ID of this ESP32 controller
constexpr uint16_t DEFAULT_ROCNET_NODE_ID = 204;

// DE: Standard-Knotenname (UID-Name in Rocrail) / EN: Default node name (UID name in Rocrail)
constexpr const char* DEFAULT_ROCNET_NODE_NAME = "ESP32-Signal";

// DE: Hersteller-ID (0 = Eigenbau / Unbekannt) / EN: Vendor ID (0 = DIY / Unknown, 1 = GCA)
constexpr uint8_t DEFAULT_ROCNET_VENDOR_ID = 0;

// DE: Firmware-Versionsnummer (Major / Minor) / EN: Firmware version numbers (Major / Minor)
constexpr uint8_t DEFAULT_ROCNET_VERSION_MAJOR = 1;
constexpr uint8_t DEFAULT_ROCNET_VERSION_MINOR = 0;

// DE: RocNet-Funktionsgruppen / EN: RocNet function groups (lib/rocnet-const.h)
constexpr uint8_t ROCNET_GROUP_TURNOUT        = 0x02; // RN_GROUP_MOBILE / Turnout straight/thrown
constexpr uint8_t ROCNET_GROUP_SERVO_DIRECT   = 0x03; // RN_GROUP_STATIONARY / Direct servo angle/pulse
constexpr uint8_t ROCNET_GROUP_ASPECT         = 0x08; // RN_GROUP_SENSOR / Aspect extended output
constexpr uint8_t ROCNET_GROUP_OUTPUT         = 0x09; // RN_GROUP_OUTPUT / Rocrail standard output

// ==============================================================================
// 3. WS2811 Signal Configuration / WS2811-Signalkonfiguration
// ==============================================================================
// DE: Daten-Pin für WS2811-LED-Kette / EN: GPIO data pin for WS2811 LED string
constexpr uint8_t WS2811_DATA_PIN = 32;

// DE: Standard-Anzahl physischer WS2811-Chips (je 3 Kanäle/LEDs) -> 16 Chips = 48 Kanäle
// EN: Default number of physical WS2811 chips (3 channels/LEDs each) -> 16 chips = 48 channels
constexpr uint16_t DEFAULT_WS2811_CHIP_COUNT = 16;

// DE: Standard-Signalhelligkeit (0..255) / EN: Default signal brightness (0..255)
constexpr uint8_t DEFAULT_SIGNAL_BRIGHTNESS = 255;

// DE: Signalbild-Masken (Bit 0 = LED1/Rot, Bit 1 = LED2/Grün, Bit 2 = LED3/Gelb bzw. Weiß)
// EN: Signal aspect bitmasks (Bit 0 = LED1/Red, Bit 1 = LED2/Green, Bit 2 = LED3/Yellow or White)
constexpr uint16_t ASPECT_1_MASK  = 0x0001; // DE: Rot statisch / EN: Red static
constexpr uint16_t ASPECT_1_BLINK = 0x0000;

constexpr uint16_t ASPECT_2_MASK  = 0x0002; // DE: Grün statisch / EN: Green static
constexpr uint16_t ASPECT_2_BLINK = 0x0000;

constexpr uint16_t ASPECT_3_MASK  = 0x0002; // DE: Gelb/Blinkend (wie Grün, aber blinkend) / EN: Flashing
constexpr uint16_t ASPECT_3_BLINK = 0x0002; 

constexpr uint16_t ASPECT_4_MASK  = 0x0004; // DE: Gelb/Weiß statisch / EN: Yellow/White static
constexpr uint16_t ASPECT_4_BLINK = 0x0000;

// DE: Blinkintervall in Millisekunden (500ms = 1 Hz) / EN: Blink interval in ms (500ms = 1 Hz)
constexpr uint32_t SIGNAL_BLINK_INTERVAL_MS = 500;

// ==============================================================================
// 4. PCA9685 Servo / Turnout Configuration / Weichenkonfiguration
// ==============================================================================
// DE: Hardware-I2C-Pins (Wemos Lolin32 Lite) / EN: Hardware I2C pins (Wemos Lolin32 Lite)
constexpr int I2C_SDA_PIN = 19;
constexpr int I2C_SCL_PIN = 18;

// DE: PCA9685 I2C-Basisadresse (Modul 1 = 0x40, Modul 2 = 0x41, ...)
// EN: PCA9685 I2C base address (Module 1 = 0x40, Module 2 = 0x41, ...)
constexpr uint8_t DEFAULT_PCA9685_BASE_ADDR = 0x40;
constexpr uint8_t DEFAULT_PCA9685_I2C_ADDR = DEFAULT_PCA9685_BASE_ADDR;

// DE: Modul-Grenzwerte (1..4 Module, 16..64 Servos) / EN: Module limits (1..4 modules, 16..64 servos)
constexpr uint8_t DEFAULT_PCA9685_MODULE_COUNT = 1;
constexpr uint8_t MAX_PCA9685_MODULES = 4;
constexpr uint8_t SERVOS_PER_MODULE = 16;
constexpr uint8_t MAX_SERVO_CHANNELS = MAX_PCA9685_MODULES * SERVOS_PER_MODULE; // 64 Servos
constexpr uint8_t SERVO_CHANNEL_COUNT = MAX_SERVO_CHANNELS;

// DE: PWM-Frequenz für Servos (50 Hz = 20 ms Periode) / EN: PWM frequency for servos (50 Hz = 20 ms)
constexpr float PCA9685_PWM_FREQ = 50.0f;

// DE: Umrechnung Winkel (0..180°) in RC-Impuls (500..2500 µs)
// EN: Helper converting angle degrees (0..180°) to standard RC servo pulse (500..2500 µs)
constexpr uint16_t degreesToUs(float deg) {
    return (uint16_t)(500.0f + (deg * 2000.0f / 180.0f));
}

// DE: Standard-Winkelgrenzen vor Web-Kalibrierung / EN: Default initial angle limits before calibration
constexpr uint16_t SERVO_DEFAULT_LEFT_ANGLE_DEG = 30;
constexpr uint16_t SERVO_DEFAULT_RIGHT_ANGLE_DEG = 150;

// DE: Absolute Impulsgrenzen (Vollbereich 0°..180°) / EN: Absolute pulse width limits (full 0°..180° range)
constexpr uint16_t SERVO_DEFAULT_MIN_PULSE_US = degreesToUs(0.0f);   // 500 µs
constexpr uint16_t SERVO_DEFAULT_MAX_PULSE_US = degreesToUs(180.0f); // 2500 µs

// DE: Weichen-Stellzeit in Millisekunden (S-Kurve) / EN: Turnout travel time in ms (smoothstep easing)
constexpr uint32_t SERVO_DEFAULT_TRAVEL_TIME_MS = 400;

// DE: Adressierungsmodus: true = Paarweise Ports (Rocrail Standard: Port 1=Gerade, Port 2=Abzweig -> Servo 1)
// EN: Addressing mode: true = Paired ports (Rocrail default: Port 1=Straight, Port 2=Thrown -> Servo 1)
constexpr bool TURNOUT_PAIRED_PORTS = true;

// DE: Zeitintervall für Interpolationsschritte in ms / EN: Interval between interpolation steps in ms
constexpr uint32_t SERVO_STEP_INTERVAL_MS = 20;

// DE: PWM-Abschaltung nach Erreichen der Endlage (verhindert Brummen & spart Strom)
// EN: Power off PWM signal on idle to eliminate servo humming and save power
constexpr bool SERVO_DEFAULT_SLEEP_ON_IDLE = true;
constexpr uint32_t SERVO_IDLE_TIMEOUT_MS = 500;
