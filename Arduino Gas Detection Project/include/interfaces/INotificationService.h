/**
 * @file INotificationService.h
 * @brief Abstract interface for remote notification systems (GSM SIM800L, SMS).
 */

#ifndef I_NOTIFICATION_SERVICE_H
#define I_NOTIFICATION_SERVICE_H

#include "SystemTypes.h"

class INotificationService {
public:
    virtual ~INotificationService() = default;

    /// Initialize communication with notification hardware
    virtual void init() = 0;

    /// Non-blocking driver state machine execution
    virtual void update() = 0;

    /// Request dispatch of an emergency alarm notification
    virtual bool sendAlert() = 0;

    /// Request dispatch of an alarm recovery/cleared notification
    virtual bool sendRecovery() = 0;

    /// Check if the service is currently executing an asynchronous transmission
    virtual bool isBusy() const = 0;

    /// Get current network registration status
    virtual GsmNetworkStatus getNetworkStatus() const = 0;

    /// Reset SMS cooldown timer (e.g., for bench testing)
    virtual void resetCooldown() {}
};

#endif // I_NOTIFICATION_SERVICE_H
