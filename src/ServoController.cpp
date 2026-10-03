/*
 * ==============================================================================
 * QUELLENNACHWEIS / SOURCE ATTRIBUTION:
 * ==============================================================================
 * ServoController Implementierung: PCA9685 I2C Weichenservos
 * Verwendet Adafruit PWM Servo Driver Library (BSD-Lizenz) und implementiert
 * RocNet Port-Mapping und Konfigurationsparameter (OffPos, OnPos, Steps).
 * ==============================================================================
 */

#include "ServoController.h"
#include "AppLog.h"
#include <cmath>
#ifdef ARDUINO
#include <Preferences.h>
#endif

ServoController::ServoController()
    : _moduleCount(DEFAULT_PCA9685_MODULE_COUNT),
      _baseI2cAddr(DEFAULT_PCA9685_BASE_ADDR),
      _sdaPin(I2C_SDA_PIN),
      _sclPin(I2C_SCL_PIN),
      _pairedPorts(TURNOUT_PAIRED_PORTS),
      _highestTurnoutPort(0),
      _lastStepTime(0),
      _initialized(false) {
    for (uint8_t m = 0; m < MAX_PCA9685_MODULES; m++) {
        _moduleOnline[m] = false;
    }

    // 1. Initialize channel configs with defaults
    for (uint8_t ch = 0; ch < MAX_SERVO_CHANNELS; ch++) {
        _config[ch].offPosAngle = SERVO_DEFAULT_LEFT_ANGLE_DEG;
        _config[ch].onPosAngle  = SERVO_DEFAULT_RIGHT_ANGLE_DEG;
        _config[ch].minPulseUs  = degreesToUs((float)_config[ch].offPosAngle);
        _config[ch].maxPulseUs  = degreesToUs((float)_config[ch].onPosAngle);
        _config[ch].inverted    = false;
        _config[ch].sleepOnIdle = SERVO_DEFAULT_SLEEP_ON_IDLE;
        _config[ch].travelDurationMs = SERVO_DEFAULT_TRAVEL_TIME_MS;

        _state[ch].currentPulseUs = _config[ch].minPulseUs;
        _state[ch].startPulseUs = _config[ch].minPulseUs;
        _state[ch].targetPulseUs = _config[ch].minPulseUs;
        _state[ch].turnoutState = false;
        _state[ch].isMoving = false;
        _state[ch].pwmActive = false;
        _state[ch].idleSince = 0;
        _state[ch].switchCount = 0;
    }

    // 2. Load any user-programmed angles saved in Flash/NVS
    loadPreferences();
}

bool ServoController::begin(uint8_t baseI2cAddr, int sdaPin, int sclPin) {
    _baseI2cAddr = baseI2cAddr;
    _sdaPin = sdaPin;
    _sclPin = sclPin;

    loadPreferences();

    log_i("Initializing I2C for PCA9685 bus: SDA=%d, SCL=%d, BaseAddr=0x%02X, ModuleCount=%u",
          sdaPin, sclPin, baseI2cAddr, _moduleCount);
    Wire.begin(sdaPin, sclPin);

    _initialized = true;
    _lastStepTime = millis();

    // Initialize all configured modules
    for (uint8_t m = 0; m < _moduleCount; m++) {
        initModule(m);
    }

    return true;
}

void ServoController::initModule(uint8_t moduleIdx) {
    if (moduleIdx >= MAX_PCA9685_MODULES) return;

    uint8_t addr = _baseI2cAddr + moduleIdx;
    Wire.beginTransmission(addr);
    uint8_t err = Wire.endTransmission();

    if (err == 0) {
        _pwm[moduleIdx] = Adafruit_PWMServoDriver(addr);
        _pwm[moduleIdx].begin();
        _pwm[moduleIdx].setOscillatorFrequency(25000000);
        _pwm[moduleIdx].setPWMFreq(PCA9685_PWM_FREQ);
        _moduleOnline[moduleIdx] = true;
        log_i("PCA9685 Module %u at 0x%02X is ONLINE (Servos %u..%u)",
              moduleIdx + 1, addr, moduleIdx * SERVOS_PER_MODULE + 1, (moduleIdx + 1) * SERVOS_PER_MODULE);

        // Position all channels of this module to initial position
        for (uint8_t pin = 0; pin < SERVOS_PER_MODULE; pin++) {
            uint8_t ch = moduleIdx * SERVOS_PER_MODULE + pin;
            uint16_t initialPulse = _config[ch].inverted ? _config[ch].maxPulseUs : _config[ch].minPulseUs;
            _state[ch].currentPulseUs = initialPulse;
            _state[ch].startPulseUs = initialPulse;
            _state[ch].targetPulseUs = initialPulse;
            writePulseUs(ch, initialPulse);
            _state[ch].idleSince = millis();
        }
    } else {
        _moduleOnline[moduleIdx] = false;
        log_w("PCA9685 Module %u at 0x%02X NOT found (I2C error %u)", moduleIdx + 1, addr, err);
    }
}

