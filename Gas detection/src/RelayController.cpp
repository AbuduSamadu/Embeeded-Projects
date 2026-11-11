#include "RelayController.h"

RelayController::RelayController(uint8_t pin, uint8_t activeLogic)
    : _pin(pin),
      _activeLogic(activeLogic),
      _inactiveLogic(activeLogic == HIGH ? LOW : HIGH),
      _safetyLockout(true),       // Safe initial state: locked out until explicitly cleared
      _requestedNormalState(false),
      _isCurrentlyEnergized(false) {}

void RelayController::begin() {
    pinMode(_pin, OUTPUT);
    // Guarantee relay is physically OFF at startup
    digitalWrite(_pin, _inactiveLogic);
    _isCurrentlyEnergized = false;
}

void RelayController::applyHardwareState() {
    // Critical safety interlock: If safety lockout is active, force relay OFF unconditionally.
    if (_safetyLockout) {
        if (_isCurrentlyEnergized) {
            digitalWrite(_pin, _inactiveLogic);
            _isCurrentlyEnergized = false;
        }
        return;
    }

    bool shouldEnergize = _requestedNormalState;
    if (_isCurrentlyEnergized != shouldEnergize) {
        _isCurrentlyEnergized = shouldEnergize;
        digitalWrite(_pin, shouldEnergize ? _activeLogic : _inactiveLogic);
    }
}

void RelayController::setSafetyLockout(bool lockout) {
    _safetyLockout = lockout;
    applyHardwareState();
}

void RelayController::setNormalOperatingState(bool turnOn) {
    _requestedNormalState = turnOn;
    applyHardwareState();
}

bool RelayController::isLampOn() const {
    return _isCurrentlyEnergized;
}

bool RelayController::isLockoutActive() const {
    return _safetyLockout;
}

void RelayController::update() {
    applyHardwareState();
}
