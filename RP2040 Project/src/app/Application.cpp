#include "app/Application.h"
#include "config/AppConfig.h"

Application::Application(IDisplay& display, ICommunicationChannel& commChannel)
    : _display(display),
      _comm(commChannel),
      _uptimeSec(0),
      _messageCount(0),
      _lastHeartbeatMs(0),
      _activeMessage("Ready") {}

void Application::initialize() {
    _comm.init();
    _display.init();

    _display.setHeader("RP2040 Core");
    _display.showMessage("System Status", _activeMessage);
    _display.updateStatus(_uptimeSec, _messageCount);

    _comm.sendResponse("[APP]: System initialized successfully.");
}

void Application::tick() {
    handleIncomingMessages();
    handlePeriodicTasks();
}

void Application::handleIncomingMessages() {
    if (!_comm.hasMessage()) {
        return;
    }

    String incoming = _comm.readMessage();
    if (incoming.length() == 0) {
        return;
    }

    _activeMessage = incoming;
    _messageCount++;

    // Update presentation layer via abstraction
    _display.showMessage("Last Message", _activeMessage);
    _display.updateStatus(_uptimeSec, _messageCount);

    // Reply confirmation
    _comm.sendResponse("[ACK] Received (" + String(_messageCount) + "): " + _activeMessage);
}

void Application::handlePeriodicTasks() {
    unsigned long now = millis();
    if (now - _lastHeartbeatMs >= Config::Timing::HEARTBEAT_INTERVAL_MS) {
        _lastHeartbeatMs = now;
        _uptimeSec++;
        _display.updateStatus(_uptimeSec, _messageCount);
    }
}