bool ServoController::isModuleOnline(uint8_t moduleIdx) const {
    if (moduleIdx < MAX_PCA9685_MODULES) {
        return _moduleOnline[moduleIdx];
    }
    return false;
}

bool ServoController::setModuleCount(uint8_t count, bool saveToNvs) {
    if (count < 1) count = 1;
    if (count > MAX_PCA9685_MODULES) count = MAX_PCA9685_MODULES;
    _moduleCount = count;

    if (_initialized) {
        for (uint8_t m = 0; m < _moduleCount; m++) {
            initModule(m);
        }
    }

#ifdef ARDUINO
    if (saveToNvs) {
        Preferences prefs;
        if (prefs.begin("rn_servo", false)) {
            prefs.putUChar("mod_cnt", _moduleCount);
            prefs.end();
            log_i("Saved module count: %u to Flash NVS", _moduleCount);
        }
    }
#endif
    return true;
}

void ServoController::setTurnout(uint8_t port, bool thrown, uint32_t durationMs) {
    if (port < 1) return;

    // Smart Auto-Scale: If port exceeds current capacity, expand PCA9685 modules up to MAX_PCA9685_MODULES
    if (port > getTotalServos() && port <= MAX_SERVO_CHANNELS) {
        uint8_t requiredModules = ((port - 1) / SERVOS_PER_MODULE) + 1;
        log_i("⚡ Smart Auto-Scale: Servo Port %u exceeds capacity -> expanding PCA9685 from %u to %u module(s) (%u servos)",
              port, _moduleCount, requiredModules, requiredModules * SERVOS_PER_MODULE);
        setModuleCount(requiredModules, true);
    }

    if (port > getTotalServos()) {
        log_w("Turnout port %u out of range (1..%u)", port, getTotalServos());
        return;
    }

    uint8_t channel = port - 1;
    _state[channel].turnoutState = thrown;
    _state[channel].switchCount++;
    if (port > _highestTurnoutPort) {
        _highestTurnoutPort = port;
    }

    // Determine target pulse based on straight/thrown and inversion
    bool targetIsThrown = thrown;
    if (_config[channel].inverted) {
        targetIsThrown = !targetIsThrown;
    }

    uint16_t targetPulse = targetIsThrown ? _config[channel].maxPulseUs : _config[channel].minPulseUs;
    uint32_t travelMs = (durationMs > 0) ? durationMs : _config[channel].travelDurationMs;

    log_i("Turnout Port %u (Servo %u, Board %u, Pin %u) -> %s (Target Pulse: %u us, Travel: %u ms, Switch #%u)",
          port, channel + 1, (channel / SERVOS_PER_MODULE) + 1, channel % SERVOS_PER_MODULE,
          thrown ? "THROWN (Abzweig)" : "STRAIGHT (Gerade)",
          targetPulse, travelMs, _state[channel].switchCount);

    moveToPulse(channel, targetPulse, travelMs);
}

