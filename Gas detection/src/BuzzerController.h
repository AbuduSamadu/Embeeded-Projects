#ifndef BUZZER_CONTROLLER_H
#define BUZZER_CONTROLLER_H

#include <Arduino.h>
#include "Config.h"

class BuzzerController {
public:
    BuzzerController(uint8_t pin = PIN_BUZZER, uint8_t activeLogic = BUZZER_ACTIVE_LOGIC);

    void begin();
    void setAlarmActive(bool active);
    void singleBeep(uint16_t durationMs = 100);
    void update();
    bool isSounding() const;

private:
    uint8_t  _pin;
    uint8_t  _activeLogic;
    uint8_t  _inactiveLogic;
    bool     _alarmActive;
    bool     _isCurrentlyHigh;
    uint32_t _lastToggleTime;
    uint32_t _singleBeepEndTime;

    void setBuzzerHardware(bool on);
};

#endif // BUZZER_CONTROLLER_H
