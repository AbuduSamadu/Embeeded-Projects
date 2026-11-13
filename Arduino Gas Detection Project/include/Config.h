/**
 * @file Config.h
 * @brief User configuration, parameters, and tuning constants.
 * 
 * Centralized configuration following the Separation of Concerns principle.
 * Modifying thresholds, timings, or network credentials does not require
 * editing application logic or hardware driver implementation.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

namespace SystemConfig {

    // =========================================================================
    // GSM / SIM800L CONFIGURATION
    // =========================================================================
    /// Recipient mobile phone number for SMS notifications (international format)
    constexpr const char* EMERGENCY_PHONE_NUMBER = "+233248666037";

    /// SIM800L UART baud rate (9600 is recommended for SoftwareSerial stability)
    constexpr unsigned long GSM_BAUD_RATE = 9600;

    /// Cooldown period between alert SMS dispatches (milliseconds) to prevent spam
    constexpr unsigned long SMS_ALERT_COOLDOWN_MS = 60000UL; // 60 seconds

    /// Set to true to send an SMS when air quality normalizes following an alarm
    constexpr bool ENABLE_RECOVERY_SMS = true;

    // =========================================================================
    // MQ-2 SENSOR CONFIGURATION
    // =========================================================================
    /// Sensor warm-up duration in milliseconds.
    /// (For initial demonstration / testing: 20 seconds. Manufacturer recommendation
    /// for full chemical burn-in is 24-48 hours, with 60-180 seconds typical for daily power-up).
    constexpr unsigned long SENSOR_WARMUP_TIME_MS = 20000UL;

    /// Sensor sampling interval in milliseconds
    constexpr unsigned long SENSOR_SAMPLE_INTERVAL_MS = 100UL;

    /// Number of rolling samples for moving average noise rejection filter
    constexpr uint8_t SENSOR_FILTER_WINDOW_SIZE = 8;

    /// Raw ADC threshold for smoke/gas alarm condition (range: 0 to 1023)
    /// Adjust according to ambient environment baseline and calibration
    constexpr uint16_t GAS_ALARM_THRESHOLD = 350;

    /// Hysteresis offset for alarm recovery (alarm clears when reading < THRESHOLD - HYSTERESIS)
    /// Prevents bouncing/oscillation when gas level fluctuates near the threshold
    constexpr uint16_t GAS_ALARM_HYSTERESIS = 50;

    /// Time (ms) sensor reading must stay above threshold before alarm is triggered (debounce)
    constexpr unsigned long ALARM_TRIGGER_CONFIRM_MS = 1500UL;

    /// Time (ms) sensor reading must stay below recovery threshold before clearing alarm
    constexpr unsigned long ALARM_RECOVERY_CONFIRM_MS = 3000UL;

    // =========================================================================
    // ACTUATOR CONFIGURATION (BUZZER & RELAY)
    // =========================================================================
    /// True if relay module is Active-LOW (most standard Arduino 5V relay breakout boards)
    constexpr bool RELAY_ACTIVE_LOW = true;

    /// Normal operating state of the demonstration lamp (true = ON, false = OFF)
    /// Lamp is ALWAYS forced OFF during alarm for safety (eliminating switching spark hazard)
    constexpr bool RELAY_LAMP_NORMAL_ON = true;

    /// Buzzer active polarity (true = HIGH to sound, false = LOW to sound)
    constexpr bool BUZZER_ACTIVE_HIGH = true;

    /// Set to true if using a passive buzzer (piezo transducer requiring tone() AC frequency)
    /// Set to false if using an active buzzer (integrated oscillator driven by DC voltage)
    constexpr bool BUZZER_IS_PASSIVE = false;

    /// Audible tone frequency in Hertz if BUZZER_IS_PASSIVE is true (typically 2000 - 3000 Hz)
    constexpr unsigned int BUZZER_TONE_FREQ_HZ = 2500;

    /// Alarm buzzer pulsation ON duration in milliseconds
    constexpr unsigned long BUZZER_CADENCE_ON_MS = 250UL;

    /// Alarm buzzer pulsation OFF duration in milliseconds
    constexpr unsigned long BUZZER_CADENCE_OFF_MS = 250UL;

    // =========================================================================
    // DISPLAY CONFIGURATION (16x2 I2C LCD)
    // =========================================================================
    /// I2C Address of PCF8574 backpack (commonly 0x27 or 0x3F)
    constexpr uint8_t LCD_I2C_ADDRESS = 0x27;

    constexpr uint8_t LCD_COLUMNS = 16;
    constexpr uint8_t LCD_ROWS    = 2;

    /// Display refresh interval in milliseconds (prevents I2C bus flooding)
    constexpr unsigned long LCD_REFRESH_INTERVAL_MS = 300UL;

    // =========================================================================
    // SYSTEM / SERIAL DIAGNOSTICS
    // =========================================================================
    /// Hardware Serial baud rate for debugging
    constexpr unsigned long SERIAL_MONITOR_BAUD = 9600;

    /// Periodic serial heartbeat log interval in milliseconds
    constexpr unsigned long SERIAL_LOG_INTERVAL_MS = 2000UL;

} // namespace SystemConfig

#endif // CONFIG_H