void ServoController::setTurnoutPort(uint8_t port, uint8_t state, uint32_t durationMs) {
    uint8_t channel = 0;
    bool thrown = false;

    if (_pairedPorts) {
        // Paired ports (Rocrail default with Single Gate unchecked):
        // Port 1 = Servo 1 Straight (Left), Port 2 = Servo 1 Thrown (Right)
        // Port 3 = Servo 2 Straight,        Port 4 = Servo 2 Thrown, etc.
        // Port 33 = Servo 17 Straight,      Port 34 = Servo 17 Thrown, etc.
        channel = (port > 0) ? (port - 1) / 2 : 0;
        thrown = ((port % 2) == 0); // Even port = Thrown, Odd port = Straight
    } else {
        // 1:1 Direct Channel (Single Gate): Port 1 = Servo 1, Port 2 = Servo 2
        channel = (port > 0) ? (port - 1) : 0;
        thrown = (state != 0);
    }

    // Smart Auto-Scale: If channel exceeds current capacity, expand PCA9685 modules up to MAX_PCA9685_MODULES
    if (channel >= getTotalServos() && channel < MAX_SERVO_CHANNELS) {
        uint8_t requiredModules = (channel / SERVOS_PER_MODULE) + 1;
        log_i("⚡ Smart Auto-Scale: Turnout Port %u maps to Servo %u -> expanding PCA9685 from %u to %u module(s) (%u servos)",
              port, channel + 1, _moduleCount, requiredModules, requiredModules * SERVOS_PER_MODULE);
        setModuleCount(requiredModules, true);
    }

    if (channel >= getTotalServos()) {
        log_w("Turnout channel %u out of range (0..%u) from Port %u", channel, getTotalServos() - 1, port);
        return;
    }

    // Rocrail sends 0..10 for servo rotation speed in Switch Time:
    // If durationMs is in range 1..10, scale by 100 ms (e.g. 5 -> 500ms)
    uint32_t effectiveDurationMs = durationMs;
    if (effectiveDurationMs > 0 && effectiveDurationMs <= 10) {
        effectiveDurationMs = effectiveDurationMs * 100;
    } else if (effectiveDurationMs == 0) {
        effectiveDurationMs = _config[channel].travelDurationMs;
    }

    log_i("Turnout Port %u (State %u, Raw Delay %u -> %u ms) -> Servo %u (Board %u Pin %u) -> %s",
          port, state, durationMs, effectiveDurationMs, channel + 1,
          (channel / SERVOS_PER_MODULE) + 1, channel % SERVOS_PER_MODULE,
          thrown ? "THROWN (Right)" : "STRAIGHT (Left)");
    setTurnout(channel + 1, thrown, effectiveDurationMs);
}

void ServoController::setServoDirect(uint8_t port, uint16_t value) {
    if (port < 1) return;

    // Smart Auto-Scale: If port exceeds current capacity, expand PCA9685 modules up to MAX_PCA9685_MODULES
    if (port > getTotalServos() && port <= MAX_SERVO_CHANNELS) {
        uint8_t requiredModules = ((port - 1) / SERVOS_PER_MODULE) + 1;
        log_i("⚡ Smart Auto-Scale: Servo Direct Port %u exceeds capacity -> expanding PCA9685 from %u to %u module(s) (%u servos)",
              port, _moduleCount, requiredModules, requiredModules * SERVOS_PER_MODULE);
        setModuleCount(requiredModules, true);
    }

    if (port > getTotalServos()) {
        log_w("Servo port %u out of range (1..%u)", port, getTotalServos());
        return;
    }

    uint8_t channel = port - 1;
    _state[channel].switchCount++;
    if (port > _highestTurnoutPort) {
        _highestTurnoutPort = port;
    }

    uint16_t targetPulse = 1500;

    if (value <= 180) {
        targetPulse = degreesToUs((float)value);
    } else if (value <= 512) {
        targetPulse = (uint16_t)(((uint32_t)value * 20000UL) / 4096UL);
    } else {
        targetPulse = constrain(value, 500, 2500);
    }

    log_i("Servo Direct Port %u (Servo %u) -> Angle/Value: %u -> Target Pulse: %u us (Switch #%u)",
          port, channel + 1, value, targetPulse, _state[channel].switchCount);

    moveToPulse(channel, targetPulse, 0);
}

void ServoController::moveToPulse(uint8_t channel, uint16_t targetPulseUs, uint32_t durationMs) {
    if (channel >= getTotalServos()) return;

    if ((uint16_t)round(_state[channel].currentPulseUs) == targetPulseUs && !_state[channel].isMoving) {
        return;
    }

    uint16_t currentPulse = (uint16_t)round(_state[channel].currentPulseUs);
    uint32_t baseDuration = (durationMs > 0) ? durationMs : _config[channel].travelDurationMs;

    if (baseDuration == 0) {
        _state[channel].currentPulseUs = targetPulseUs;
        _state[channel].startPulseUs = targetPulseUs;
        _state[channel].targetPulseUs = targetPulseUs;
        _state[channel].isMoving = false;
        _state[channel].idleSince = millis();
        writePulseUs(channel, targetPulseUs);
        return;
    }

    int32_t fullSpan = abs((int32_t)_config[channel].maxPulseUs - (int32_t)_config[channel].minPulseUs);
    int32_t currentDist = abs((int32_t)targetPulseUs - (int32_t)currentPulse);
    uint32_t actualDuration = baseDuration;
    if (fullSpan > 0 && currentDist < fullSpan) {
        actualDuration = (uint32_t)(((uint64_t)baseDuration * currentDist) / fullSpan);
        if (actualDuration < 40) actualDuration = 40;
    }

    _state[channel].startPulseUs = currentPulse;
    _state[channel].targetPulseUs = targetPulseUs;
    _state[channel].moveStartTime = millis();
    _state[channel].moveDurationMs = actualDuration;
    _state[channel].isMoving = true;

    writePulseUs(channel, currentPulse);
}

