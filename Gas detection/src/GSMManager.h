#ifndef GSM_MANAGER_H
#define GSM_MANAGER_H

#include <Arduino.h>
#include <SoftwareSerial.h>
#include "Config.h"
#include "SystemTypes.h"

class GSMManager {
public:
    GSMManager(uint8_t rxPin = PIN_GSM_RX, uint8_t txPin = PIN_GSM_TX);

    void begin();
    void update();

    bool isNetworkReady() const;
    GSMTelemetry getTelemetry() const;
    
    // SMS Operations
    bool sendAlertSMS(const char* recipientNumber = TARGET_PHONE_NUMBER);
    bool sendRecoverySMS(const char* recipientNumber = TARGET_PHONE_NUMBER);
    void resetAlertSentFlag();
    bool hasAlertSmsBeenSent() const;

private:
    SoftwareSerial   _serial;
    GSMNetworkStatus _networkStatus;
    int              _rssi;
    bool             _isInitialized;
    bool             _alertSmsSent;
    uint32_t         _lastAlertSmsTime;
    uint32_t         _lastNetworkCheckTime;
    bool             _lastSmsSuccess;

    // Helper AT command execution
    bool sendATCommand(const char* cmd, const char* expectedResponse, uint32_t timeoutMs = 2000);
    String sendATCommandReadResponse(const char* cmd, uint32_t timeoutMs = 2000);
    void queryNetworkRegistration();
    void querySignalQuality();
    bool configureTextMessageMode();
    bool dispatchRawSMS(const char* recipientNumber, const char* messageText);
};

#endif // GSM_MANAGER_H
