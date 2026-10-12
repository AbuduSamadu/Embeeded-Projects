#pragma once

#include <Arduino.h>

/**
 * @brief Abstract interface for communication channels (Serial, BLE, WiFi, CAN, etc.)
 */
class ICommunicationChannel {
public:
    virtual ~ICommunicationChannel() = default;

    virtual bool init() = 0;
    virtual bool hasMessage() = 0;
    virtual String readMessage() = 0;
    virtual void sendResponse(const String& response) = 0;
};