void ServoController::programChannel(uint8_t channel, uint16_t offPos, uint16_t onPos, uint8_t offSteps, uint8_t onSteps, uint8_t options, uint32_t travelMs) {
    if (channel >= MAX_SERVO_CHANNELS) return;

    // Smart Auto-Scale: If channel exceeds current capacity, expand PCA9685 modules up to MAX_PCA9685_MODULES
    if (channel >= getTotalServos()) {
        uint8_t requiredModules = (channel / SERVOS_PER_MODULE) + 1;
        log_i("⚡ Smart Auto-Scale: Program Channel %u -> expanding PCA9685 from %u to %u module(s) (%u servos)",
              channel + 1, _moduleCount, requiredModules, requiredModules * SERVOS_PER_MODULE);
        setModuleCount(requiredModules, true);
    }

    _config[channel].offPosAngle = offPos;
    _config[channel].onPosAngle  = onPos;
    _config[channel].offSteps    = offSteps;
    _config[channel].onSteps     = onSteps;
    _config[channel].options     = options;
    _config[channel].inverted    = ((options & 0x01) != 0);
    if (travelMs > 0) {
        _config[channel].travelDurationMs = travelMs;
    }

    if (offPos <= 180 && onPos <= 180) {
        _config[channel].minPulseUs = degreesToUs((float)offPos);
        _config[channel].maxPulseUs = degreesToUs((float)onPos);
    } else if (offPos <= 512 && onPos <= 512) {
        _config[channel].minPulseUs = (uint16_t)(((uint32_t)offPos * 20000UL) / 4096UL);
        _config[channel].maxPulseUs = (uint16_t)(((uint32_t)onPos * 20000UL) / 4096UL);
    } else {
        _config[channel].minPulseUs = constrain(offPos, 500, 2500);
        _config[channel].maxPulseUs = constrain(onPos, 500, 2500);
    }

    log_i("Programmed Servo %u: Left=%u (%u us), Right=%u (%u us), Inverted=%d, Travel=%u ms",
          channel + 1, offPos, _config[channel].minPulseUs, onPos, _config[channel].maxPulseUs,
          _config[channel].inverted, _config[channel].travelDurationMs);

    saveChannelPreferences(channel);
}

void ServoController::testServoLeft(uint8_t channel) {
    if (channel >= getTotalServos()) return;
    _state[channel].switchCount++;
    _state[channel].turnoutState = false;
    if (channel + 1 > _highestTurnoutPort) {
        _highestTurnoutPort = channel + 1;
    }
    uint16_t targetPulse = _config[channel].inverted ? _config[channel].maxPulseUs : _config[channel].minPulseUs;
    log_i("WebUI Test Left: Servo %u -> %u us (%u deg, Switch #%u)", channel + 1, targetPulse, _config[channel].offPosAngle, _state[channel].switchCount);
    moveToPulse(channel, targetPulse, _config[channel].travelDurationMs);
}

void ServoController::testServoRight(uint8_t channel) {
    if (channel >= getTotalServos()) return;
    _state[channel].switchCount++;
    _state[channel].turnoutState = true;
    if (channel + 1 > _highestTurnoutPort) {
        _highestTurnoutPort = channel + 1;
    }
    uint16_t targetPulse = _config[channel].inverted ? _config[channel].minPulseUs : _config[channel].maxPulseUs;
    log_i("WebUI Test Right: Servo %u -> %u us (%u deg, Switch #%u)", channel + 1, targetPulse, _config[channel].onPosAngle, _state[channel].switchCount);
    moveToPulse(channel, targetPulse, _config[channel].travelDurationMs);
}

void ServoController::testServoAngle(uint8_t channel, uint16_t angleDeg) {
    if (channel >= getTotalServos()) return;
    uint16_t pulse = degreesToUs((float)constrain(angleDeg, 0, 180));
    moveToPulse(channel, pulse, 0);
}

