/**
 * @file LcdDisplay.cpp
 * @brief Implementation of the I2C LCD Display driver.
 */

#include "drivers/LcdDisplay.h"
#include <string.h>
#include <stdio.h>

LcdDisplay::LcdDisplay(uint8_t address, uint8_t cols, uint8_t rows)
    : _lcd(address, cols, rows),
      _cols(cols),
      _rows(rows),
      _lastRefreshTime(0),
      _alarmScreenToggle(0) {
    memset(_currentLine0, 0, sizeof(_currentLine0));
    memset(_currentLine1, 0, sizeof(_currentLine1));
}

void LcdDisplay::init() {
    _lcd.init();
    _lcd.backlight();
    _lcd.clear();
    showMessage("SYSTEM INIT...", "PLEASE WAIT...");
}

void LcdDisplay::clear() {
    _lcd.clear();
    memset(_currentLine0, 0, sizeof(_currentLine0));
    memset(_currentLine1, 0, sizeof(_currentLine1));
}

void LcdDisplay::showMessage(const char* line1, const char* line2) {
    printLinePadded(0, line1);
    printLinePadded(1, line2);
}

const char* LcdDisplay::gsmStatusShortString(GsmNetworkStatus status) {
    switch (status) {
        case GsmNetworkStatus::REGISTERED_HOME:   return "GSM:OK";
        case GsmNetworkStatus::REGISTERED_ROAMING:return "GSM:ROAM";
        case GsmNetworkStatus::SEARCHING:         return "GSM:SRCH";
        case GsmNetworkStatus::NOT_REGISTERED:    return "GSM:NO";
        case GsmNetworkStatus::REGISTRATION_DENIED:return "GSM:DENY";
        default:                                  return "GSM:--";
    }
}

void LcdDisplay::update(const SystemTelemetry& telemetry) {
    unsigned long now = millis();
    if (now - _lastRefreshTime < SystemConfig::LCD_REFRESH_INTERVAL_MS) {
        return;
    }
    _lastRefreshTime = now;

    char line0[17];
    char line1[17];

    switch (telemetry.systemState) {
        case SystemState::INITIALIZING: {
            snprintf_P(line0, sizeof(line0), PSTR("%-16s"), "SYSTEM INIT...");
            snprintf_P(line1, sizeof(line1), PSTR("%-16s"), "PLEASE WAIT...");
            break;
        }

        case SystemState::WARMUP: {
            snprintf_P(line0, sizeof(line0), PSTR("%-16s"), "SENSOR WARM-UP");
            snprintf_P(line1, sizeof(line1), PSTR("WAIT: %2lus RAW:%-4u"),
                     telemetry.warmupRemainingSeconds,
                     telemetry.rawSensorValue);
            break;
        }

        case SystemState::NORMAL: {
            snprintf_P(line0, sizeof(line0), PSTR("GAS MON  %s"), gsmStatusShortString(telemetry.gsmStatus));
            snprintf_P(line1, sizeof(line1), PSTR("AIR:OK  VAL:%-4u"), telemetry.filteredSensorValue);
            break;
        }

        case SystemState::ALARM: {
            _alarmScreenToggle = (_alarmScreenToggle + 1) % 4;
            if (_alarmScreenToggle < 2) {
                snprintf_P(line0, sizeof(line0), PSTR("%-16s"), "WARNING DETECTED");
                snprintf_P(line1, sizeof(line1), PSTR("ALARM! VAL:%-4u"), telemetry.filteredSensorValue);
            } else {
                snprintf_P(line0, sizeof(line0), PSTR("%-16s"), "SMOKE/GAS LEAK!");
                if (telemetry.smsAlertDispatched) {
                    snprintf_P(line1, sizeof(line1), PSTR("%-16s"), "SMS DISPATCHED");
                } else {
                    snprintf_P(line1, sizeof(line1), PSTR("%-16s"), "SENDING SMS...");
                }
            }
            break;
        }

        case SystemState::RECOVERY: {
            snprintf_P(line0, sizeof(line0), PSTR("%-16s"), "ALARM RECOVERY");
            snprintf_P(line1, sizeof(line1), PSTR("CLEARING VAL:%-4u"), telemetry.filteredSensorValue);
            break;
        }
    }

    printLinePadded(0, line0);
    printLinePadded(1, line1);
}

void LcdDisplay::printLinePadded(uint8_t row, const char* text) {
    char formatted[17];
    // Copy and space pad up to 16 characters
    uint8_t len = 0;
    while (text[len] != '\0' && len < _cols) {
        formatted[len] = text[len];
        len++;
    }
    while (len < _cols) {
        formatted[len] = ' ';
        len++;
    }
    formatted[_cols] = '\0';

    // Only update LCD if content actually changed to avoid I2C bus traffic
    char* currentCache = (row == 0) ? _currentLine0 : _currentLine1;
    if (strcmp(currentCache, formatted) != 0) {
        strncpy(currentCache, formatted, sizeof(_currentLine0));
        _lcd.setCursor(0, row);
        _lcd.print(formatted);
    }
}
