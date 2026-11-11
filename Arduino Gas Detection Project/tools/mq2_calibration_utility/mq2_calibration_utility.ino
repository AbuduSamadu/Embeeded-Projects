/**
 * @file mq2_calibration_utility.ino
 * @brief Calibration tool to monitor MQ-2 raw ADC readings, stability, and determine baseline.
 */

#include <Arduino.h>

const int MQ2_PIN = A0;

void setup() {
    Serial.begin(9600);
    Serial.println(F("\n================================================"));
    Serial.println(F("     MQ-2 GAS SENSOR CALIBRATION UTILITY       "));
    Serial.println(F("================================================"));
    Serial.println(F("Allow sensor to warm up in clean ambient air."));
    Serial.println(F("Time(s)\tRaw_ADC\tVoltage(V)\tStatus"));
}

void loop() {
    unsigned long sec = millis() / 1000;
    int rawValue = analogRead(MQ2_PIN);
    float voltage = rawValue * (5.0 / 1023.0);

    Serial.print(sec);
    Serial.print(F("s\t"));
    Serial.print(rawValue);
    Serial.print(F("\t"));
    Serial.print(voltage, 2);
    Serial.print(F("V\t"));

    if (sec < 20) {
        Serial.println(F("[WARMING UP - Unstable]"));
    } else {
        Serial.println(F("[STABILIZED BASELINE]"));
    }

    delay(1000);
}
