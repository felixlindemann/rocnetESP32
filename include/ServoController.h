#pragma once

/*
 * ==============================================================================
 * QUELLENNACHWEIS / SOURCE ATTRIBUTION:
 * ==============================================================================
 * ServoController: Ansteuerung von Weichenservos via PCA9685 I2C-PWM-Treiber
 *
 * Externe Bibliothek:
 *   - Adafruit PWM Servo Driver Library (BSD-Lizenz, Adafruit Industries)
 *
 * Modellbahn- & Protokollbezug:
 *   - Weichen-Portadressierung und Parameter (Paired Ports, OffPos/OnPos,
 *     OffSteps/OnSteps) gemäß Rocrail & RocNet-Spezifikation:
 *     https://www.rocrail-wiki.net/doku.php?id=rocnet:rocnet-prot-en#programming_stationary
 *     https://wiki.rocrail.net/doku.php?id=switch-int-en
 *     (Copyright (c) Robert Jan Versluis, Rocrail.net)
 * ==============================================================================
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include "Config.h"

// Configuration parameters for an individual servo channel
struct ServoChannelConfig {
    uint16_t minPulseUs = SERVO_DEFAULT_MIN_PULSE_US;         // Pulse for straight (us)
    uint16_t maxPulseUs = SERVO_DEFAULT_MAX_PULSE_US;         // Pulse for thrown (us)
    uint16_t offPosAngle = 0;                                 // Angle in degrees for Left/Straight
    uint16_t onPosAngle = 180;                                // Angle in degrees for Right/Thrown
    uint8_t offSteps = 0;
    uint8_t onSteps = 0;
    uint8_t options = 0;
    uint32_t travelDurationMs = SERVO_DEFAULT_TRAVEL_TIME_MS; // Travel duration in ms
    bool inverted = false;                                    // Invert direction
    bool sleepOnIdle = SERVO_DEFAULT_SLEEP_ON_IDLE;           // Turn off PWM when movement is done
};

// Current dynamic runtime state of a servo channel
struct ServoChannelState {
    float currentPulseUs = 1500.0f;
    uint16_t startPulseUs = 1500;
    uint16_t targetPulseUs = 1500;
    uint32_t moveStartTime = 0;
    uint32_t moveDurationMs = SERVO_DEFAULT_TRAVEL_TIME_MS;
    bool isMoving = false;
    bool turnoutState = false; // false = straight, true = thrown
    uint32_t idleSince = 0;
    bool pwmActive = false;
    uint32_t switchCount = 0; // Turnout switch cycle counter (Schaltspiel-Zähler)
};

class ServoController {
public:
    ServoController();

    // Initialize PCA9685 module(s) on hardware I2C bus
    bool begin(uint8_t baseI2cAddr = DEFAULT_PCA9685_BASE_ADDR, int sdaPin = I2C_SDA_PIN, int sclPin = I2C_SCL_PIN);

    // Module count management (1..MAX_PCA9685_MODULES)
    uint8_t getModuleCount() const { return _moduleCount; }
    bool setModuleCount(uint8_t count, bool saveToNvs = true);
    uint8_t getTotalServos() const { return _moduleCount * SERVOS_PER_MODULE; }
    bool isModuleOnline(uint8_t moduleIdx) const;

    // Switch turnout by port (1-based, 1..totalServos)
    // thrown: false = Straight (Gerade), true = Thrown (Abzweigend)
    void setTurnout(uint8_t port, bool thrown, uint32_t durationMs = 0);

    // Switch turnout handling paired ports or direct ports from RocNet
    void setTurnoutPort(uint8_t port, uint8_t state, uint32_t durationMs = 0);
    void setPairedPorts(bool paired) { _pairedPorts = paired; }
    bool getPairedPorts() const { return _pairedPorts; }

    // Direct servo positioning by port (1-based, 1..totalServos)
    // value: 0..180 (degrees) or >180 (pulse in us)
    void setServoDirect(uint8_t port, uint16_t value);

    // Program channel configuration dynamically from RocNet Group 5 Action 16 or Web-UI
    void programChannel(uint8_t channel, uint16_t offPos, uint16_t onPos, uint8_t offSteps = 0, uint8_t onSteps = 0, uint8_t options = 0, uint32_t travelMs = 0);

    // Move channel to a target pulse width with non-blocking smoothstep interpolation
    void moveToPulse(uint8_t channel, uint16_t targetPulseUs, uint32_t durationMs = 0);

    // Channel configuration
    void setChannelConfig(uint8_t channel, const ServoChannelConfig& config);
    ServoChannelConfig getChannelConfig(uint8_t channel) const;
    const ServoChannelState& getChannelState(uint8_t channel) const;

    // Load and save channel settings to ESP32 Flash/NVS
    void loadPreferences();
    void saveChannelPreferences(uint8_t channel);
    void saveAllPreferences();

    // Live test functions for Web-UI
    void testServoLeft(uint8_t channel);
    void testServoRight(uint8_t channel);
    void testServoAngle(uint8_t channel, uint16_t angleDeg);

    // Turnout switch cycle counters & statistics
    uint32_t getSwitchCount(uint8_t channel) const;
    uint32_t getTotalSwitchCount() const;
    uint8_t getActiveTurnoutCount() const;
    uint8_t getHighestTurnoutPort() const { return _highestTurnoutPort; }
    void resetSwitchCounters();

    // Turnout completion callback and status queries
    void setTurnoutCompletionCallback(std::function<void(uint8_t channel, bool thrown)> cb) {
        _turnoutCompletionCallback = cb;
    }
    bool getTurnoutState(uint8_t channel) const {
        return (channel < MAX_SERVO_CHANNELS) ? _state[channel].turnoutState : false;
    }
    uint8_t getI2cScanMask() const;

    // Non-blocking step update, called continuously from loop()
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
