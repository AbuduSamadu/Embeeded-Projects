#include "BuzzerController.h"

BuzzerController::BuzzerController(uint8_t pin, uint8_t activeLogic)
    : _pin(pin),
      _activeLogic(activeLogic),
      _inactiveLogic(activeLogic == HIGH ? LOW : HIGH),
      _alarmActive(false),
      _isCurrentlyHigh(false),
      _lastToggleTime(0),
      _singleBeepEndTime(0) {}

void BuzzerController::begin() {
    pinMode(_pin, OUTPUT);
    setBuzzerHardware(false);
}

void BuzzerController::setBuzzerHardware(bool on) {
    _isCurrentlyHigh = on;
    digitalWrite(_pin, on ? _activeLogic : _inactiveLogic);
}

void BuzzerController::setAlarmActive(bool active) {
    if (_alarmActive != active) {
        _alarmActive = active;
        if (!_alarmActive) {
            setBuzzerHardware(false);
        } else {
            _lastToggleTime = millis();
            setBuzzerHardware(true);
        }
    }
}

void BuzzerController::singleBeep(uint16_t durationMs) {
    _singleBeepEndTime = millis() + durationMs;
    setBuzzerHardware(true);
}

void BuzzerController::update() {
    uint32_t now = millis();

    // Priority 1: Persistent alarm mode (cadence pulsing)
    if (_alarmActive) {
        if (now - _lastToggleTime >= INTERVAL_BUZZER_PULSE_MS) {
            _lastToggleTime = now;
            setBuzzerHardware(!_isCurrentlyHigh);
        }
        return;
    }

    // Priority 2: One-shot non-blocking beep
    if (_singleBeepEndTime > 0) {
        if (now >= _singleBeepEndTime) {
            setBuzzerHardware(false);
            _singleBeepEndTime = 0;
        }
    }
}

bool BuzzerController::isSounding() const {
    return _isCurrentlyHigh;
}
