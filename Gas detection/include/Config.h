#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// =============================================================================
// HARDWARE PIN ALLOCATION (ESP8266 NodeMCU)
// =============================================================================
// MQ-2 Smoke and Combustible Gas Sensor
// Note: ESP8266 A0 accepts 0 - 1.0V (0 - 3.3V on NodeMCU on-board divider).
// MQ-2 AO (0 - 5V) must be conditioned with an external resistor divider.
constexpr uint8_t PIN_MQ2_AO          = A0;

// I2C Display (16x2 or 20x4 LCD with PCF8574 backpack)
constexpr uint8_t PIN_LCD_SDA         = D2; // GPIO4
constexpr uint8_t PIN_LCD_SCL         = D1; // GPIO5

// Relay Module for Demonstration Lamp
constexpr uint8_t PIN_RELAY           = D5; // GPIO14

// Buzzer Driver Module / Transistor
constexpr uint8_t PIN_BUZZER          = D0; // GPIO16

// SIM800L GSM Module (SoftwareSerial)
// Crossed serial connection:
// SIM800L TXD -> ESP8266 D6 (RX)
// SIM800L RXD <- ESP8266 D7 (TX) via 5V/3.3V level shifter or divider
constexpr uint8_t PIN_GSM_RX          = D6; // GPIO12 (ESP8266 RX)
constexpr uint8_t PIN_GSM_TX          = D7; // GPIO13 (ESP8266 TX)

// =============================================================================
// HARDWARE LOGIC POLARITY & ELECTRICAL CONFIGURATION
// =============================================================================
// Relay: Most standard relay modules are ACTIVE LOW (LOW = Relay ON, HIGH = Relay OFF).
// Set to HIGH if using an ACTIVE HIGH relay module.
constexpr uint8_t RELAY_ACTIVE_LOGIC  = LOW;

// Buzzer: Active buzzer driven by HIGH signal on GPIO16
constexpr uint8_t BUZZER_ACTIVE_LOGIC = HIGH;

// I2C LCD Configuration
constexpr uint8_t LCD_I2C_ADDRESS     = 0x27; // Default PCF8574 address (change to 0x3F if needed)
constexpr uint8_t LCD_COLUMNS         = 16;   // 16 or 20 columns
constexpr uint8_t LCD_ROWS            = 2;    // 2 or 4 rows

// Serial Monitor Baud Rate
constexpr uint32_t SERIAL_BAUD_RATE   = 115200;

// SIM800L Baud Rate (Default factory baud is usually 9600)
constexpr uint32_t GSM_BAUD_RATE      = 9600;

// =============================================================================
// SENSOR CALIBRATION & ALARM THRESHOLDS
// =============================================================================
// Sensor Warm-up Duration:
// MQ-2 sensors require a warm-up phase for internal heater stabilization.
// Recommended: 20 seconds for prototype testing (real-world burn-in is 24h-48h).
constexpr uint32_t SENSOR_WARMUP_MS         = 20000UL;

// ADC Alarm Threshold (0 - 1023):
// Baseline clean air typically reads ~100-300 depending on voltage divider.
// Adjust this threshold based on your specific baseline calibration.
constexpr uint16_t GAS_ALARM_THRESHOLD_ADC  = 450;

// Hysteresis ADC margin:
// To avoid rapid bouncing, air must drop below (THRESHOLD - HYSTERESIS) to clear alarm.
constexpr uint16_t GAS_HYSTERESIS_ADC       = 40;

// Debounce / Persistence Filter:
// Number of consecutive samples above threshold required before triggering alarm.
constexpr uint8_t  ALARM_PERSIST_SAMPLES    = 3;

// Recovery Stabilization Time:
// Time in milliseconds that sensor must remain continuously below recovery threshold
// before the system officially recovers to normal state.
constexpr uint32_t RECOVERY_HOLD_MS         = 5000UL;

// Exponential Moving Average filter alpha (0.0f - 1.0f).
// Lower value = smoother filter; Higher value = faster response.
constexpr float    SENSOR_FILTER_ALPHA      = 0.25f;

// =============================================================================
// GSM & SMS NOTIFICATION CONFIGURATION
// =============================================================================
// Registered recipient mobile phone number (international format recommended, e.g. "+1234567890")
#define TARGET_PHONE_NUMBER "+233248666037"

// Minimum cooldown interval between SMS alert dispatches during a persistent alarm (ms)
constexpr uint32_t SMS_ALERT_COOLDOWN_MS    = 120000UL; // 2 minutes

// Whether to send an SMS when the alarm condition recovers
constexpr bool     SEND_RECOVERY_SMS        = true;

// Alert SMS Text Content
#define SMS_ALERT_MESSAGE \
  "SMOKE AND GAS ALERT!\n\n" \
  "Abnormal smoke/gas levels detected by the IoT monitoring system.\n" \
  "Please investigate immediately and safely!"

// Recovery SMS Text Content
#define SMS_RECOVERY_MESSAGE \
  "SYSTEM RECOVERY NOTICE:\n\n" \
  "Environmental smoke/gas levels have returned to normal operating range.\n" \
  "Continuous monitoring active."

// =============================================================================
// TIMING INTERVALS (NON-BLOCKING MILLIS SCHEDULING)
// =============================================================================
constexpr uint32_t INTERVAL_SENSOR_SAMPLE_MS  = 200UL;  // Read sensor every 200ms (5Hz)
constexpr uint32_t INTERVAL_DISPLAY_UPDATE_MS = 500UL;  // Refresh LCD every 500ms
constexpr uint32_t INTERVAL_GSM_CHECK_MS      = 5000UL; // Check GSM network status every 5s
constexpr uint32_t INTERVAL_BUZZER_PULSE_MS   = 200UL;  // Alarm buzzer beep pulse duration
constexpr uint32_t INTERVAL_STATUS_PRINT_MS   = 2000UL; // Print telemetry to Serial Monitor

#endif // CONFIG_H
