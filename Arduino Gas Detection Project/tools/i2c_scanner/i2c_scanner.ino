/**
 * @file i2c_scanner.ino
 * @brief Diagnostic utility to detect the I2C backpack address (typically 0x27 or 0x3F).
 */

#include <Arduino.h>
#include <Wire.h>

void setup() {
    Wire.begin();
    Serial.begin(9600);
    while (!Serial);
    Serial.println(F("\n--- I2C Bus Scanner ---"));
}

void loop() {
    byte error, address;
    int nDevices = 0;

    Serial.println(F("Scanning I2C bus..."));

    for (address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0) {
            Serial.print(F("I2C device found at address 0x"));
            if (address < 16) {
                Serial.print("0");
            }
            Serial.print(address, HEX);
            Serial.println(F(" !"));
            nDevices++;
        } else if (error == 4) {
            Serial.print(F("Unknown error at address 0x"));
            if (address < 16) {
                Serial.print("0");
            }
            Serial.println(address, HEX);
        }
    }

    if (nDevices == 0) {
        Serial.println(F("No I2C devices found. Check wiring (SDA=A4, SCL=A5, VCC, GND).\n"));
    } else {
        Serial.println(F("Scan complete.\n"));
    }

    delay(5000);
}
