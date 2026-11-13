/**
 * @file Sim800lGsm.cpp
 * @brief Non-blocking SIM800L driver implementation.
 */

#include "drivers/Sim800lGsm.h"
#include <string.h>

Sim800lGsm::Sim800lGsm(uint8_t rxPin, uint8_t txPin, const char* targetPhoneNumber, unsigned long baudRate)
    : _serial(rxPin, txPin),
      _phoneNumber(targetPhoneNumber),
      _baudRate(baudRate),
      _state(GsmState::IDLE),
      _networkStatus(GsmNetworkStatus::UNKNOWN),
      _rxIndex(0),
      _pendingSms(PendingSms::NONE),
      _smsDispatched(false),
      _lastSmsTime(0),
      _stateStartTime(0),
      _lastNetworkCheckTime(0),
      _retryCount(0) {
    memset(_rxBuffer, 0, sizeof(_rxBuffer));
}

void Sim800lGsm::init() {
    _serial.begin(_baudRate);
    clearBuffer();
    _state = GsmState::CHECK_AT;
    _stateStartTime = millis();
    _retryCount = 0;
    _smsDispatched = false;
    Serial.println(F("[GSM] Initializing SIM800L module..."));
    sendCommand(F("AT\r\n"));
}

void Sim800lGsm::update() {
    processIncomingBytes();
    unsigned long now = millis();

    switch (_state) {
        case GsmState::CHECK_AT: {
            if (checkBufferContains("OK")) {
                Serial.println(F("[GSM] AT OK - Modem responded."));
                clearBuffer();
                transitionTo(GsmState::SET_ECHO_OFF);
                sendCommand(F("ATE0\r\n"));
            } else if (now - _stateStartTime > 2000UL) {
                _retryCount++;
                if (_retryCount > 5) {
                    Serial.println(F("[GSM] Warning: No response to AT. Will retry later."));
                    transitionTo(GsmState::IDLE);
                } else {
                    _stateStartTime = now;
                    clearBuffer();
                    sendCommand(F("AT\r\n"));
                }
            }
            break;
        }

        case GsmState::SET_ECHO_OFF: {
            if (checkBufferContains("OK") || (now - _stateStartTime > 2000UL)) {
                clearBuffer();
                transitionTo(GsmState::CHECK_PIN);
                sendCommand(F("AT+CPIN?\r\n"));
            }
            break;
        }

        case GsmState::CHECK_PIN: {
            if (checkBufferContains("READY")) {
                Serial.println(F("[GSM] SIM Card Ready."));
                clearBuffer();
                transitionTo(GsmState::SET_TEXT_MODE);
                sendCommand(F("AT+CMGF=1\r\n"));
            } else if (checkBufferContains("ERROR") || (now - _stateStartTime > 3000UL)) {
                Serial.println(F("[GSM] SIM Card check failed or not ready."));
                clearBuffer();
                transitionTo(GsmState::SET_TEXT_MODE);
                sendCommand(F("AT+CMGF=1\r\n"));
            }
            break;
        }

        case GsmState::SET_TEXT_MODE: {
            if (checkBufferContains("OK") || (now - _stateStartTime > 2000UL)) {
                Serial.println(F("[GSM] SMS Text mode configured."));
                clearBuffer();
                transitionTo(GsmState::CHECK_NETWORK);
                sendCommand(F("AT+CREG?\r\n"));
            }
            break;
        }

        case GsmState::CHECK_NETWORK: {
            if (checkBufferContains("+CREG:")) {
                parseNetworkRegistration();
                clearBuffer();
                _lastNetworkCheckTime = now;
                transitionTo(GsmState::IDLE);
            } else if (now - _stateStartTime > 3000UL) {
                _networkStatus = GsmNetworkStatus::UNKNOWN;
                clearBuffer();
                _lastNetworkCheckTime = now;
                transitionTo(GsmState::IDLE);
            }
            break;
        }

        case GsmState::IDLE: {
            // Check if there is an SMS waiting to be sent
            if (_pendingSms != PendingSms::NONE) {
                if (now - _lastSmsTime >= SystemConfig::SMS_ALERT_COOLDOWN_MS || _lastSmsTime == 0) {
                    clearBuffer();
                    transitionTo(GsmState::SEND_SMS_CMD);
                    _serial.print(F("AT+CMGS=\""));
                    _serial.print(_phoneNumber);
                    _serial.print(F("\"\r\n"));
                    Serial.print(F("[GSM] Initiating SMS to: "));
                    Serial.println(_phoneNumber);
                }
            } else {
                // Periodic network check every 30 seconds
                if (now - _lastNetworkCheckTime > 30000UL) {
                    _lastNetworkCheckTime = now;
                    clearBuffer();
                    transitionTo(GsmState::CHECK_NETWORK);
                    sendCommand(F("AT+CREG?\r\n"));
                }
            }
            break;
        }

        case GsmState::SEND_SMS_CMD: {
            if (checkBufferContains(">")) {
                // Prompt received; transmit body
                clearBuffer();
                transitionTo(GsmState::SEND_SMS_BODY);

                if (_pendingSms == PendingSms::ALERT) {
                    _serial.print(F("SMOKE AND GAS ALERT!\r\nAbnormal smoke/gas level detected by monitoring system.\r\nPlease investigate safely."));
                } else if (_pendingSms == PendingSms::RECOVERY) {
                    _serial.print(F("ALERT RESOLVED:\r\nGas readings returned to normal baseline.\r\nArea monitoring active."));
                }
                _serial.write(0x1A); // Ctrl+Z to submit
                _serial.print(F("\r\n"));

                Serial.println(F("[GSM] SMS payload dispatched, awaiting network confirmation..."));
                transitionTo(GsmState::WAIT_SMS_CONFIRM);
            } else if (checkBufferContains("ERROR") || (now - _stateStartTime > 7000UL)) {
                Serial.println(F("[GSM] Error or timeout awaiting '>' prompt. Aborting SMS."));
                _pendingSms = PendingSms::NONE;
                clearBuffer();
                transitionTo(GsmState::IDLE);
            }
            break;
        }

        case GsmState::SEND_SMS_BODY: {
            // Transitioned to WAIT_SMS_CONFIRM immediately after writing body
            break;
        }

        case GsmState::WAIT_SMS_CONFIRM: {
            if (checkBufferContains("+CMGS:") || checkBufferContains("OK")) {
                Serial.println(F("[GSM] SUCCESS: SMS delivered to network."));
                _smsDispatched = true;
                _lastSmsTime = now;
                _pendingSms = PendingSms::NONE;
                clearBuffer();
                transitionTo(GsmState::COOLDOWN);
            } else if (checkBufferContains("ERROR") || (now - _stateStartTime > 15000UL)) {
                Serial.println(F("[GSM] FAILURE: SMS transmission failed or timed out."));
                _pendingSms = PendingSms::NONE;
                clearBuffer();
                transitionTo(GsmState::IDLE);
            }
            break;
        }

        case GsmState::COOLDOWN: {
            if (now - _stateStartTime > 2000UL) {
                transitionTo(GsmState::IDLE);
            }
            break;
        }
    }
}

