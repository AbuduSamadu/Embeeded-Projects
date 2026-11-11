#include "SystemController.h"

SystemController::SystemController()
    : _state(SystemState::INITIALIZING),
      _sensor(PIN_MQ2_AO),
      _display(LCD_I2C_ADDRESS, LCD_COLUMNS, LCD_ROWS),
      _buzzer(PIN_BUZZER, BUZZER_ACTIVE_LOGIC),
      _relay(PIN_RELAY, RELAY_ACTIVE_LOGIC),
      _gsm(PIN_GSM_RX, PIN_GSM_TX),
      _lastTelemetryPrintTime(0) {}

const char* SystemController::stateToString(SystemState s) {
    switch (s) {
        case SystemState::INITIALIZING: return "INITIALIZING";
        case SystemState::WARMUP:       return "WARMUP";
        case SystemState::NORMAL:       return "NORMAL";
        case SystemState::ALARM:        return "ALARM";
        case SystemState::RECOVERY:     return "RECOVERY";
        default:                        return "UNKNOWN";
    }
}

void SystemController::transitionTo(SystemState newState) {
    if (_state != newState) {
        Serial.printf("[STATE] %s -> %s\n", stateToString(_state), stateToString(newState));
        _state = newState;

        switch (_state) {
            case SystemState::WARMUP:
                _relay.setSafetyLockout(true);
                _buzzer.setAlarmActive(false);
                break;

            case SystemState::NORMAL:
                _relay.setSafetyLockout(false);
                // In normal state, demonstration lamp can be illuminated
                _relay.setNormalOperatingState(true);
                _buzzer.setAlarmActive(false);
                break;

            case SystemState::ALARM:
                // CRITICAL SAFETY INTERLOCK:
                // Force demonstration lamp OFF immediately in the presence of gas/smoke!
                _relay.setSafetyLockout(true);
                _buzzer.setAlarmActive(true);
                // Dispatch emergency SMS alert
                _gsm.sendAlertSMS(TARGET_PHONE_NUMBER);
                break;

            case SystemState::RECOVERY:
                _buzzer.setAlarmActive(false);
                // Optional recovery SMS notification
                _gsm.sendRecoverySMS(TARGET_PHONE_NUMBER);
                break;

            default:
                break;
        }
    }
}

void SystemController::setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    delay(500);

    Serial.println();
    Serial.println(F("=================================================================="));
    Serial.println(F("  IoT Smoke Detection, Gas Leakage Alert & Light Control System   "));
    Serial.println(F("=================================================================="));
    Serial.printf("[PINOUT] MQ-2 AO: A0 | LCD SDA: D2 (GPIO%d), SCL: D1 (GPIO%d)\n", PIN_LCD_SDA, PIN_LCD_SCL);
    Serial.printf("[PINOUT] Relay IN: D5 (GPIO%d) | Buzzer: D0 (GPIO%d)\n", PIN_RELAY, PIN_BUZZER);
    Serial.printf("[PINOUT] GSM RX: D6 (GPIO%d), TX: D7 (GPIO%d)\n", PIN_GSM_RX, PIN_GSM_TX);
    Serial.println(F("=================================================================="));

    // Initialize low-level peripherals
    _buzzer.begin();
    _relay.begin();
    _relay.setSafetyLockout(true); // Always boot safely with relay OFF

    _display.begin();
    _display.showInitializing();

    // Short boot-up acoustic chirp
    _buzzer.singleBeep(150);

    // Initialize sensor & GSM subsystem
    _sensor.begin();
    _gsm.begin();

    Serial.println(F("[SYSTEM] Initialization complete. Entering sensor warm-up phase..."));
    transitionTo(SystemState::WARMUP);
}

void SystemController::processStateTransitions() {
    bool sensorAlarm = _sensor.isAlarmActive();

    switch (_state) {
        case SystemState::INITIALIZING:
            // Handled during setup
            break;

        case SystemState::WARMUP:
            if (!_sensor.isWarmingUp()) {
                Serial.println(F("[SYSTEM] Sensor preheat conditioning completed. Entering NORMAL mode."));
                transitionTo(SystemState::NORMAL);
            }
            break;

        case SystemState::NORMAL:
            if (sensorAlarm) {
                Serial.println(F("[SYSTEM] !! ABNORMAL GAS/SMOKE CONCENTRATION DETECTED !!"));
                transitionTo(SystemState::ALARM);
            }
            break;

        case SystemState::ALARM:
            if (!sensorAlarm) {
                Serial.println(F("[SYSTEM] Air quality normalized. Commencing recovery sequence."));
                transitionTo(SystemState::RECOVERY);
            }
            break;

        case SystemState::RECOVERY:
            // Recovery complete, return to normal operation
            transitionTo(SystemState::NORMAL);
            break;
    }
}

