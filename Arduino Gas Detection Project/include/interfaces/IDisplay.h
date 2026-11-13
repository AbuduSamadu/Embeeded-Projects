/**
 * @file IDisplay.h
 * @brief Abstract interface for visual output devices (I2C LCD).
 */

#ifndef I_DISPLAY_H
#define I_DISPLAY_H

#include "SystemTypes.h"

class IDisplay {
public:
    virtual ~IDisplay() = default;

    /// Initialize the display hardware
    virtual void init() = 0;

    /// Periodic refresh with the latest system telemetry
    virtual void update(const SystemTelemetry& telemetry) = 0;

    /// Display custom two-line status message
    virtual void showMessage(const char* line1, const char* line2) = 0;

    /// Clear display buffer
    virtual void clear() = 0;
};

#endif // I_DISPLAY_H
