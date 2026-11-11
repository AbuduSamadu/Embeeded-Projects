/**
 * @file LcdDisplay.h
 * @brief Concrete driver for the 16x2 I2C Character LCD (HD44780 + PCF8574).
 * 
 * Implements IDisplay.
 * Employs line caching and throttled refreshes to eliminate screen flicker
 * and minimize I2C bus traffic.
 */

#ifndef LCD_DISPLAY_H
#define LCD_DISPLAY_H

#include <LiquidCrystal_I2C.h>
#include "interfaces/IDisplay.h"
#include "Config.h"

class LcdDisplay : public IDisplay {
public:
    explicit LcdDisplay(uint8_t address = SystemConfig::LCD_I2C_ADDRESS,
                        uint8_t cols = SystemConfig::LCD_COLUMNS,
                        uint8_t rows = SystemConfig::LCD_ROWS);

    void init() override;
    void update(const SystemTelemetry& telemetry) override;
    void showMessage(const char* line1, const char* line2) override;
    void clear() override;

private:
    LiquidCrystal_I2C _lcd;
    const uint8_t _cols;
    const uint8_t _rows;

    char _currentLine0[17];
    char _currentLine1[17];
    unsigned long _lastRefreshTime;
    uint8_t _alarmScreenToggle;

    void printLinePadded(uint8_t row, const char* text);
    const char* gsmStatusShortString(GsmNetworkStatus status);
};

#endif // LCD_DISPLAY_H
