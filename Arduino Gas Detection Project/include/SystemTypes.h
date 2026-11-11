/**
 * @file SystemTypes.h
 * @brief Common types, enumerations, and data structures for the system.
 */

#ifndef SYSTEM_TYPES_H
#define SYSTEM_TYPES_H

#include <Arduino.h>

/**
 * @brief Overall operational state of the safety monitoring system.
 */
enum class SystemState : uint8_t {
    INITIALIZING,       ///< Bootstrapping hardware peripherals
    WARMUP,             ///< MQ-2 chemical sensor thermal stabilization
    NORMAL,             ///< Environmental air within normal limits
    ALARM,              ///< Elevated smoke/gas level detected (alarm active)
    RECOVERY            ///< Reading returning to baseline, debouncing prior to normal
};

/**
 * @brief Current evaluation of sensor reading relative to thresholds.
 */
enum class SensorEvaluation : uint8_t {
    NORMAL,             ///< Safe ambient level
    ABOVE_THRESHOLD,    ///< Exceeds alarm threshold
    CLEARING            ///< Dropping below threshold but within hysteresis band
};

/**
 * @brief Cellular network registration status reported by SIM800L (+CREG).
 */
enum class GsmNetworkStatus : uint8_t {
    UNKNOWN,            ///< Status not yet determined
    NOT_REGISTERED,     ///< Module not registered, not searching
    REGISTERED_HOME,    ///< Registered on home cellular network
    SEARCHING,          ///< Searching for network operator
    REGISTRATION_DENIED,///< Network rejected registration
    REGISTERED_ROAMING  ///< Registered on roaming network
};

/**
 * @brief Internal state of the SIM800L asynchronous SMS sender state machine.
 */
enum class GsmState : uint8_t {
    IDLE,               ///< Ready to process requests
    CHECK_AT,           ///< Sending AT synchronization ping
    SET_ECHO_OFF,       ///< Disabling command echo (ATE0)
    CHECK_PIN,          ///< Checking SIM card status (AT+CPIN?)
    CHECK_NETWORK,      ///< Querying cellular registration (AT+CREG?)
    SET_TEXT_MODE,      ///< Setting SMS text mode (AT+CMGF=1)
    SEND_SMS_CMD,       ///< Initiating SMS dispatch (AT+CMGS="...")
    SEND_SMS_BODY,      ///< Sending SMS body and Ctrl+Z
    WAIT_SMS_CONFIRM,   ///< Awaiting network transmission confirmation
    COOLDOWN            ///< Post-SMS dispatch cooldown
};

/**
 * @brief Snapshot of the system state for telemetry and display.
 */
struct SystemTelemetry {
    SystemState systemState;
    uint16_t rawSensorValue;
    uint16_t filteredSensorValue;
    unsigned long warmupRemainingSeconds;
    bool buzzerActive;
    bool lampRelayActive;
    GsmNetworkStatus gsmStatus;
    bool smsAlertDispatched;
};

#endif // SYSTEM_TYPES_H
