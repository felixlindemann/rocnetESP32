#pragma once

/*
 * ==============================================================================
 * QUELLENNACHWEIS & LIZENZ / SOURCE ATTRIBUTION & LICENSE:
 * ==============================================================================
 * DE: SignalOutput: Ansteuerung von Signalen via WS2811 LED-Ketten
 * EN: SignalOutput: Model railway signal control via WS2811 LED strings
 *
 * Externe Bibliothek / External Library:
 *   - NeoPixelBus by Makuna (LGPLv3) für ESP32 RMT-Hardware-Treiber
 *
 * Modellbahn- & Protokollbezug / Model Railroad & Protocol Reference:
 *   - DE: Aspect-Bitmasken-Interpretation und Signalbild-Zuordnung gemäß Rocrail:
 *   - EN: Aspect bitmask interpretation and signal mapping according to Rocrail:
 *     https://wiki.rocrail.net/doku.php?id=aspects-en
 *     https://www.rocrail-wiki.net/doku.php?id=rocnet:rocnet-prot-en
 *     (Copyright (c) Robert Jan Versluis, Rocrail.net)
 * ==============================================================================
 */

#include <Arduino.h>
#include <NeoPixelBus.h>
#include <map>
#include "Config.h"

// DE: Kapselung der WS2811-LED-Ausgänge für Signale via NeoPixelBus (RMT-Hardware)
// EN: Encapsulation of WS2811 LED outputs for railway signals using NeoPixelBus via RMT
class SignalOutput {
public:
    explicit SignalOutput(uint8_t pin = WS2811_DATA_PIN);
    ~SignalOutput();

    // DE: Initialisierung mit Pin, Chipanzahl und Helligkeit
    // EN: Initialize with dynamic pin, number of WS2811 chips and initial brightness
    bool begin(uint8_t pin, uint16_t numChips = DEFAULT_WS2811_CHIP_COUNT, uint8_t brightness = DEFAULT_SIGNAL_BRIGHTNESS);
    bool begin(uint16_t numChips = DEFAULT_WS2811_CHIP_COUNT, uint8_t brightness = DEFAULT_SIGNAL_BRIGHTNESS);

    // DE: Chipanzahl zur Laufzeit rekonfigurieren / EN: Dynamic reconfiguration of chip count
    void setChipCount(uint16_t numChips);
    uint16_t getChipCount() const;

    // DE: Einstellbare Gesamthelligkeit (0..255) / EN: Configurable brightness (0..255)
    void setBrightness(uint8_t brightness);
    uint8_t getBrightness() const;

    // DE: Port auf Startkanal mappen (Standard: (port - 1) * 3)
    // EN: Map a Rocrail signal port (1-based) to an arbitrary start channel (0-based)
    void setPortStartChannel(uint16_t port, uint16_t startChannel);
    uint16_t getPortStartChannel(uint16_t port) const;

    // DE: Signalbegriff (Aspect) mit Bitmaske setzen / EN: Set aspect according to Rocrail aspect bitmask
    void setAspect(uint16_t port, uint16_t aspectMask, uint16_t blinkMask = 0);

    // DE: Alle Ausgänge ausschalten / EN: Turn off all outputs
    void clearAll();

    // DE: Nicht-blockierende Blinksteuerung im loop() / EN: Non-blocking update loop (handles blinking timer)
    void update();

    // DE: Optisches Testmuster für Signale / EN: Visual test sequence for hardware verification
    void testPattern();

private:
    uint8_t _pin;
    uint16_t _numChips;
    uint8_t _brightness;
    bool _dirty;

    struct SignalPortState {
        uint16_t aspectMask = 0;
        uint16_t blinkMask = 0;
    };
    std::map<uint16_t, SignalPortState> _portStateMap;
    uint32_t _lastBlinkToggleMs = 0;
    bool _blinkPhase = true;

    void applyPortLeds(uint16_t port, uint16_t aspectMask, uint16_t blinkMask, bool blinkPhase);

    NeoPixelBus<NeoRgbFeature, NeoWs2811Method>* _strip;
    std::map<uint16_t, uint16_t> _portChannelMap;
};
