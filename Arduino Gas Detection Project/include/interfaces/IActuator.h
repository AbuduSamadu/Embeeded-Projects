/**
 * @file IActuator.h
 * @brief Abstract interface for output devices and actuators (buzzer, relay).
 */

#ifndef I_ACTUATOR_H
#define I_ACTUATOR_H

#include <Arduino.h>

class IActuator {
public:
    virtual ~IActuator() = default;

    /// Initialize the actuator to its safe default power-on state
    virtual void init() = 0;

    /// Command the actuator to activate
    virtual void activate() = 0;

    /// Command the actuator to deactivate (safe state)
    virtual void deactivate() = 0;

    /// Periodic non-blocking processing (e.g. cadence, pulsing, timeouts)
    virtual void update() = 0;

    /// Returns true if the actuator is currently active / operating
    virtual bool isActive() const = 0;

    /// Optional one-shot activation for UI feedback or hardware test
    virtual void beepOnce(unsigned long durationMs = 100) { (void)durationMs; }
};

#endif // I_ACTUATOR_H