void SystemController::printTelemetry() {
    uint32_t now = millis();
    if (now - _lastTelemetryPrintTime >= INTERVAL_STATUS_PRINT_MS) {
        _lastTelemetryPrintTime = now;

        SensorReading reading = _sensor.getLatestReading();
        GSMTelemetry gsmInfo = _gsm.getTelemetry();

        Serial.printf("[TELEMETRY] State: %-7s | ADC: %4u (Filt: %4u, ~%.2fV) | Relay: %-3s | Buzz: %-3s | GSM: %-10s | RSSI: %2d\n",
            stateToString(_state),
            reading.rawAdc,
            reading.filteredAdc,
            reading.equivalentVolts,
            _relay.isLampOn() ? "ON" : "OFF",
            _buzzer.isSounding() ? "ON" : "OFF",
            gsmInfo.isReadyForSMS ? "READY" : "NO_NET",
            gsmInfo.signalQualityRssi
        );
    }
}

void SystemController::processSerialCommands() {
    if (!Serial.available()) return;

    char cmd = static_cast<char>(Serial.read());
    while (Serial.available()) Serial.read(); // flush remaining chars

    switch (cmd) {
        case 't':
        case 'T':
            Serial.println(F("[TEST] Simulating High Gas Spike (ADC = 750)..."));
            _sensor.setSimulatedReading(750);
            break;

        case 'n':
        case 'N':
            Serial.println(F("[TEST] Simulating Normal Clean Air (ADC = 180)..."));
            _sensor.setSimulatedReading(180);
            break;

        case 'c':
        case 'C':
            Serial.println(F("[TEST] Cleared simulation. Reverting to physical sensor ADC."));
            _sensor.clearSimulatedReading();
            break;

        case 's':
        case 'S':
            Serial.println(F("[TEST] Manual SMS test dispatch requested..."));
            _gsm.sendAlertSMS(TARGET_PHONE_NUMBER);
            break;

        case 'l':
        case 'L':
            if (_state == SystemState::NORMAL) {
                bool newState = !_relay.isLampOn();
                Serial.printf("[TEST] Toggling demo lamp: %s\n", newState ? "ON" : "OFF");
                _relay.setNormalOperatingState(newState);
            } else {
                Serial.println(F("[TEST] Lamp control blocked by Safety Interlock!"));
            }
            break;

        case 'h':
        case 'H':
        case '?':
            Serial.println(F("\n--- Interactive Test Commands ---"));
            Serial.println(F("  't' : Simulate Gas Spike (ADC=750)"));
            Serial.println(F("  'n' : Simulate Normal Air (ADC=180)"));
            Serial.println(F("  'c' : Clear simulation (read live MQ-2)"));
            Serial.println(F("  's' : Send test alert SMS"));
            Serial.println(F("  'l' : Toggle demonstration lamp (in normal state)"));
            Serial.println(F("  'h' : Help menu\n"));
            break;

        default:
            break;
    }
}

void SystemController::update() {
    // 1. Update sensor module (reads ADC and evaluates thresholds)
    _sensor.update();

    // 2. Evaluate system state transitions
    processStateTransitions();

    // 3. Update peripheral controllers
    _buzzer.update();
    _relay.update();
    _gsm.update();

    // 4. Update display
    SensorReading currentReading = _sensor.getLatestReading();
    GSMTelemetry gsmInfo = _gsm.getTelemetry();
    _display.update(_state, currentReading, gsmInfo);

    // 5. Output serial telemetry and check interactive commands
    printTelemetry();
    processSerialCommands();

    // Yield CPU time to ESP8266 background WiFi/IP and soft watchdog timers
    yield();
}

SystemState SystemController::getCurrentState() const {
    return _state;
}
