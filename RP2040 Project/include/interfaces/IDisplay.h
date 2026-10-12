#pragma once

#include <Arduino.h>

/**
 * @brief Abstract interface for display devices (Dependency Inversion Principle)
 * Business logic communicates with displays purely through this contract.
 */
class IDisplay {
public:
    virtual ~IDisplay() = default;

    virtual bool init() = 0;
    virtual void clear() = 0;
    virtual void setHeader(const String& title) = 0;
    virtual void showMessage(const String& label, const String& message) = 0;
    virtual void updateStatus(uint32_t uptimeSec, uint32_t messageCount) = 0;
};
