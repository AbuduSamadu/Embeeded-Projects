/**
 * @file SafetyController.cpp
 * @brief Implementation of the Safety Controller and FSM.
 */

#include "controllers/SafetyController.h"

SafetyController::SafetyController(ISensor& sensor,
                                   IActuator& buzzer,
                                   IActuator& relay,
                                   IDisplay& display,
                                   INotificationService& notificationService)
    : _sensor(sensor),
      _buzzer(buzzer),
      _relay(relay),
      _display(display),
      _notification(notificationService),
      _state(SystemState::INITIALIZING),
      _stateStartTime(0),
      _alarmTriggerStartTime(0),
      _recoveryStartTime(0),
      _lastSerialLogTime(0),
      _smsDispatchedForCurrentAlarm(false) {}

void SafetyController::begin() {
    Serial.println(F("=================================================="));
    Serial.println(F("  SMOKE & GAS LEAK DETECTION ALERT SYSTEM (UNO)   "));
    Serial.println(F("=================================================="));

    // Initialize display first to show status
    _display.init();
    _display.showMessage("SYSTEM INIT...", "STARTING HW...");

    // Initialize actuators in safe default states
    _relay.init();      // Relay de-energized (Lamp OFF)
    _buzzer.init();     // Buzzer silent (OFF)

    // Initialize sensor
    _sensor.init();

    // Initialize GSM module asynchronously
    _notification.init();

    Serial.println(F("[SAFETY] Peripherals initialized. Starting sensor warm-up..."));
    transitionTo(SystemState::WARMUP);
}

void SafetyController::process() {
    // 1. Non-blocking update of all hardware peripherals
    _sensor.update();
    _buzzer.update();
    _relay.update();
    _notification.update();

    // 2. FSM state processing
    switch (_state) {
        case SystemState::INITIALIZING:
            handleInitializing();
            break;
        case SystemState::WARMUP:
            handleWarmup();
            break;
        case SystemState::NORMAL:
            handleNormal();
            break;
        case SystemState::ALARM:
            handleAlarm();
            break;
        case SystemState::RECOVERY:
            handleRecovery();
            break;
    }

    // 3. Update LCD with fresh telemetry
    _display.update(getTelemetry());

    // 4. Periodic diagnostic logging to Serial Monitor
    logHeartbeatDiagnostics();
}

void SafetyController::handleInitializing() {
    transitionTo(SystemState::WARMUP);
}

void SafetyController::handleWarmup() {
    unsigned long elapsed = millis() - _stateStartTime;
    if (elapsed >= SystemConfig::SENSOR_WARMUP_TIME_MS) {
        Serial.println(F("[SAFETY] Warm-up complete. Entering NORMAL monitoring mode."));
        transitionTo(SystemState::NORMAL);

        // Turn demonstration lamp to configured normal operating state
        if (SystemConfig::RELAY_LAMP_NORMAL_ON) {
            _relay.activate();
            Serial.println(F("[SAFETY] Demonstration lamp turned ON (Normal Mode)."));
        } else {
            _relay.deactivate();
        }
    }
}

void SafetyController::handleNormal() {
    if (_sensor.isThresholdExceeded()) {
        if (_alarmTriggerStartTime == 0) {
            _alarmTriggerStartTime = millis();
            Serial.println(F("[SAFETY] Potential gas detected. Debounce confirmation started..."));
        } else if (millis() - _alarmTriggerStartTime >= SystemConfig::ALARM_TRIGGER_CONFIRM_MS) {
            Serial.println(F("[SAFETY] ALARM CONFIRMED! Abnormal smoke/gas detected!"));
            transitionTo(SystemState::ALARM);
        }
    } else {
        _alarmTriggerStartTime = 0; // Reset debounce if reading drops back
    }
}

void SafetyController::handleAlarm() {
    // Keep demonstration lamp OFF to prevent electrical sparks in gas atmosphere
    if (_relay.isActive()) {
        _relay.deactivate();
    }

    // Dispatch SMS alert once per alarm activation
    if (!_smsDispatchedForCurrentAlarm) {
        if (_notification.sendAlert()) {
            _smsDispatchedForCurrentAlarm = true;
            Serial.println(F("[SAFETY] Alert notification requested."));
        }
    }

    // Check for recovery condition
    if (_sensor.isRecoveryLevel()) {
        if (_recoveryStartTime == 0) {
            _recoveryStartTime = millis();
            Serial.println(F("[SAFETY] Gas reading dropped below recovery threshold. Monitoring stability..."));
        } else if (millis() - _recoveryStartTime >= SystemConfig::ALARM_RECOVERY_CONFIRM_MS) {
            Serial.println(F("[SAFETY] Recovery confirmed. Entering RECOVERY state."));
            transitionTo(SystemState::RECOVERY);
        }
    } else {
        _recoveryStartTime = 0; // Reset recovery timer if gas spikes again
    }
}

