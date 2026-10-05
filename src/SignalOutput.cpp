/*
 * ==============================================================================
 * QUELLENNACHWEIS & LIZENZ / SOURCE ATTRIBUTION & LICENSE:
 * ==============================================================================
 * DE: SignalOutput Implementierung: WS2811 Signalsteuerung via RMT-Hardware.
 * EN: SignalOutput implementation: WS2811 model railroad signal control via RMT.
 *
 * DE: Verwendet NeoPixelBus (Makuna, LGPLv3) und Rocrail Aspect-Bitmasken
 *     (https://wiki.rocrail.net/doku.php?id=aspects-en).
 * EN: Uses NeoPixelBus (Makuna, LGPLv3) and Rocrail aspect bitmask mapping
 *     (https://wiki.rocrail.net/doku.php?id=aspects-en).
 * ==============================================================================
 */

#include "SignalOutput.h"
#include "AppLog.h"

SignalOutput::SignalOutput(uint8_t pin)
    : _pin(pin),
      _numChips(0),
      _brightness(DEFAULT_SIGNAL_BRIGHTNESS),
      _dirty(false),
      _strip(nullptr) {
}

SignalOutput::~SignalOutput() {
    if (_strip != nullptr) {
        delete _strip;
        _strip = nullptr;
    }
}

bool SignalOutput::begin(uint8_t pin, uint16_t numChips, uint8_t brightness) {
    _pin = pin;
    return begin(numChips, brightness);
}

bool SignalOutput::begin(uint16_t numChips, uint8_t brightness) {
    _brightness = brightness;
    setChipCount(numChips);
    return (_strip != nullptr);
}

void SignalOutput::setChipCount(uint16_t numChips) {
    if (_strip != nullptr) {
        delete _strip;
        _strip = nullptr;
    }

    _numChips = (numChips > 0) ? numChips : 1;
    // Dynamically instantiate NeoPixelBus with RMT method for WS2811 on GPIO 32
    _strip = new NeoPixelBus<NeoRgbFeature, NeoWs2811Method>(_numChips, _pin);
    _strip->Begin();
    _strip->ClearTo(RgbColor(0, 0, 0));
    _strip->Show();
    _dirty = false;

    LOG_I("WS2811 SignalOutput initialized with %u chips (%u channels) on GPIO %u",
          _numChips, _numChips * 3, _pin);
}

uint16_t SignalOutput::getChipCount() const {
    return _numChips;
}

void SignalOutput::setBrightness(uint8_t brightness) {
    _brightness = brightness;
}

uint8_t SignalOutput::getBrightness() const {
    return _brightness;
}

void SignalOutput::setPortStartChannel(uint16_t port, uint16_t startChannel) {
    _portChannelMap[port] = startChannel;
}

uint16_t SignalOutput::getPortStartChannel(uint16_t port) const {
    auto it = _portChannelMap.find(port);
    if (it != _portChannelMap.end()) {
        return it->second;
    }
    // Default mapping: Port 1 -> Channel 0, Port 2 -> Channel 3, etc.
    return (port > 0) ? ((port - 1) * 3) : 0;
}

void SignalOutput::setAspect(uint16_t port, uint16_t aspectMask, uint16_t blinkMask) {
    if (_strip == nullptr || _numChips == 0) return;

    SignalPortState state;
    state.aspectMask = aspectMask;
    state.blinkMask = blinkMask;
    _portStateMap[port] = state;

    uint16_t startChannel = getPortStartChannel(port);

    LOG_D("Signal Port %u -> Aspect: 0x%04X, Blink: 0x%04X, startChannel: %u",
          port, aspectMask, blinkMask, startChannel);

    applyPortLeds(port, aspectMask, blinkMask, _blinkPhase);
    _dirty = true;
}

void SignalOutput::applyPortLeds(uint16_t port, uint16_t aspectMask, uint16_t blinkMask, bool blinkPhase) {
    if (_strip == nullptr || _numChips == 0) return;

    uint16_t startChannel = getPortStartChannel(port);

    for (uint8_t bit = 0; bit < 16; bit++) {
        uint16_t channelIndex = startChannel + bit;
        uint16_t pixelIndex = channelIndex / 3;
        uint8_t subChannel = channelIndex % 3; // 0 = Red, 1 = Green, 2 = Blue

        if (pixelIndex >= _numChips) {
            break;
        }

        bool isSet = (aspectMask & (1 << bit)) != 0;
        bool isBlinking = (blinkMask & (1 << bit)) != 0;
        bool ledOn = isSet && (!isBlinking || blinkPhase);
        uint8_t level = ledOn ? _brightness : 0;

        RgbColor color = _strip->GetPixelColor(pixelIndex);
        if (subChannel == 0) {
            color.R = level;
        } else if (subChannel == 1) {
            color.G = level;
        } else if (subChannel == 2) {
            color.B = level;
        }

        _strip->SetPixelColor(pixelIndex, color);
    }
}

void SignalOutput::clearAll() {
    _portStateMap.clear();
    if (_strip != nullptr) {
        _strip->ClearTo(RgbColor(0, 0, 0));
        _dirty = true;
    }
}

void SignalOutput::update() {
    uint32_t now = millis();

    // Check if any port has blinking LEDs
    bool hasBlinking = false;
    for (const auto& kv : _portStateMap) {
        if (kv.second.blinkMask != 0) {
            hasBlinking = true;
            break;
        }
    }

    if (hasBlinking && (now - _lastBlinkToggleMs >= SIGNAL_BLINK_INTERVAL_MS)) {
        _lastBlinkToggleMs = now;
        _blinkPhase = !_blinkPhase;

        for (const auto& kv : _portStateMap) {
            if (kv.second.blinkMask != 0) {
                applyPortLeds(kv.first, kv.second.aspectMask, kv.second.blinkMask, _blinkPhase);
            }
        }
        _dirty = true;
    }

    if (_dirty && _strip != nullptr && _strip->CanShow()) {
        _strip->Show();
        _dirty = false;
    }
}

void SignalOutput::testPattern() {
    if (_strip == nullptr || _numChips == 0) return;

    LOG_I("Running WS2811 visual test pattern...");
    for (uint16_t i = 0; i < _numChips; i++) {
        _strip->SetPixelColor(i, RgbColor(_brightness / 4, 0, 0));
    }
    _strip->Show();
    delay(200);

    for (uint16_t i = 0; i < _numChips; i++) {
        _strip->SetPixelColor(i, RgbColor(0, _brightness / 4, 0));
    }
    _strip->Show();
    delay(200);

    for (uint16_t i = 0; i < _numChips; i++) {
        _strip->SetPixelColor(i, RgbColor(0, 0, _brightness / 4));
    }
    _strip->Show();
    delay(200);

    _strip->ClearTo(RgbColor(0, 0, 0));
    _strip->Show();
}
