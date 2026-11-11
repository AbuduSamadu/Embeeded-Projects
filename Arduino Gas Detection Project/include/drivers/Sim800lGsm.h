/**
 * @file Sim800lGsm.h
 * @brief Concrete driver for the SIM800L GSM/GPRS Module.
 * 
 * Implements INotificationService.
 * Features a 100% non-blocking finite state machine for AT commands,
 * network registration monitoring, SMS alert queuing, and anti-spam rate limiting.
 */

#ifndef SIM800L_GSM_H
#define SIM800L_GSM_H

#include <SoftwareSerial.h>
#include "interfaces/INotificationService.h"
#include "Config.h"

class Sim800lGsm : public INotificationService {
public:
    explicit Sim800lGsm(uint8_t rxPin,
                        uint8_t txPin,
                        const char* targetPhoneNumber = SystemConfig::EMERGENCY_PHONE_NUMBER,
                        unsigned long baudRate = SystemConfig::GSM_BAUD_RATE);

    void init() override;
    void update() override;

    bool sendAlert() override;
    bool sendRecovery() override;

    bool isBusy() const override;
    GsmNetworkStatus getNetworkStatus() const override;
    bool isSmsDispatched() const;

private:
    SoftwareSerial _serial;
    const char* _phoneNumber;
    const unsigned long _baudRate;

    GsmState _state;
    GsmNetworkStatus _networkStatus;

    // Buffer for reading AT responses
    static constexpr uint8_t RX_BUFFER_SIZE = 80;
    char _rxBuffer[RX_BUFFER_SIZE];
    uint8_t _rxIndex;

    // SMS dispatch queue tracking
    enum class PendingSms : uint8_t { NONE, ALERT, RECOVERY };
    PendingSms _pendingSms;

    bool _smsDispatched;
    unsigned long _lastSmsTime;
    unsigned long _stateStartTime;
    unsigned long _lastNetworkCheckTime;
    uint8_t _retryCount;

    void processIncomingBytes();
    void transitionTo(GsmState nextState);
    bool checkBufferContains(const char* needle) const;
    void clearBuffer();
    void sendCommand(const char* cmd);
    void sendCommand(const __FlashStringHelper* cmd);
    void parseNetworkRegistration();
};

#endif // SIM800L_GSM_H
