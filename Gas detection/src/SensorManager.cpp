#include "SensorManager.h"

SensorManager::SensorManager(uint8_t analogPin)
    : _analogPin(analogPin),
      _warmupStartTime(0),
      _warmupComplete(false),
      _lastSampleTime(0),
      _filteredAdc(0.0f),
      _rawAdc(0),
      _consecutiveAlarmSamples(0),
      _alarmTriggered(false),
      _recoveryStartTime(0),
      _recoveringCandidate(false),
      _simulatedAdc(-1) {}

void SensorManager::begin() {
    _warmupStartTime = millis();
    _warmupComplete = false;
    _alarmTriggered = false;
    _consecutiveAlarmSamples = 0;
    _recoveringCandidate = false;

    // Seed the initial ADC reading
    _rawAdc = analogRead(_analogPin);
    _filteredAdc = static_cast<float>(_rawAdc);
    _lastSampleTime = millis();
}

bool SensorManager::isWarmingUp() const {
    if (_warmupComplete) return false;
    return (millis() - _warmupStartTime) < SENSOR_WARMUP_MS;
}

uint32_t SensorManager::getWarmupRemainingSeconds() const {
    if (isWarmingUp()) {
        uint32_t elapsed = millis() - _warmupStartTime;
        if (elapsed < SENSOR_WARMUP_MS) {
            return (SENSOR_WARMUP_MS - elapsed + 999UL) / 1000UL;
        }
    }
    return 0;
}

bool SensorManager::isAlarmActive() const {
    return _alarmTriggered;
}

void SensorManager::setSimulatedReading(int adcValue) {
    _simulatedAdc = constrain(adcValue, 0, 1023);
}

void SensorManager::clearSimulatedReading() {
    _simulatedAdc = -1;
}

void SensorManager::sampleHardware() {
    if (_simulatedAdc >= 0) {
        _rawAdc = static_cast<uint16_t>(_simulatedAdc);
    } else {
        _rawAdc = analogRead(_analogPin);
    }

    // Apply exponential moving average filter
    _filteredAdc = (SENSOR_FILTER_ALPHA * _rawAdc) + ((1.0f - SENSOR_FILTER_ALPHA) * _filteredAdc);
}

void SensorManager::evaluateAlarmState() {
    // If still in sensor warm-up phase, suppress alarm decisions
    if (isWarmingUp()) {
        _alarmTriggered = false;
        _consecutiveAlarmSamples = 0;
        _recoveringCandidate = false;
        return;
    } else {
        _warmupComplete = true;
    }

    uint16_t currentReading = static_cast<uint16_t>(_filteredAdc + 0.5f);
    const uint16_t recoveryThreshold = (GAS_ALARM_THRESHOLD_ADC > GAS_HYSTERESIS_ADC) 
                                       ? (GAS_ALARM_THRESHOLD_ADC - GAS_HYSTERESIS_ADC) 
                                       : 0;

    if (!_alarmTriggered) {
        // Normal monitoring mode: check for persistent threshold crossing
        if (currentReading >= GAS_ALARM_THRESHOLD_ADC) {
            _consecutiveAlarmSamples++;
            if (_consecutiveAlarmSamples >= ALARM_PERSIST_SAMPLES) {
                _alarmTriggered = true;
                _consecutiveAlarmSamples = 0;
                _recoveringCandidate = false;
            }
        } else {
            if (_consecutiveAlarmSamples > 0) {
                _consecutiveAlarmSamples--;
            }
        }
    } else {
        // Alarm is active: check for stable recovery below hysteresis threshold
        if (currentReading < recoveryThreshold) {
            if (!_recoveringCandidate) {
                _recoveringCandidate = true;
                _recoveryStartTime = millis();
            } else {
                if (millis() - _recoveryStartTime >= RECOVERY_HOLD_MS) {
                    // Stable recovery confirmed!
                    _alarmTriggered = false;
                    _recoveringCandidate = false;
                    _consecutiveAlarmSamples = 0;
                }
            }
        } else {
            // Reading spiked back above recovery threshold, reset recovery candidate
            _recoveringCandidate = false;
        }
    }
}

void SensorManager::update() {
    uint32_t now = millis();
    if (now - _lastSampleTime >= INTERVAL_SENSOR_SAMPLE_MS) {
        _lastSampleTime = now;
        sampleHardware();
        evaluateAlarmState();
    }
}

SensorReading SensorManager::getLatestReading() const {
    SensorReading reading;
    reading.rawAdc = _rawAdc;
    reading.filteredAdc = static_cast<uint16_t>(_filteredAdc + 0.5f);
    // NodeMCU ADC accepts up to 3.3V (due to internal 220k/100k divider on A0)
    reading.equivalentVolts = (reading.filteredAdc / 1023.0f) * 3.3f;
    reading.alarmCondition = _alarmTriggered;
    reading.warmupRemainingSec = getWarmupRemainingSeconds();
    return reading;
}
