#pragma once

/*
 * ==============================================================================
 * QUELLENNACHWEIS / SOURCE ATTRIBUTION:
 * ==============================================================================
 * SignalOutput: Ansteuerung von Signalen via WS2811 LED-Ketten
 *
 * Externe Bibliothek:
 *   - NeoPixelBus by Makuna (LGPLv3) für ESP32 RMT-Hardware-Treiber
 *
 * Modellbahn- & Protokollbezug:
 *   - Aspect-Bitmasken-Interpretation und Signalbild-Zuordnung gemäß Rocrail:
 *     https://wiki.rocrail.net/doku.php?id=aspects-en
 *     https://www.rocrail-wiki.net/doku.php?id=rocnet:rocnet-prot-en
 *     (Copyright (c) Robert Jan Versluis, Rocrail.net)
 * ==============================================================================
 */

#include <Arduino.h>
#include <NeoPixelBus.h>
#include <map>
#include "Config.h"

// Encapsulation of WS2811 LED outputs for railway signals using NeoPixelBus via RMT
class SignalOutput {
public:
    explicit SignalOutput(uint8_t pin = WS2811_DATA_PIN);
    ~SignalOutput();

    // Initialize with dynamic pin, number of WS2811 chips and initial brightness
    bool begin(uint8_t pin, uint16_t numChips = DEFAULT_WS2811_CHIP_COUNT, uint8_t brightness = DEFAULT_SIGNAL_BRIGHTNESS);
    bool begin(uint16_t numChips = DEFAULT_WS2811_CHIP_COUNT, uint8_t brightness = DEFAULT_SIGNAL_BRIGHTNESS);

    // Dynamic reconfiguration of chip count at runtime
    void setChipCount(uint16_t numChips);
    uint16_t getChipCount() const;

    // Configurable brightness (0..255)
    void setBrightness(uint8_t brightness);
    uint8_t getBrightness() const;

    // Map a Rocrail signal port (1-based) to an arbitrary start channel (0-based)
    // Default mapping if not explicitly mapped: startChannel = (port - 1) * 3
    void setPortStartChannel(uint16_t port, uint16_t startChannel);
    uint16_t getPortStartChannel(uint16_t port) const;

    // Set aspect for a given port (1-based) according to Rocrail aspect bitmask
    // blinkMask specifies which bits/LEDs of the aspect should blink
    void setAspect(uint16_t port, uint16_t aspectMask, uint16_t blinkMask = 0);

    // Turn off all outputs
    void clearAll();

    // Non-blocking update loop (handles blinking timer and calls Show() when needed)
    void update();

    // Visual test sequence for hardware verification
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
