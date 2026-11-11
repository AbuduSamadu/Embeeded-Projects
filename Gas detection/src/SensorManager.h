#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include "Config.h"
#include "SystemTypes.h"

class SensorManager {
public:
    SensorManager(uint8_t analogPin = PIN_MQ2_AO);

    void begin();
    void update();

    bool isWarmingUp() const;
    uint32_t getWarmupRemainingSeconds() const;
    bool isAlarmActive() const;
    SensorReading getLatestReading() const;

    // Manual / Testing overrides
    void setSimulatedReading(int adcValue);
    void clearSimulatedReading();

private:
    uint8_t       _analogPin;
    uint32_t      _warmupStartTime;
    bool          _warmupComplete;
    uint32_t      _lastSampleTime;

    float         _filteredAdc;
    uint16_t      _rawAdc;
    uint8_t       _consecutiveAlarmSamples;
    bool          _alarmTriggered;

    uint32_t      _recoveryStartTime;
    bool          _recoveringCandidate;

    int           _simulatedAdc; // -1 if disabled

    void sampleHardware();
    void evaluateAlarmState();
};

#endif // SENSOR_MANAGER_H
