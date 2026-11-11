#ifndef RELAY_CONTROLLER_H
#define RELAY_CONTROLLER_H

#include <Arduino.h>
#include "Config.h"

class RelayController {
public:
    RelayController(uint8_t pin = PIN_RELAY, uint8_t activeLogic = RELAY_ACTIVE_LOGIC);

    void begin();
    void setSafetyLockout(bool lockout);
    void setNormalOperatingState(bool turnOn);
    bool isLampOn() const;
    bool isLockoutActive() const;
    void update();

private:
    uint8_t _pin;
    uint8_t _activeLogic;
    uint8_t _inactiveLogic;
    bool    _safetyLockout;
    bool    _requestedNormalState;
    bool    _isCurrentlyEnergized;

    void applyHardwareState();
};

#endif // RELAY_CONTROLLER_H
