/**
 * @file MQ2Sensor.h
 * @brief Concrete driver for the MQ-2 Combustible Gas & Smoke Sensor.
 * 
 * Implements ISensor interface.
 * Features rolling moving-average filtering and hysteresis comparison.
 */

#ifndef MQ2_SENSOR_H
#define MQ2_SENSOR_H

#include "interfaces/ISensor.h"
#include "Config.h"

class MQ2Sensor : public ISensor {
public:
    explicit MQ2Sensor(uint8_t analogPin,
                       uint16_t alarmThreshold = SystemConfig::GAS_ALARM_THRESHOLD,
                       uint16_t hysteresis = SystemConfig::GAS_ALARM_HYSTERESIS,
                       unsigned long sampleIntervalMs = SystemConfig::SENSOR_SAMPLE_INTERVAL_MS);

    void init() override;
    void update() override;

    uint16_t getRawValue() const override;
    uint16_t getFilteredValue() const override;

    bool isThresholdExceeded() const override;
    bool isRecoveryLevel() const override;

    void setSimulatedValue(uint16_t value) override;
    void clearSimulation() override;
    bool isSimulated() const override;

    /// Dynamic threshold configuration (for runtime calibration)
    void setThreshold(uint16_t threshold, uint16_t hysteresis);

private:
    const uint8_t _analogPin;
    uint16_t _alarmThreshold;
    uint16_t _hysteresis;
    const unsigned long _sampleIntervalMs;

    uint16_t _lastRawValue;
    uint16_t _filteredValue;

    // Rolling circular buffer for noise filtering
    uint16_t _samples[SystemConfig::SENSOR_FILTER_WINDOW_SIZE];
    uint8_t _sampleIndex;
    uint8_t _sampleCount;
    unsigned long _lastSampleTime;

    bool _isSimulated;
    uint16_t _simulatedValue;

    void sample();
};

#endif // MQ2_SENSOR_H
