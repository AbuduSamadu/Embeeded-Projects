#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "Config.h"
#include "SystemTypes.h"

class DisplayManager {
public:
    DisplayManager(uint8_t i2cAddr = LCD_I2C_ADDRESS, uint8_t cols = LCD_COLUMNS, uint8_t rows = LCD_ROWS);

    void begin();
    void showInitializing();
    void update(SystemState state, const SensorReading& reading, const GSMTelemetry& gsm);

private:
    LiquidCrystal_I2C _lcd;
    uint8_t           _cols;
    uint8_t           _rows;
    uint32_t          _lastUpdate;
    uint8_t           _pageToggle;

    char _lineBuffer0[21];
    char _lineBuffer1[21];

    void writeLine(uint8_t row, const char* text);
    const char* gsmStatusToString(GSMNetworkStatus status);
};

#endif // DISPLAY_MANAGER_H
