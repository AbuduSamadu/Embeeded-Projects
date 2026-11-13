/**
 * @file Relay.h
 * @brief Concrete driver for the Relay Module controlling the demonstration lamp.
 * 
 * Implements IActuator.
 * Handles active-high / active-low logic polarity and fail-safe power-on initialization.
 */

#ifndef RELAY_H
#define RELAY_H

#include "interfaces/IActuator.h"
#include "Config.h"

class Relay : public IActuator {
public:
    explicit Relay(uint8_t pin, bool activeLow = SystemConfig::RELAY_ACTIVE_LOW);

    void init() override;
    void activate() override;    ///< Turn relay ON (energized)
    void deactivate() override;  ///< Turn relay OFF (de-energized, safe state)
    void update() override;      ///< No-op for static relay
    bool isActive() const override;

private:
    const uint8_t _pin;
    const bool _activeLow;
    bool _isEnergized;

    void applyState();
};

#endif // RELAY_H
