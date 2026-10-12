#pragma once

#include "interfaces/IDisplay.h"
#include "interfaces/ICommunicationChannel.h"

/**
 * @brief Core Application Business Logic
 * Follows SRP, DIP, and Clean Architecture.
 * Has NO knowledge of TFT_eSPI or specific hardware pinout.
 */
class Application {
public:
    Application(IDisplay& display, ICommunicationChannel& commChannel);

    void initialize();
    void tick();

private:
    void handleIncomingMessages();
    void handlePeriodicTasks();

    IDisplay& _display;
    ICommunicationChannel& _comm;

    uint32_t _uptimeSec;
    uint32_t _messageCount;
    unsigned long _lastHeartbeatMs;
    String _activeMessage;
};
