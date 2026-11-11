#ifndef SYSTEM_TYPES_H
#define SYSTEM_TYPES_H

#include <Arduino.h>

// Overall System Operation States
enum class SystemState {
    INITIALIZING,
    WARMUP,
    NORMAL,
    ALARM,
    RECOVERY
};

// Sensor Operational Status
enum class SensorHealth {
    WARMING_UP,
    CLEAN_NORMAL,
    EVALUATING_SPIKE,
    ALARM_ACTIVE,
    RECOVERING
};

// SIM800L Cellular Registration Status
enum class GSMNetworkStatus {
    NOT_INITIALIZED,
    UNKNOWN,
    CHECKING,
    NOT_REGISTERED,
    REGISTERED_HOME,
    SEARCHING,
    REGISTRATION_DENIED,
    REGISTERED_ROAMING,
    COMMUNICATION_ERROR
};

// Processed Sensor Data Struct
struct SensorReading {
    uint16_t rawAdc;
    uint16_t filteredAdc;
    float    equivalentVolts;
    bool     alarmCondition;
    uint32_t warmupRemainingSec;
};

// GSM Telemetry and Status Struct
struct GSMTelemetry {
    GSMNetworkStatus networkStatus;
    int              signalQualityRssi; // 0 - 31, or 99 if unknown
    bool             isReadyForSMS;
    bool             lastSmsSentSuccess;
};

#endif // SYSTEM_TYPES_H
