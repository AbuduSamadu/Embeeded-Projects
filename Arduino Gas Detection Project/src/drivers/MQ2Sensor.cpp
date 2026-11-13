/**
 * @file MQ2Sensor.cpp
 * @brief Implementation of the MQ-2 Sensor driver.
 */

#include "drivers/MQ2Sensor.h"

MQ2Sensor::MQ2Sensor(uint8_t analogPin,
                     uint16_t alarmThreshold,
                     uint16_t hysteresis,
                     unsigned long sampleIntervalMs)
    : _analogPin(analogPin),
      _alarmThreshold(alarmThreshold),
      _hysteresis(hysteresis),
      _sampleIntervalMs(sampleIntervalMs),
      _lastRawValue(0),
      _filteredValue(0),
      _sampleIndex(0),
      _sampleCount(0),
      _lastSampleTime(0),
      _isSimulated(false),
      _simulatedValue(0) {
    for (uint8_t i = 0; i < SystemConfig::SENSOR_FILTER_WINDOW_SIZE; ++i) {
        _samples[i] = 0;
    }
}

void MQ2Sensor::init() {
    pinMode(_analogPin, INPUT);
    // Take an initial sample to seed filter
    sample();
    _filteredValue = _lastRawValue;
    for (uint8_t i = 0; i < SystemConfig::SENSOR_FILTER_WINDOW_SIZE; ++i) {
        _samples[i] = _lastRawValue;
    }
    _sampleCount = SystemConfig::SENSOR_FILTER_WINDOW_SIZE;
}

void MQ2Sensor::update() {
    unsigned long now = millis();
    if (now - _lastSampleTime >= _sampleIntervalMs) {
        _lastSampleTime = now;
        sample();
    }
}

void MQ2Sensor::sample() {
    if (_isSimulated) {
        _lastRawValue = _simulatedValue;
        _filteredValue = _simulatedValue;
        return;
    }

    _lastRawValue = analogRead(_analogPin);

    // Update rolling buffer
    _samples[_sampleIndex] = _lastRawValue;
    _sampleIndex = (_sampleIndex + 1) % SystemConfig::SENSOR_FILTER_WINDOW_SIZE;

    if (_sampleCount < SystemConfig::SENSOR_FILTER_WINDOW_SIZE) {
        _sampleCount++;
    }

    // Compute moving average
    uint32_t sum = 0;
    for (uint8_t i = 0; i < _sampleCount; ++i) {
        sum += _samples[i];
    }
    _filteredValue = static_cast<uint16_t>(sum / _sampleCount);
}

uint16_t MQ2Sensor::getRawValue() const {
    return _lastRawValue;
}

uint16_t MQ2Sensor::getFilteredValue() const {
    return _filteredValue;
}

bool MQ2Sensor::isThresholdExceeded() const {
    return _filteredValue >= _alarmThreshold;
}

bool MQ2Sensor::isRecoveryLevel() const {
    if (_filteredValue <= (_alarmThreshold - _hysteresis)) {
        return true;
    }
    return false;
}

void MQ2Sensor::setThreshold(uint16_t threshold, uint16_t hysteresis) {
    _alarmThreshold = threshold;
    _hysteresis = hysteresis;
}

void MQ2Sensor::setSimulatedValue(uint16_t value) {
    _isSimulated = true;
    _simulatedValue = value;
    _lastRawValue = value;
    _filteredValue = value;

    // Fill filter buffer so moving average immediately reflects the simulated value
    for (uint8_t i = 0; i < SystemConfig::SENSOR_FILTER_WINDOW_SIZE; ++i) {
        _samples[i] = value;
    }
}

void MQ2Sensor::clearSimulation() {
    _isSimulated = false;
}

bool MQ2Sensor::isSimulated() const {
    return _isSimulated;
}
