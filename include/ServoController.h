#pragma once

/*
 * ==============================================================================
 * QUELLENNACHWEIS & LIZENZ / SOURCE ATTRIBUTION & LICENSE:
 * ==============================================================================
 * DE: ServoController: Ansteuerung von Weichenservos via PCA9685 I2C-PWM-Treiber
 * EN: ServoController: RC servo turnout control via PCA9685 I2C PWM driver
 *
 * Externe Bibliothek / External Library:
 *   - Adafruit PWM Servo Driver Library (BSD-Lizenz / BSD License, Adafruit Industries)
 *
 * Modellbahn- & Protokollbezug / Model Railroad & Protocol Reference:
 *   - DE: Weichen-Portadressierung und Parameter (Paired Ports, OffPos/OnPos,
 *         OffSteps/OnSteps) gemäß Rocrail & RocNet-Spezifikation.
 *   - EN: Turnout port addressing and channel parameters (Paired Ports, OffPos/OnPos,
 *         OffSteps/OnSteps) according to Rocrail & RocNet specifications:
 *     https://www.rocrail-wiki.net/doku.php?id=rocnet:rocnet-prot-en#programming_stationary
 *     https://wiki.rocrail.net/doku.php?id=switch-int-en
 *     (Copyright (c) Robert Jan Versluis, Rocrail.net)
 * ==============================================================================
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include "Config.h"

// DE: Konfigurationsparameter eines einzelnen Servokanals
// EN: Configuration parameters for an individual servo channel
struct ServoChannelConfig {
    uint16_t minPulseUs = SERVO_DEFAULT_MIN_PULSE_US;         // DE: Impuls Gerade (µs) / EN: Pulse for straight (µs)
    uint16_t maxPulseUs = SERVO_DEFAULT_MAX_PULSE_US;         // DE: Impuls Abzweig (µs) / EN: Pulse for thrown (µs)
    uint16_t offPosAngle = 0;                                 // DE: Grenzwinkel Links / EN: Angle degrees for Left/Straight
    uint16_t onPosAngle = 180;                                // DE: Grenzwinkel Rechts / EN: Angle degrees for Right/Thrown
    uint8_t offSteps = 0;
    uint8_t onSteps = 0;
    uint8_t options = 0;
    uint32_t travelDurationMs = SERVO_DEFAULT_TRAVEL_TIME_MS; // DE: Stellzeit in ms / EN: Travel duration in ms
    bool inverted = false;                                    // DE: Richtung umkehren / EN: Invert direction
    bool sleepOnIdle = SERVO_DEFAULT_SLEEP_ON_IDLE;           // DE: PWM nach Stillstand aus / EN: Turn off PWM on idle
};

// DE: Dynamischer Laufzeitzustand eines Servokanals
// EN: Current dynamic runtime state of a servo channel
struct ServoChannelState {
    float currentPulseUs = 1500.0f;
    uint16_t startPulseUs = 1500;
    uint16_t targetPulseUs = 1500;
    uint32_t moveStartTime = 0;
    uint32_t moveDurationMs = SERVO_DEFAULT_TRAVEL_TIME_MS;
    bool isMoving = false;
    bool turnoutState = false; // DE: false = Gerade, true = Abzweig / EN: false = straight, true = thrown
    uint32_t idleSince = 0;
    bool pwmActive = false;
    uint32_t switchCount = 0; // DE: Schaltspiel-Zähler / EN: Turnout switch cycle counter
};

class ServoController {
public:
    ServoController();

    // DE: PCA9685-Modul(e) auf I2C initialisieren / EN: Initialize PCA9685 module(s) on I2C bus
    bool begin(uint8_t baseI2cAddr = DEFAULT_PCA9685_BASE_ADDR, int sdaPin = I2C_SDA_PIN, int sclPin = I2C_SCL_PIN);

    // DE: Modulverwaltung (1..MAX_PCA9685_MODULES) / EN: Module count management (1..4 modules)
    uint8_t getModuleCount() const { return _moduleCount; }
    bool setModuleCount(uint8_t count, bool saveToNvs = true);
    uint8_t getTotalServos() const { return _moduleCount * SERVOS_PER_MODULE; }
    bool isModuleOnline(uint8_t moduleIdx) const;

    // DE: Weiche nach Port schalten (1-basiert) / EN: Switch turnout by port (1-based)
    // thrown: false = Gerade / straight, true = Abzweig / thrown
    void setTurnout(uint8_t port, bool thrown, uint32_t durationMs = 0);

    // DE: Weichenport mit Paarweise/Einzeln-Logik / EN: Switch turnout handling paired/single ports
    void setTurnoutPort(uint8_t port, uint8_t state, uint32_t durationMs = 0);
    void setPairedPorts(bool paired) { _pairedPorts = paired; }
    bool getPairedPorts() const { return _pairedPorts; }

    // DE: Servo direkt positionieren / EN: Direct servo positioning by port
    // value: 0..180 (Grad / deg) oder >180 (Impuls in µs / pulse in us)
    void setServoDirect(uint8_t port, uint16_t value);

    // DE: Kanalkonfiguration dynamisch programmieren / EN: Program channel configuration dynamically
    void programChannel(uint8_t channel, uint16_t offPos, uint16_t onPos, uint8_t offSteps = 0, uint8_t onSteps = 0, uint8_t options = 0, uint32_t travelMs = 0);

    // DE: Auf Zielimpuls mit S-Kurve fahren / EN: Move to target pulse with smoothstep easing
    void moveToPulse(uint8_t channel, uint16_t targetPulseUs, uint32_t durationMs = 0);

    // DE: Kanal-Konfiguration / EN: Channel configuration getters & setters
    void setChannelConfig(uint8_t channel, const ServoChannelConfig& config);
    ServoChannelConfig getChannelConfig(uint8_t channel) const;
    const ServoChannelState& getChannelState(uint8_t channel) const;

    // DE: Flash NVS Laden und Speichern / EN: Load & save channel settings to ESP32 Flash NVS
    void loadPreferences();
    void saveChannelPreferences(uint8_t channel);
    void saveAllPreferences();

    // DE: Live-Testfunktionen für WebUI / EN: Live test functions for WebUI
    void testServoLeft(uint8_t channel);
    void testServoRight(uint8_t channel);
    void testServoAngle(uint8_t channel, uint16_t angleDeg);

    // DE: Schaltspiel-Zähler & Statistiken / EN: Turnout switch cycle counters & statistics
    uint32_t getSwitchCount(uint8_t channel) const;
    uint32_t getTotalSwitchCount() const;
    uint8_t getActiveTurnoutCount() const;
    uint8_t getHighestTurnoutPort() const { return _highestTurnoutPort; }
    void resetSwitchCounters();

    // DE: Endlagen-Callback & Status / EN: Turnout completion callback and status queries
    void setTurnoutCompletionCallback(std::function<void(uint8_t channel, bool thrown)> cb) {
        _turnoutCompletionCallback = cb;
    }
    bool getTurnoutState(uint8_t channel) const {
        return (channel < MAX_SERVO_CHANNELS) ? _state[channel].turnoutState : false;
    }
    uint8_t getI2cScanMask() const;

    // DE: Nicht-blockierende Schritt-Aktualisierung im loop() / EN: Non-blocking step update in loop()
    void update();

private:
    std::function<void(uint8_t channel, bool thrown)> _turnoutCompletionCallback;
    Adafruit_PWMServoDriver _pwm[MAX_PCA9685_MODULES];
    bool _moduleOnline[MAX_PCA9685_MODULES];
    uint8_t _moduleCount;
    uint8_t _baseI2cAddr;
    int _sdaPin;
    int _sclPin;
    bool _pairedPorts;
    uint8_t _highestTurnoutPort;

    ServoChannelConfig _config[MAX_SERVO_CHANNELS];
    ServoChannelState _state[MAX_SERVO_CHANNELS];
    uint32_t _lastStepTime;
    bool _initialized;

    void initModule(uint8_t moduleIdx);
    void writePulseUs(uint8_t channel, uint16_t pulseUs);
};
