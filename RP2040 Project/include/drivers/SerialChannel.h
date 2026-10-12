#pragma once

#include "interfaces/ICommunicationChannel.h"
#include "config/AppConfig.h"

/**
 * @brief Concrete Hardware Serial communication channel
 */
class SerialChannel : public ICommunicationChannel {
public:
    SerialChannel();

    bool init() override;
    bool hasMessage() override;
    String readMessage() override;
    void sendResponse(const String& response) override;
};
