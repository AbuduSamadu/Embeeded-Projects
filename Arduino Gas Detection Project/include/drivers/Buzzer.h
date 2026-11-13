/**
 * @file Buzzer.h
 * @brief Concrete driver for the audible alert buzzer.
 * 
 * Implements IActuator.
 * Provides non-blocking alert cadence pulsing via millis().
 */

#ifndef BUZZER_H
#define BUZZER_H

#include "interfaces/IActuator.h"
#include "Config.h"

class Buzzer : public IActuator {
public:
    explicit Buzzer(uint8_t pin,
                    bool activeHigh = SystemConfig::BUZZER_ACTIVE_HIGH,
                    unsigned long onTimeMs = SystemConfig::BUZZER_CADENCE_ON_MS,
                    unsigned long offTimeMs = SystemConfig::BUZZER_CADENCE_OFF_MS);

    void init() override;
    void activate() override;
    void deactivate() override;
    void update() override;
    bool isActive() const override;

    /// Optional one-shot beep for UI feedback (e.g., boot completion)
    void beepOnce(unsigned long durationMs = 100) override;

private:
    const uint8_t _pin;
    const bool _activeHigh;
    const unsigned long _onTimeMs;
    const unsigned long _offTimeMs;

    bool _isEnabled;       ///< Overall alarm state (pulsing active)
    bool _isPhysicalHigh;  ///< Current digital pin output state
    unsigned long _lastToggleTime;

    void writeOutput(bool active);
};

#endif // BUZZER_H
