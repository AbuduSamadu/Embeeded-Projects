/**
 * @file main.cpp
 * @brief Arduino Uno Smoke Detection and Gas Leakage Alert System with GSM Notification
 *        and Automatic Light Control.
 * 
 * Hardware Architecture:
 * - Microcontroller: Arduino Uno R3 (ATmega328P)
 * - Gas & Smoke Sensor: MQ-2 (AO -> A0)
 * - Display: 16x2 I2C LCD (SDA -> A4, SCL -> A5)
 * - Audible Alert: Buzzer (Control -> D8)
 * - Switching Device: Relay Module (IN -> D7, active-low)
 * - Output Device: 12V LED Demonstration Lamp (switched via Relay)
 * - GSM Module: SIM800L (SoftwareSerial: TXD -> D10, RXD -> D11 via level divider)
 * 
 * Design Principles:
 * - Separation of Concerns (Configuration, Drivers, Orchestrator decoupled)
 * - SOLID Principles (ISensor, IActuator, IDisplay, INotificationService interfaces)
 * - 100% Non-blocking cooperative multitasking via millis()
 * - Debouncing, filtering, and hysteresis for false-alarm rejection
 * - Anti-spam SMS rate limiting
 * - Safe fail-safe actuator states (demonstration lamp de-energized in alarm to avoid sparks)
 */

#ifndef UNIT_TEST

#include <Arduino.h>
#include "Config.h"
#include "PinDefinitions.h"
#include "drivers/MQ2Sensor.h"
#include "drivers/Buzzer.h"
#include "drivers/Relay.h"
#include "drivers/LcdDisplay.h"
#include "drivers/Sim800lGsm.h"
#include "controllers/SafetyController.h"

// =============================================================================
// COMPONENT INSTANTIATION (COMPOSITION ROOT)
// =============================================================================

// Sensor Driver: MQ-2 on Pin A0 with moving-average filtering & hysteresis
static MQ2Sensor mq2Sensor(
    HardwarePins::PIN_MQ2_ANALOG,
    SystemConfig::GAS_ALARM_THRESHOLD,
    SystemConfig::GAS_ALARM_HYSTERESIS,
    SystemConfig::SENSOR_SAMPLE_INTERVAL_MS
);

// Actuator Driver: Buzzer on Pin D8 with non-blocking pulsing cadence
static Buzzer alarmBuzzer(
    HardwarePins::PIN_BUZZER_CONTROL,
    SystemConfig::BUZZER_ACTIVE_HIGH,
    SystemConfig::BUZZER_CADENCE_ON_MS,
    SystemConfig::BUZZER_CADENCE_OFF_MS
);

// Actuator Driver: Relay on Pin D7 controlling 12V demonstration lamp
static Relay lampRelay(
    HardwarePins::PIN_RELAY_CONTROL,
    SystemConfig::RELAY_ACTIVE_LOW
);

// Display Driver: 16x2 I2C LCD on pins A4 (SDA) / A5 (SCL)
static LcdDisplay statusDisplay(
    SystemConfig::LCD_I2C_ADDRESS,
    SystemConfig::LCD_COLUMNS,
    SystemConfig::LCD_ROWS
);

// Notification Service: SIM800L GSM module on pins D10 (RX) / D11 (TX)
static Sim800lGsm gsmNotifier(
    HardwarePins::PIN_SIM800L_RX,
    HardwarePins::PIN_SIM800L_TX,
    SystemConfig::EMERGENCY_PHONE_NUMBER,
    SystemConfig::GSM_BAUD_RATE
);

// System Safety Controller (Orchestrator FSM)
static SafetyController safetySystem(
    mq2Sensor,
    alarmBuzzer,
    lampRelay,
    statusDisplay,
    gsmNotifier
);

// =============================================================================
// ARDUINO LIFECYCLE HOOKS
// =============================================================================

void setup() {
    // Initialize hardware UART for diagnostic logs
    Serial.begin(SystemConfig::SERIAL_MONITOR_BAUD);
    delay(200); // Allow hardware serial UART to stabilize

    // Initialize safety orchestrator and all peripherals
    safetySystem.begin();
}

void loop() {
    // Non-blocking cooperative execution tick
    safetySystem.process();
}

#endif // UNIT_TEST