bool Sim800lGsm::sendAlert() {
    unsigned long now = millis();
    if (now - _lastSmsTime < SystemConfig::SMS_ALERT_COOLDOWN_MS && _lastSmsTime != 0) {
        Serial.println(F("[GSM] SMS Alert suppressed: Cooldown active."));
        return false;
    }
    _pendingSms = PendingSms::ALERT;
    _smsDispatched = false;
    Serial.println(F("[GSM] Alert SMS queued."));
    return true;
}

bool Sim800lGsm::sendRecovery() {
    if (!SystemConfig::ENABLE_RECOVERY_SMS) {
        return false;
    }
    _pendingSms = PendingSms::RECOVERY;
    Serial.println(F("[GSM] Recovery SMS queued."));
    return true;
}

bool Sim800lGsm::isBusy() const {
    return (_state == GsmState::SEND_SMS_CMD ||
            _state == GsmState::SEND_SMS_BODY ||
            _state == GsmState::WAIT_SMS_CONFIRM);
}

GsmNetworkStatus Sim800lGsm::getNetworkStatus() const {
    return _networkStatus;
}

bool Sim800lGsm::isSmsDispatched() const {
    return _smsDispatched;
}

void Sim800lGsm::resetCooldown() {
    _lastSmsTime = 0;
}

void Sim800lGsm::processIncomingBytes() {
    while (_serial.available() > 0) {
        char c = static_cast<char>(_serial.read());
        if (_rxIndex < RX_BUFFER_SIZE - 1) {
            _rxBuffer[_rxIndex++] = c;
            _rxBuffer[_rxIndex] = '\0';
        } else {
            // Buffer full: shift left by half to keep recent data
            memmove(_rxBuffer, _rxBuffer + (RX_BUFFER_SIZE / 2), RX_BUFFER_SIZE / 2);
            _rxIndex = RX_BUFFER_SIZE / 2;
            _rxBuffer[_rxIndex++] = c;
            _rxBuffer[_rxIndex] = '\0';
        }
    }
}

void Sim800lGsm::transitionTo(GsmState nextState) {
    _state = nextState;
    _stateStartTime = millis();
}

bool Sim800lGsm::checkBufferContains(const char* needle) const {
    return (strstr(_rxBuffer, needle) != nullptr);
}

void Sim800lGsm::clearBuffer() {
    memset(_rxBuffer, 0, sizeof(_rxBuffer));
    _rxIndex = 0;
}

void Sim800lGsm::sendCommand(const char* cmd) {
    _serial.print(cmd);
}

void Sim800lGsm::sendCommand(const __FlashStringHelper* cmd) {
    _serial.print(cmd);
}

void Sim800lGsm::parseNetworkRegistration() {
    // Looks for +CREG: <n>,<stat>
    const char* p = strstr(_rxBuffer, "+CREG:");
    if (p) {
        const char* comma = strchr(p, ',');
        if (comma && *(comma + 1) != '\0') {
            char statChar = *(comma + 1);
            switch (statChar) {
                case '1': _networkStatus = GsmNetworkStatus::REGISTERED_HOME; break;
                case '2': _networkStatus = GsmNetworkStatus::SEARCHING; break;
                case '3': _networkStatus = GsmNetworkStatus::REGISTRATION_DENIED; break;
                case '5': _networkStatus = GsmNetworkStatus::REGISTERED_ROAMING; break;
                default:  _networkStatus = GsmNetworkStatus::NOT_REGISTERED; break;
            }
            Serial.print(F("[GSM] Network status: "));
            Serial.println(statChar);
            return;
        }
    }
    _networkStatus = GsmNetworkStatus::UNKNOWN;
}
