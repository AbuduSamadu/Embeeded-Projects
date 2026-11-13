/**
 * @file Buzzer.cpp
 * @brief Implementation of the Buzzer alert driver.
 */

#include "drivers/Buzzer.h"

Buzzer::Buzzer(uint8_t pin, bool activeHigh, unsigned long onTimeMs, unsigned long offTimeMs)
    : _pin(pin),
      _activeHigh(activeHigh),
      _onTimeMs(onTimeMs),
      _offTimeMs(offTimeMs),
      _isEnabled(false),
      _isPhysicalHigh(false),
      _lastToggleTime(0) {}

void Buzzer::init() {
    pinMode(_pin, OUTPUT);
    deactivate();
}

void Buzzer::activate() {
    if (!_isEnabled) {
        _isEnabled = true;
        _lastToggleTime = millis();
        writeOutput(true); // Start pulse cycle immediately
    }
}

void Buzzer::deactivate() {
    _isEnabled = false;
    writeOutput(false);
    if (SystemConfig::BUZZER_IS_PASSIVE) {
        noTone(_pin);
    }
}

void Buzzer::update() {
    if (!_isEnabled) {
        return;
    }

    unsigned long now = millis();
    unsigned long interval = _isPhysicalHigh ? _onTimeMs : _offTimeMs;

    if (now - _lastToggleTime >= interval) {
        _lastToggleTime = now;
        writeOutput(!_isPhysicalHigh);
    }
}

bool Buzzer::isActive() const {
    return _isEnabled;
}

void Buzzer::beepOnce(unsigned long durationMs) {
    writeOutput(true);
    delay(durationMs);
    writeOutput(false);
}

void Buzzer::writeOutput(bool active) {
    _isPhysicalHigh = active;
    if (SystemConfig::BUZZER_IS_PASSIVE) {
        if (active) {
            tone(_pin, SystemConfig::BUZZER_TONE_FREQ_HZ);
        } else {
            noTone(_pin);
        }
    } else {
        uint8_t pinLevel = (_activeHigh ? (active ? HIGH : LOW) : (active ? LOW : HIGH));
        digitalWrite(_pin, pinLevel);
    }
}
