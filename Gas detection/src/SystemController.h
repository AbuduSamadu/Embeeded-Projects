#ifndef SYSTEM_CONTROLLER_H
#define SYSTEM_CONTROLLER_H

#include <Arduino.h>
#include "Config.h"
#include "SystemTypes.h"
#include "SensorManager.h"
#include "DisplayManager.h"
#include "BuzzerController.h"
#include "RelayController.h"
#include "GSMManager.h"

class SystemController {
public:
    SystemController();

    void setup();
    void update();

    SystemState getCurrentState() const;

private:
    SystemState      _state;
    SensorManager    _sensor;
    DisplayManager   _display;
    BuzzerController _buzzer;
    RelayController  _relay;
    GSMManager       _gsm;

    uint32_t         _lastTelemetryPrintTime;

    void processStateTransitions();
    void printTelemetry();
    void processSerialCommands();
    void transitionTo(SystemState newState);
    const char* stateToString(SystemState s);
};

#endif // SYSTEM_CONTROLLER_H
