/**
 * @file ISensor.h
 * @brief Abstract interface for environmental sensors.
 * 
 * Follows the Interface Segregation and Dependency Inversion Principles.
 */

#ifndef I_SENSOR_H
#define I_SENSOR_H

#include <Arduino.h>

class ISensor {
public:
    virtual ~ISensor() = default;

    /// Initialize sensor hardware pin configuration
    virtual void init() = 0;

    /// Periodic non-blocking sampling and filtering routine
    virtual void update() = 0;

    /// Get the latest raw ADC conversion reading
    virtual uint16_t getRawValue() const = 0;

    /// Get the filtered/smoothed reading (noise-reduced)
    virtual uint16_t getFilteredValue() const = 0;

    /// Returns true if reading has exceeded the trigger threshold
    virtual bool isThresholdExceeded() const = 0;

    /// Returns true if reading has dropped below recovery hysteresis threshold
    virtual bool isRecoveryLevel() const = 0;

    /// Set a simulated ADC reading for bench testing
    virtual void setSimulatedValue(uint16_t value) { (void)value; }

    /// Clear simulated reading and restore live physical readings
    virtual void clearSimulation() {}

    /// Check if the sensor is currently operating in simulation mode
    virtual bool isSimulated() const { return false; }
};

#endif // I_SENSOR_H
