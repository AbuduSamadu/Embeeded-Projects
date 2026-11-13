/**
 * @file SafetyController.h
 * @brief Application Orchestrator and Finite State Machine.
 * 
 * Implements Dependency Inversion: depends on abstract interfaces (ISensor,
 * IActuator, IDisplay, INotificationService), decoupling core safety logic
 * from concrete hardware drivers.
 */

#ifndef SAFETY_CONTROLLER_H
#define SAFETY_CONTROLLER_H

#include "SystemTypes.h"
#include "interfaces/ISensor.h"
#include "interfaces/IActuator.h"
#include "interfaces/IDisplay.h"
#include "interfaces/INotificationService.h"
#include "Config.h"

class SafetyController {
public:
    SafetyController(ISensor& sensor,
                     IActuator& buzzer,
                     IActuator& relay,
                     IDisplay& display,
                     INotificationService& notificationService);

    /// Perform system-wide initialization sequence
    void begin();

    /// Main non-blocking tick routine to be invoked in Arduino loop()
    void process();

    /// Process an interactive bench testing/simulation command character
    void handleSerialCommand(char cmd);

    /// Display interactive bench command help menu on Serial Monitor
    void printHelpMenu();

    /// Get current system telemetry snapshot
    SystemTelemetry getTelemetry() const;

private:
    ISensor& _sensor;
    IActuator& _buzzer;
    IActuator& _relay;
    IDisplay& _display;
    INotificationService& _notification;

    SystemState _state;
    unsigned long _stateStartTime;
    unsigned long _alarmTriggerStartTime;
    unsigned long _recoveryStartTime;
    unsigned long _lastSerialLogTime;
    bool _smsDispatchedForCurrentAlarm;

    void handleInitializing();
    void handleWarmup();
    void handleNormal();
    void handleAlarm();
    void handleRecovery();

    void transitionTo(SystemState newState);
    void logHeartbeatDiagnostics();
    void printCurrentStatus();
};

#endif // SAFETY_CONTROLLER_H