void ServoController::loadPreferences() {
#ifdef ARDUINO
    Preferences prefs;
    if (!prefs.begin("rn_servo", true)) {
        return;
    }
    _moduleCount = prefs.getUChar("mod_cnt", DEFAULT_PCA9685_MODULE_COUNT);
    if (_moduleCount < 1) _moduleCount = 1;
    if (_moduleCount > MAX_PCA9685_MODULES) _moduleCount = MAX_PCA9685_MODULES;

    for (uint8_t ch = 0; ch < MAX_SERVO_CHANNELS; ch++) {
        char keyMin[16], keyMax[16], keyOpt[16], keySpd[16];
        snprintf(keyMin, sizeof(keyMin), "s%u_off", ch);
        snprintf(keyMax, sizeof(keyMax), "s%u_on", ch);
        snprintf(keyOpt, sizeof(keyOpt), "s%u_opt", ch);
        snprintf(keySpd, sizeof(keySpd), "s%u_spd", ch);

        if (prefs.isKey(keyMin)) {
            uint16_t offPos = prefs.getUShort(keyMin, _config[ch].offPosAngle);
            uint16_t onPos  = prefs.getUShort(keyMax, _config[ch].onPosAngle);
            uint8_t opt     = prefs.getUChar(keyOpt, _config[ch].options);
            uint16_t spd    = prefs.getUShort(keySpd, (uint16_t)_config[ch].travelDurationMs);

            _config[ch].offPosAngle = offPos;
            _config[ch].onPosAngle  = onPos;
            _config[ch].options     = opt;
            _config[ch].inverted    = ((opt & 0x01) != 0);
            if (spd > 0) _config[ch].travelDurationMs = spd;

            if (offPos <= 180 && onPos <= 180) {
                _config[ch].minPulseUs = degreesToUs((float)offPos);
                _config[ch].maxPulseUs = degreesToUs((float)onPos);
            } else if (offPos <= 512 && onPos <= 512) {
                _config[ch].minPulseUs = (uint16_t)(((uint32_t)offPos * 20000UL) / 4096UL);
                _config[ch].maxPulseUs = (uint16_t)(((uint32_t)onPos * 20000UL) / 4096UL);
            } else {
                _config[ch].minPulseUs = constrain(offPos, 500, 2500);
                _config[ch].maxPulseUs = constrain(onPos, 500, 2500);
            }
        }
    }
    prefs.end();
    log_i("Loaded NVS: %u module(s), total %u servos configured", _moduleCount, getTotalServos());
#endif
}

void ServoController::saveChannelPreferences(uint8_t channel) {
#ifdef ARDUINO
    if (channel >= MAX_SERVO_CHANNELS) return;
    Preferences prefs;
    if (prefs.begin("rn_servo", false)) {
        char keyMin[16], keyMax[16], keyOpt[16], keySpd[16];
        snprintf(keyMin, sizeof(keyMin), "s%u_off", channel);
        snprintf(keyMax, sizeof(keyMax), "s%u_on", channel);
        snprintf(keyOpt, sizeof(keyOpt), "s%u_opt", channel);
        snprintf(keySpd, sizeof(keySpd), "s%u_spd", channel);

        prefs.putUShort(keyMin, _config[channel].offPosAngle);
        prefs.putUShort(keyMax, _config[channel].onPosAngle);
        prefs.putUChar(keyOpt, _config[channel].options);
        prefs.putUShort(keySpd, (uint16_t)_config[channel].travelDurationMs);
        prefs.end();
        log_i("Saved Servo %u config to Flash NVS", channel + 1);
    }
#endif
}

void ServoController::saveAllPreferences() {
#ifdef ARDUINO
    Preferences prefs;
    if (prefs.begin("rn_servo", false)) {
        prefs.putUChar("mod_cnt", _moduleCount);
        for (uint8_t ch = 0; ch < getTotalServos(); ch++) {
            char keyMin[16], keyMax[16], keyOpt[16], keySpd[16];
            snprintf(keyMin, sizeof(keyMin), "s%u_off", ch);
            snprintf(keyMax, sizeof(keyMax), "s%u_on", ch);
            snprintf(keyOpt, sizeof(keyOpt), "s%u_opt", ch);
            snprintf(keySpd, sizeof(keySpd), "s%u_spd", ch);

            prefs.putUShort(keyMin, _config[ch].offPosAngle);
            prefs.putUShort(keyMax, _config[ch].onPosAngle);
            prefs.putUChar(keyOpt, _config[ch].options);
            prefs.putUShort(keySpd, (uint16_t)_config[ch].travelDurationMs);
        }
        prefs.end();
        log_i("Saved all %u servos to Flash NVS", getTotalServos());
    }
#endif
}

