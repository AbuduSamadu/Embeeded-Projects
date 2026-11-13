/**
 * @file gsm_sim800l_diagnostic.ino
 * @brief Interactive passthrough console for SIM800L GSM testing & AT debugging.
 * 
 * Hardware setup:
 * - SIM800L TXD -> Arduino Pin 10 (SoftwareSerial RX)
 * - SIM800L RXD -> Arduino Pin 11 (SoftwareSerial TX, via 1k/2k voltage divider)
 * - SIM800L VCC -> External 4.0V / 2A supply
 * - SIM800L GND -> Arduino & Supply common GND
 */

#include <Arduino.h>
#include <SoftwareSerial.h>

SoftwareSerial gsmSerial(10, 11); // RX, TX

void setup() {
    Serial.begin(9600);
    gsmSerial.begin(9600);

    Serial.println(F("\n=============================================="));
    Serial.println(F("     SIM800L GSM INTERACTIVE AT CONSOLE       "));
    Serial.println(F("=============================================="));
    Serial.println(F("Commands to test:"));
    Serial.println(F("  AT              -> Check communication (OK)"));
    Serial.println(F("  AT+CPIN?        -> Check SIM card ready"));
    Serial.println(F("  AT+CSQ          -> Check signal strength (10-31 is good)"));
    Serial.println(F("  AT+CREG?        -> Check network registration (0,1 or 0,5)"));
    Serial.println(F("  AT+CBC          -> Check battery/input voltage"));
    Serial.println(F("Type your AT command and press ENTER:\n"));
}

void loop() {
    // Forward from SIM800L to Serial Monitor
    if (gsmSerial.available()) {
        Serial.write(gsmSerial.read());
    }

    // Forward from Serial Monitor to SIM800L
    if (Serial.available()) {
        gsmSerial.write(Serial.read());
    }
}