void SafetyController::handleRecovery() {
    // Silence buzzer
    _buzzer.deactivate();

    // Send recovery SMS if configured
    if (SystemConfig::ENABLE_RECOVERY_SMS && _smsDispatchedForCurrentAlarm) {
        _notification.sendRecovery();
    }

    // Reset alarm tracking
    _smsDispatchedForCurrentAlarm = false;

    // Restore demonstration lamp to normal operating state
    if (SystemConfig::RELAY_LAMP_NORMAL_ON) {
        _relay.activate();
        Serial.println(F("[SAFETY] Normal state restored. Demonstration lamp turned ON."));
    }

    // Transition back to normal monitoring
    transitionTo(SystemState::NORMAL);
}

void SafetyController::transitionTo(SystemState newState) {
    _state = newState;
    _stateStartTime = millis();
    _alarmTriggerStartTime = 0;
    _recoveryStartTime = 0;

    switch (newState) {
        case SystemState::INITIALIZING:
            break;

        case SystemState::WARMUP:
            _buzzer.deactivate();
            _relay.deactivate();
            break;

        case SystemState::NORMAL:
            _buzzer.deactivate();
            if (SystemConfig::RELAY_LAMP_NORMAL_ON) {
                _relay.activate();
            } else {
                _relay.deactivate();
            }
            break;

        case SystemState::ALARM:
            _relay.deactivate(); // Safe state: cut power to demonstration lamp
            _buzzer.activate();   // Audible warning cadence
            break;

        case SystemState::RECOVERY:
            _buzzer.deactivate();
            break;
    }
}

SystemTelemetry SafetyController::getTelemetry() const {
    SystemTelemetry telemetry;
    telemetry.systemState = _state;
    telemetry.rawSensorValue = _sensor.getRawValue();
    telemetry.filteredSensorValue = _sensor.getFilteredValue();

    if (_state == SystemState::WARMUP) {
        unsigned long elapsed = millis() - _stateStartTime;
        if (elapsed < SystemConfig::SENSOR_WARMUP_TIME_MS) {
            telemetry.warmupRemainingSeconds = (SystemConfig::SENSOR_WARMUP_TIME_MS - elapsed) / 1000UL;
        } else {
            telemetry.warmupRemainingSeconds = 0;
        }
    } else {
        telemetry.warmupRemainingSeconds = 0;
    }

    telemetry.buzzerActive = _buzzer.isActive();
    telemetry.lampRelayActive = _relay.isActive();
    telemetry.gsmStatus = _notification.getNetworkStatus();
    telemetry.smsAlertDispatched = _smsDispatchedForCurrentAlarm;

    return telemetry;
}

void SafetyController::logHeartbeatDiagnostics() {
    unsigned long now = millis();
    if (now - _lastSerialLogTime < SystemConfig::SERIAL_LOG_INTERVAL_MS) {
        return;
    }
    _lastSerialLogTime = now;

    Serial.print(F("[STATUS] State: "));
    switch (_state) {
        case SystemState::INITIALIZING: Serial.print(F("INIT")); break;
        case SystemState::WARMUP:       Serial.print(F("WARMUP")); break;
        case SystemState::NORMAL:       Serial.print(F("NORMAL")); break;
        case SystemState::ALARM:        Serial.print(F("ALARM!")); break;
        case SystemState::RECOVERY:     Serial.print(F("RECOVERY")); break;
    }

    Serial.print(F(" | Sensor: Raw="));
    Serial.print(_sensor.getRawValue());
    Serial.print(F(" Filt="));
    Serial.print(_sensor.getFilteredValue());

    Serial.print(F(" | Relay: "));
    Serial.print(_relay.isActive() ? F("ON") : F("OFF"));

    Serial.print(F(" | Buzzer: "));
    Serial.print(_buzzer.isActive() ? F("ON") : F("OFF"));

    Serial.print(F(" | GSM: "));
    switch (_notification.getNetworkStatus()) {
        case GsmNetworkStatus::REGISTERED_HOME:    Serial.print(F("HOME_NET")); break;
        case GsmNetworkStatus::REGISTERED_ROAMING: Serial.print(F("ROAMING")); break;
        case GsmNetworkStatus::SEARCHING:          Serial.print(F("SEARCHING")); break;
        case GsmNetworkStatus::NOT_REGISTERED:     Serial.print(F("NOT_REG")); break;
        case GsmNetworkStatus::REGISTRATION_DENIED:Serial.print(F("DENIED")); break;
        default:                                   Serial.print(F("UNKNOWN")); break;
    }

    Serial.println();
}
