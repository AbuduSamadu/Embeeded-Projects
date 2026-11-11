#include "GSMManager.h"

GSMManager::GSMManager(uint8_t rxPin, uint8_t txPin)
    : _serial(rxPin, txPin),
      _networkStatus(GSMNetworkStatus::NOT_INITIALIZED),
      _rssi(99),
      _isInitialized(false),
      _alertSmsSent(false),
      _lastAlertSmsTime(0),
      _lastNetworkCheckTime(0),
      _lastSmsSuccess(false) {}

void GSMManager::begin() {
    _serial.begin(GSM_BAUD_RATE);
    delay(100);

    Serial.println(F("[GSM] Initializing SIM800L serial interface..."));

    // Check basic communication
    if (sendATCommand("AT", "OK", 1500)) {
        Serial.println(F("[GSM] SIM800L responded to AT command."));
        // Turn off echo for clean parsing
        sendATCommand("ATE0", "OK", 1000);
        // Verify PIN is ready
        if (sendATCommand("AT+CPIN?", "READY", 2000)) {
            Serial.println(F("[GSM] SIM Card is detected and READY."));
            configureTextMessageMode();
            queryNetworkRegistration();
            querySignalQuality();
            _isInitialized = true;
        } else {
            Serial.println(F("[GSM] Warning: SIM Card not ready or PIN locked."));
            _networkStatus = GSMNetworkStatus::REGISTRATION_DENIED;
        }
    } else {
        Serial.println(F("[GSM] Error: SIM800L not responding. Check power & connections."));
        _networkStatus = GSMNetworkStatus::COMMUNICATION_ERROR;
    }
}

bool GSMManager::sendATCommand(const char* cmd, const char* expectedResponse, uint32_t timeoutMs) {
    while (_serial.available()) {
        _serial.read();
    }

    _serial.println(cmd);
    uint32_t start = millis();
    String response = "";

    while (millis() - start < timeoutMs) {
        while (_serial.available()) {
            char c = static_cast<char>(_serial.read());
            response += c;
            if (response.indexOf(expectedResponse) != -1) {
                return true;
            }
        }
        yield();
    }
    return false;
}

String GSMManager::sendATCommandReadResponse(const char* cmd, uint32_t timeoutMs) {
    while (_serial.available()) {
        _serial.read();
    }

    _serial.println(cmd);
    uint32_t start = millis();
    String response = "";

    while (millis() - start < timeoutMs) {
        while (_serial.available()) {
            char c = static_cast<char>(_serial.read());
            response += c;
        }
        if (response.indexOf("OK") != -1 || response.indexOf("ERROR") != -1) {
            break;
        }
        yield();
    }
    return response;
}

bool GSMManager::configureTextMessageMode() {
    // AT+CMGF=1: SMS text mode
    if (sendATCommand("AT+CMGF=1", "OK", 1500)) {
        // AT+CSCS="GSM": Standard GSM 7-bit character set
        sendATCommand("AT+CSCS=\"GSM\"", "OK", 1000);
        return true;
    }
    return false;
}

void GSMManager::queryNetworkRegistration() {
    String resp = sendATCommandReadResponse("AT+CREG?", 2000);
    int idx = resp.indexOf("+CREG:");
    if (idx != -1) {
        int commaIdx = resp.indexOf(',', idx);
        if (commaIdx != -1 && commaIdx + 1 < static_cast<int>(resp.length())) {
            char stat = resp.charAt(commaIdx + 1);
            switch (stat) {
                case '0': _networkStatus = GSMNetworkStatus::NOT_REGISTERED; break;
                case '1': _networkStatus = GSMNetworkStatus::REGISTERED_HOME; break;
                case '2': _networkStatus = GSMNetworkStatus::SEARCHING; break;
                case '3': _networkStatus = GSMNetworkStatus::REGISTRATION_DENIED; break;
                case '4': _networkStatus = GSMNetworkStatus::UNKNOWN; break;
                case '5': _networkStatus = GSMNetworkStatus::REGISTERED_ROAMING; break;
                default:  _networkStatus = GSMNetworkStatus::UNKNOWN; break;
            }
            return;
        }
    }
    if (resp.length() == 0) {
        _networkStatus = GSMNetworkStatus::COMMUNICATION_ERROR;
    }
}

