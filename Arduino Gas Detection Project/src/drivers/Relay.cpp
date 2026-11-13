/**
 * @file Relay.cpp
 * @brief Implementation of the Relay driver.
 */

#include "drivers/Relay.h"

Relay::Relay(uint8_t pin, bool activeLow)
    : _pin(pin),
      _activeLow(activeLow),
      _isEnergized(false) {}

void Relay::init() {
    // Write safe OFF state first before setting pinMode to prevent boot glitch
    uint8_t safeLevel = _activeLow ? HIGH : LOW;
    digitalWrite(_pin, safeLevel);
    pinMode(_pin, OUTPUT);
    _isEnergized = false;
}

void Relay::activate() {
    _isEnergized = true;
    applyState();
}

void Relay::deactivate() {
    _isEnergized = false;
    applyState();
}

void Relay::update() {
    // Relay holds static physical state; nothing to update periodically
}

bool Relay::isActive() const {
    return _isEnergized;
}

void Relay::applyState() {
    // If activeLow: energized = LOW, de-energized = HIGH
    // If activeHigh: energized = HIGH, de-energized = LOW
    uint8_t level = _activeLow ? (_isEnergized ? LOW : HIGH)
                               : (_isEnergized ? HIGH : LOW);
    digitalWrite(_pin, level);
}