void ServoController::setChannelConfig(uint8_t channel, const ServoChannelConfig& config) {
    if (channel < MAX_SERVO_CHANNELS) {
        _config[channel] = config;
    }
}

ServoChannelConfig ServoController::getChannelConfig(uint8_t channel) const {
    if (channel < MAX_SERVO_CHANNELS) {
        return _config[channel];
    }
    return ServoChannelConfig();
}

const ServoChannelState& ServoController::getChannelState(uint8_t channel) const {
    if (channel < MAX_SERVO_CHANNELS) {
        return _state[channel];
    }
    static ServoChannelState emptyState;
    return emptyState;
}

void ServoController::writePulseUs(uint8_t channel, uint16_t pulseUs) {
    if (!_initialized || channel >= getTotalServos()) return;
    uint8_t board = channel / SERVOS_PER_MODULE;
    uint8_t pin   = channel % SERVOS_PER_MODULE;
    if (board >= _moduleCount || !_moduleOnline[board]) return;

    uint16_t ticks = (uint16_t)(((uint32_t)pulseUs * 4096UL) / 20000UL);
    _pwm[board].setPWM(pin, 0, ticks);
    _state[channel].pwmActive = true;
}

void ServoController::update() {
    if (!_initialized) return;

    uint32_t now = millis();
    if (now - _lastStepTime < SERVO_STEP_INTERVAL_MS) {
        return;
    }
    _lastStepTime = now;

    for (uint8_t ch = 0; ch < getTotalServos(); ch++) {
        if (_state[ch].isMoving) {
            uint32_t elapsed = now - _state[ch].moveStartTime;

            if (elapsed >= _state[ch].moveDurationMs) {
                _state[ch].currentPulseUs = _state[ch].targetPulseUs;
                _state[ch].isMoving = false;
                _state[ch].idleSince = now;
                writePulseUs(ch, _state[ch].targetPulseUs);
                if (_turnoutCompletionCallback) {
                    _turnoutCompletionCallback(ch, _state[ch].turnoutState);
                }
            } else {
                float t = (float)elapsed / (float)_state[ch].moveDurationMs;
                float pulse = (float)_state[ch].startPulseUs +
                              ((float)_state[ch].targetPulseUs - (float)_state[ch].startPulseUs) * t;

                _state[ch].currentPulseUs = pulse;
                writePulseUs(ch, (uint16_t)round(pulse));
            }
        } else if (_config[ch].sleepOnIdle && _state[ch].pwmActive) {
            if (now - _state[ch].idleSince >= SERVO_IDLE_TIMEOUT_MS) {
                uint8_t board = ch / SERVOS_PER_MODULE;
                uint8_t pin   = ch % SERVOS_PER_MODULE;
                if (board < _moduleCount && _moduleOnline[board]) {
                    _pwm[board].setPin(pin, 0); // Sets PCA9685 full OFF (0V)
                }
                _state[ch].pwmActive = false;
                LOG_I("Servo %u -> Stromlos (PWM OFF after %u ms holding time)", ch + 1, SERVO_IDLE_TIMEOUT_MS);
            }
        }
    }
}

uint8_t ServoController::getI2cScanMask() const {
    uint8_t mask = 0;
    for (uint8_t i = 0; i < MAX_PCA9685_MODULES; i++) {
        if (_moduleOnline[i]) {
            mask |= (1 << i);
        }
    }
    return mask;
}

uint32_t ServoController::getSwitchCount(uint8_t channel) const {
    if (channel < MAX_SERVO_CHANNELS) {
        return _state[channel].switchCount;
    }
    return 0;
}

uint32_t ServoController::getTotalSwitchCount() const {
    uint32_t total = 0;
    for (uint8_t ch = 0; ch < getTotalServos(); ch++) {
        total += _state[ch].switchCount;
    }
    return total;
}

uint8_t ServoController::getActiveTurnoutCount() const {
    uint8_t count = 0;
    for (uint8_t ch = 0; ch < getTotalServos(); ch++) {
        if (_state[ch].switchCount > 0) {
            count++;
        }
    }
    return count;
}

void ServoController::resetSwitchCounters() {
    for (uint8_t ch = 0; ch < MAX_SERVO_CHANNELS; ch++) {
        _state[ch].switchCount = 0;
    }
    _highestTurnoutPort = 0;
    log_i("Turnout switch counters reset to 0");
}
