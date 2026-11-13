/**
 * @file PinDefinitions.h
 * @brief Hardware pin allocations for the Arduino Uno R3 Smoke & Gas Detection System.
 * 
 * Hardware Connections:
 * - MQ-2 Gas/Smoke Sensor: AO -> A0, VCC -> 5V, GND -> GND
 * - LCD 1602 I2C Backpack: SDA -> A4, SCL -> A5, VCC -> 5V, GND -> GND
 * - Relay Module: IN -> D7, VCC -> 5V, GND -> GND
 * - Buzzer Driver: IN -> D8, GND -> GND
 * - SIM800L GSM Module: TXD -> D10 (Arduino RX), RXD -> D11 (Arduino TX with level divider)
 *                       VCC -> External Regulated ~4.0V (2A burst), GND -> Common GND
 */

#ifndef PIN_DEFINITIONS_H
#define PIN_DEFINITIONS_H

#include <Arduino.h>

namespace HardwarePins {

    // --- Sensing Layer ---
    constexpr uint8_t PIN_MQ2_ANALOG      = A0;  ///< MQ-2 Analogue output (AO)

    // --- Control Layer (Actuators) ---
    constexpr uint8_t PIN_RELAY_CONTROL   = 7;   ///< Relay IN pin (controls 12V LED Demonstration Lamp)
    constexpr uint8_t PIN_BUZZER_CONTROL  = 8;   ///< Buzzer driver IN pin

    // --- Communication Layer (GSM SIM800L via SoftwareSerial) ---
    constexpr uint8_t PIN_SIM800L_RX      = 10;  ///< Arduino RX (connects to SIM800L TXD)
    constexpr uint8_t PIN_SIM800L_TX      = 11;  ///< Arduino TX (connects to SIM800L RXD via voltage divider)

    // --- Display Layer (I2C) ---
    // Note: Arduino Uno hardware I2C pins are A4 (SDA) and A5 (SCL)
    constexpr uint8_t PIN_I2C_SDA         = A4;  ///< I2C Data
    constexpr uint8_t PIN_I2C_SCL         = A5;  ///< I2C Clock

} // namespace HardwarePins

#endif // PIN_DEFINITIONS_H