void GSMManager::querySignalQuality() {
    String resp = sendATCommandReadResponse("AT+CSQ", 1500);
    int idx = resp.indexOf("+CSQ:");
    if (idx != -1) {
        int commaIdx = resp.indexOf(',', idx);
        if (commaIdx != -1) {
            String rssiStr = resp.substring(idx + 5, commaIdx);
            rssiStr.trim();
            _rssi = rssiStr.toInt();
            return;
        }
    }
    _rssi = 99; // 99 means unknown / undetectable
}

void GSMManager::update() {
    uint32_t now = millis();
    if (now - _lastNetworkCheckTime >= INTERVAL_GSM_CHECK_MS) {
        _lastNetworkCheckTime = now;
        queryNetworkRegistration();
        querySignalQuality();
    }
}

bool GSMManager::isNetworkReady() const {
    return (_networkStatus == GSMNetworkStatus::REGISTERED_HOME || 
            _networkStatus == GSMNetworkStatus::REGISTERED_ROAMING);
}

GSMTelemetry GSMManager::getTelemetry() const {
    GSMTelemetry t;
    t.networkStatus = _networkStatus;
    t.signalQualityRssi = _rssi;
    t.isReadyForSMS = isNetworkReady();
    t.lastSmsSentSuccess = _lastSmsSuccess;
    return t;
}

bool GSMManager::dispatchRawSMS(const char* recipientNumber, const char* messageText) {
    Serial.printf("[GSM] Preparing SMS to: %s\n", recipientNumber);

    // Make sure we are in SMS text mode
    configureTextMessageMode();

    while (_serial.available()) {
        _serial.read();
    }

    _serial.print(F("AT+CMGS=\""));
    _serial.print(recipientNumber);
    _serial.println(F("\""));

    // Wait for the prompt '>'
    uint32_t start = millis();
    bool promptReceived = false;
    while (millis() - start < 3000) {
        while (_serial.available()) {
            char c = static_cast<char>(_serial.read());
            if (c == '>') {
                promptReceived = true;
                break;
            }
        }
        if (promptReceived) break;
        yield();
    }

    if (!promptReceived) {
        Serial.println(F("[GSM] SMS prompt '>' not received. Aborting."));
        // Send ESC (0x1B) to cancel prompt if stuck
        _serial.write(0x1B);
        _lastSmsSuccess = false;
        return false;
    }

    // Send the message text followed by Ctrl+Z (0x1A)
    _serial.print(messageText);
    _serial.write(0x1A);

    // Wait for +CMGS: confirmation and OK (up to 15 seconds)
    start = millis();
    String response = "";
    bool success = false;
    while (millis() - start < 15000) {
        while (_serial.available()) {
            char c = static_cast<char>(_serial.read());
            response += c;
            if (response.indexOf("+CMGS:") != -1 && response.indexOf("OK") != -1) {
                success = true;
                break;
            }
            if (response.indexOf("ERROR") != -1) {
                success = false;
                break;
            }
        }
        if (success || response.indexOf("ERROR") != -1) break;
        yield();
    }

    _lastSmsSuccess = success;
    if (success) {
        Serial.println(F("[GSM] SMS successfully delivered to network!"));
    } else {
        Serial.println(F("[GSM] SMS sending failed or timed out."));
    }
    return success;
}

bool GSMManager::sendAlertSMS(const char* recipientNumber) {
    uint32_t now = millis();

    // Check if alert SMS was already sent for the active episode,
    // and respect rate-limiting cooldown
    if (_alertSmsSent && (now - _lastAlertSmsTime < SMS_ALERT_COOLDOWN_MS)) {
        return false;
    }

    bool success = dispatchRawSMS(recipientNumber, SMS_ALERT_MESSAGE);
    if (success) {
        _alertSmsSent = true;
        _lastAlertSmsTime = now;
    }
    return success;
}

bool GSMManager::sendRecoverySMS(const char* recipientNumber) {
    if (!SEND_RECOVERY_SMS) return false;
    if (!_alertSmsSent) return false; // Only send recovery if an alert SMS was previously sent

    bool success = dispatchRawSMS(recipientNumber, SMS_RECOVERY_MESSAGE);
    resetAlertSentFlag();
    return success;
}

void GSMManager::resetAlertSentFlag() {
    _alertSmsSent = false;
}

bool GSMManager::hasAlertSmsBeenSent() const {
    return _alertSmsSent;
}
