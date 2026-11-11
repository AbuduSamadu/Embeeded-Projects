#include "DisplayManager.h"

DisplayManager::DisplayManager(uint8_t i2cAddr, uint8_t cols, uint8_t rows)
    : _lcd(i2cAddr, cols, rows),
      _cols(cols),
      _rows(rows),
      _lastUpdate(0),
      _pageToggle(0) {
    memset(_lineBuffer0, 0, sizeof(_lineBuffer0));
    memset(_lineBuffer1, 0, sizeof(_lineBuffer1));
}

void DisplayManager::begin() {
    // Explicitly configure ESP8266 I2C pins: SDA on D2 (GPIO4), SCL on D1 (GPIO5)
    Wire.begin(PIN_LCD_SDA, PIN_LCD_SCL);
    _lcd.init();
    _lcd.backlight();
    _lcd.clear();
}

void DisplayManager::writeLine(uint8_t row, const char* text) {
    if (row >= _rows) return;

    char formatted[21];
    snprintf(formatted, sizeof(formatted), "%-*.*s", _cols, _cols, text);

    _lcd.setCursor(0, row);
    _lcd.print(formatted);
}

const char* DisplayManager::gsmStatusToString(GSMNetworkStatus status) {
    switch (status) {
        case GSMNetworkStatus::REGISTERED_HOME:   return "ONLINE (HOME)";
        case GSMNetworkStatus::REGISTERED_ROAMING:return "ONLINE (ROAM)";
        case GSMNetworkStatus::SEARCHING:         return "SEARCHING...";
        case GSMNetworkStatus::NOT_REGISTERED:    return "NOT REG";
        case GSMNetworkStatus::REGISTRATION_DENIED:return "DENIED";
        case GSMNetworkStatus::COMMUNICATION_ERROR:return "COMM ERROR";
        default:                                  return "CHECKING...";
    }
}

void DisplayManager::showInitializing() {
    writeLine(0, "SYSTEM INIT...");
    writeLine(1, "PLEASE WAIT...");
}

void DisplayManager::update(SystemState state, const SensorReading& reading, const GSMTelemetry& gsm) {
    uint32_t now = millis();
    if (now - _lastUpdate < INTERVAL_DISPLAY_UPDATE_MS) {
        return;
    }
    _lastUpdate = now;
    _pageToggle = (_pageToggle + 1) % 4; // Cycles periodically for secondary info

    char line0[21];
    char line1[21];

    switch (state) {
        case SystemState::INITIALIZING:
            snprintf(line0, sizeof(line0), "SYSTEM STARTUP");
            snprintf(line1, sizeof(line1), "INITIALIZING...");
            break;

        case SystemState::WARMUP:
            snprintf(line0, sizeof(line0), "SENSOR WARM-UP");
            snprintf(line1, sizeof(line1), "READY IN: %us", reading.warmupRemainingSec);
            break;

        case SystemState::NORMAL:
            // Alternate display every 2 seconds between gas monitoring reading and GSM network status
            if (_pageToggle < 2) {
                snprintf(line0, sizeof(line0), "GAS MONITORING");
                snprintf(line1, sizeof(line1), "AIR:OK ADC:%u", reading.filteredAdc);
            } else {
                snprintf(line0, sizeof(line0), "GSM: %s", gsm.isReadyForSMS ? "READY" : "WAIT");
                snprintf(line1, sizeof(line1), "AIR STATUS: OK");
            }
            break;

        case SystemState::ALARM:
            // Warning display
            if (_pageToggle % 2 == 0) {
                snprintf(line0, sizeof(line0), "** DANGER! **");
                snprintf(line1, sizeof(line1), "SMOKE/GAS ALERT!");
            } else {
                snprintf(line0, sizeof(line0), "ADC: %u EXCEEDED", reading.filteredAdc);
                snprintf(line1, sizeof(line1), gsm.lastSmsSentSuccess ? "SMS ALERT SENT" : "CHECK ENV NOW!");
            }
            break;

        case SystemState::RECOVERY:
            snprintf(line0, sizeof(line0), "AIR RECOVERING");
            snprintf(line1, sizeof(line1), "ADC: %u (STABLE)", reading.filteredAdc);
            break;
    }

    writeLine(0, line0);
    writeLine(1, line1);
}
