# Arduino Uno-Based Smoke Detection and Gas Leakage Alert System with GSM Notification and Automatic Light Control

An educational embedded systems prototype designed to monitor combustible gas and smoke levels using an **MQ-2 sensor**, provide real-time status on an **I2C 16x2 LCD**, actuate a local **buzzer** alarm, dispatch emergency SMS alerts via a **SIM800L GSM module**, and automatically manage a demonstration **12V LED lamp** via a **relay** module for spark safety isolation.

---

## 1. Hardware Architecture & Wiring Specifications

### 1.1 Complete Wiring Matrix

| Component | Module Pin | Arduino Uno R3 Pin | Electrical Notes |
|---|---|---|---|
| **MQ-2 Gas Sensor** | `VCC` | `5V` | Regulated 5V rail (~150mA heater) |
| | `GND` | `GND` | Common Ground |
| | `AO` (Analogue Out) | `A0` | 0–5V ADC input |
| | `DO` (Digital Out) | *Not connected* | Analog monitoring is used |
| **1602 LCD (I2C Backpack)** | `VCC` | `5V` | 5V supply |
| | `GND` | `GND` | Common Ground |
| | `SDA` | `A4` | Dedicated I2C Data line |
| | `SCL` | `A5` | Dedicated I2C Clock line |
| **Relay Module (1-Channel)** | `VCC` | `5V` | Module power |
| | `GND` | `GND` | Common Ground |
| | `IN` | `D7` | Configurable Active-LOW / Active-HIGH |
| | `COM` / `NO` | *External Circuit* | Switched loop for 12V Demonstration Lamp |
| **Buzzer Driver** | `IN` / `+` | `D8` | GPIO control (or transistor base via 1k resistor) |
| | `GND` / `-` | `GND` | Common Ground |
| **SIM800L GSM Module** | `VCC` | **External 3.8V – 4.2V** | **MUST NOT** connect to Arduino 5V/3.3V (requires 2A peak current) |
| | `GND` | `GND` | **Must share common reference ground with Arduino** |
| | `TXD` | `D10` | Arduino SoftwareSerial RX (3.3V out is accepted as HIGH by Uno) |
| | `RXD` | `D11` | Arduino SoftwareSerial TX **via level shifter / resistor divider** |

### 1.2 SIM800L Voltage & Level Conditioning
The SIM800L GSM module requires special attention to two electrical parameters:
1. **Power Supply**: SIM800L operates between **3.7V and 4.2V** (nominal 4.0V) and draws instantaneous current bursts of up to **2.0A** during cellular network transmission. An external buck converter (such as LM2596 tuned to 4.0V) or Li-ion 3.7V cell with a large 1000µF low-ESR electrolytic capacitor close to the module's VCC/GND pins is mandatory.
2. **Logic Level Conditioning**: The SIM800L UART is rated for **2.8V – 3.3V** logic. Connecting the Arduino Uno's 5V TX (D11) directly to SIM800L RXD can damage the module over time. Use a simple resistive voltage divider:
   - Arduino `D11` → `1kΩ resistor` → SIM800L `RXD`
   - SIM800L `RXD` → `2kΩ resistor` → `GND`
   *(This conditions the 5.0V signal down to a safe ~3.3V).*

### 1.3 Relay Safety Isolation
In gas and combustible vapor safety applications, electrical switching can produce arcs and sparks capable of igniting accumulated gases. For demonstration:
- An isolated, low-voltage **12V LED Demonstration Lamp** is switched.
- In normal operating conditions, the lamp operates in its configured state (ON).
- During any abnormal gas/smoke condition, the system **immediately de-energizes the relay (forces Lamp OFF)** to eliminate ignition spark potential.

---

## 2. Software Architecture & Clean Design Principles

The codebase is built on **Object-Oriented Clean Architecture** and strictly follows the **SOLID** design principles:

```
include/
├── Config.h                  # Centralized configuration (parameters, timings, thresholds)
├── PinDefinitions.h          # Hardware pin assignments
├── SystemTypes.h             # Enums, telemetry structures, state definitions
├── interfaces/
│   ├── ISensor.h             # Abstract sensor contract (OCP/ISP/DIP)
│   ├── IActuator.h           # Abstract actuator contract (buzzer, relay)
│   ├── IDisplay.h            # Abstract display presentation contract
│   └── INotificationService.h# Abstract notification service contract
├── drivers/
│   ├── MQ2Sensor.h           # Sensor driver with rolling filter & hysteresis
│   ├── Buzzer.h              # Audible alert driver with non-blocking cadence
│   ├── Relay.h               # Safe relay switching with active-low inversion
│   ├── LcdDisplay.h          # Flicker-free I2C LCD renderer with line caching
│   └── Sim800lGsm.h          # Asynchronous, non-blocking AT command state machine
└── controllers/
    └── SafetyController.h    # Application orchestrator and safety state machine
src/
├── controllers/SafetyController.cpp
├── drivers/
│   ├── Buzzer.cpp
│   ├── LcdDisplay.cpp
│   ├── MQ2Sensor.cpp
│   ├── Relay.cpp
│   └── Sim800lGsm.cpp
└── main.cpp                  # Composition root (dependency injection & setup/loop)
```

### Applied Design Principles:
1. **Single Responsibility Principle (SRP)**:
   - `MQ2Sensor`: Handles only sampling, rolling-average noise filtering, and threshold evaluation.
   - `Buzzer`: Handles only cadence pulsing and frequency toggling.
   - `Relay`: Handles physical polarity inversion and fail-safe latching.
   - `LcdDisplay`: Handles screen formatting, layout, and I2C write optimization without business logic.
   - `Sim800lGsm`: Handles cellular registration, AT sequencing, and SMS queuing.
   - `SafetyController`: Encapsulates system state transitions and orchestrates peripherals.
2. **Open/Closed Principle (OCP)**:
   - Adding additional sensors (e.g., MQ-7, flame sensor) or notification mechanisms (e.g., LoRa, WiFi MQTT) only requires implementing `ISensor` or `INotificationService` without modifying the core controller logic.
3. **Liskov Substitution Principle (LSP)**:
   - All drivers can be substituted by mock drivers (see `test/test_safety_logic/test_main.cpp`) for unit testing without hardware.
4. **Interface Segregation Principle (ISP)**:
   - Clean, minimal interfaces without bloated methods.
5. **Dependency Inversion Principle (DIP)**:
   - `SafetyController` depends purely on abstractions (`ISensor`, `IActuator`, `IDisplay`, `INotificationService`), not concrete hardware classes.
6. **100% Non-Blocking Execution**:
   - Zero blocking `delay()` calls in runtime loops. GSM AT commands and SMS dispatch execute via a background state machine using `millis()`. Sensor sampling and buzzer alarm cadence never stall.

---

## 3. Configuration Guide (`Config.h`)

All tunable parameters are decoupled into `include/Config.h`:

```cpp
// Emergency SMS Recipient (international format)
constexpr const char* EMERGENCY_PHONE_NUMBER = "+1234567890";

// Sensor Parameters
constexpr unsigned long SENSOR_WARMUP_TIME_MS = 20000UL; // 20s demonstration warmup
constexpr uint16_t GAS_ALARM_THRESHOLD        = 350;     // Raw ADC alert threshold (0-1023)
constexpr uint16_t GAS_ALARM_HYSTERESIS       = 50;      // Clears below (350 - 50 = 300)
constexpr unsigned long ALARM_TRIGGER_CONFIRM_MS  = 1500UL; // 1.5s debounce before alarm
constexpr unsigned long ALARM_RECOVERY_CONFIRM_MS = 3000UL; // 3.0s debounce before clearing

// Actuator Polarities
constexpr bool RELAY_ACTIVE_LOW    = true; // Standard Arduino relay boards
constexpr bool RELAY_LAMP_NORMAL_ON = true; // Lamp is ON in normal state, OFF in alarm
constexpr bool BUZZER_ACTIVE_HIGH  = true; // Active buzzer

// Anti-Spam Rate Limiting
constexpr unsigned long SMS_ALERT_COOLDOWN_MS = 60000UL; // 60s cooldown between SMS dispatches
constexpr bool ENABLE_RECOVERY_SMS            = true;    // Send SMS when air returns to normal
```

---

## 4. Operational State Machine

```
      +--------------------+
      |    INITIALIZING    |  (Peripherals setup, relay safe OFF, buzzer OFF)
      +---------+----------+
                |
                v
      +--------------------+
      |      WARMUP        |  (Stabilize MQ-2 heater, LCD countdown, alarms masked)
      +---------+----------+
                | Warmup complete (20s)
                v
      +--------------------+
+---> |      NORMAL        | <---+  (Relay ON, buzzer OFF, LCD "AIR:OK", monitoring)
|     +---------+----------+     |
|               |                |
|               | Reading > 350  |
|               | for > 1.5s     |
|               v                |
|     +--------------------+     | Reading < 300
|     |      ALARM         |     | for > 3.0s
|     | - Relay OFF (Safe) |     |
|     | - Buzzer Pulsing   |     |
|     | - SMS Alert Sent   |     |
|     +---------+----------+     |
|               |                |
|               v                |
|     +--------------------+     |
+-----+    RECOVERY        | ----+  (Buzzer OFF, Recovery SMS sent, Relay restored)
      +--------------------+
```

---

## 5. Step-by-Step Bench Testing Procedure

The project includes test utilities in the `tools/` directory to verify each module independently:

### Test 1: PlatformIO Build Verification
Compile the project to confirm toolchain and dependency resolution:
```bash
pio run
```
*Expected Output: `SUCCESS` with ~55% RAM and ~50% Flash usage.*

### Test 2: I2C LCD Address Scanner
If the LCD display stays blank or shows solid boxes, verify its I2C address using `tools/i2c_scanner/i2c_scanner.ino`.
1. Upload `tools/i2c_scanner/i2c_scanner.ino`.
2. Open Serial Monitor at 9600 baud.
3. Verify detected address (typically `0x27` or `0x3F`). Update `LCD_I2C_ADDRESS` in `Config.h` if needed.
4. Adjust the blue contrast potentiometer on the back of the LCD backpack until characters are clear.

### Test 3: MQ-2 Sensor Calibration & Baseline
1. Upload `tools/mq2_calibration_utility/mq2_calibration_utility.ino`.
2. Allow sensor to warm up in clean ambient room air for at least 2 minutes.
3. Observe baseline ADC reading. In clean air, typical baseline values are 80–180.
4. Set `GAS_ALARM_THRESHOLD` in `Config.h` approximately 150–200 units above your ambient baseline.

### Test 4: SIM800L Diagnostic & AT Passthrough
1. Upload `tools/gsm_sim800l_diagnostic/gsm_sim800l_diagnostic.ino`.
2. Open Serial Monitor (9600 baud, Both NL & CR).
3. Send test AT commands:
   - `AT` → Returns `OK` (UART communication verified).
   - `AT+CPIN?` → Returns `+CPIN: READY` (SIM card recognized).
   - `AT+CSQ` → Returns signal quality (e.g. `+CSQ: 18,0`; values > 10 indicate good reception).
   - `AT+CREG?` → Returns `+CREG: 0,1` (Registered on home network) or `0,5` (Roaming).
   - `AT+CBC` → Returns power supply voltage (must be between 3700 and 4200 mV).

### Test 5: Actuators (Buzzer & Relay)
1. Verify relay click sound and LED indication on D7.
2. Verify buzzer pulse on D8.

### Test 6: Full Integration Test
1. Set your actual recipient phone number in `include/Config.h`.
2. Build and upload main firmware:
   ```bash
   pio run -t upload
   ```
3. Open Serial Monitor at 9600 baud.
4. Observe startup sequence:
   - LCD displays `SYSTEM INIT...` then `SENSOR WARM-UP` with countdown.
   - GSM initializes asynchronously and reports network status.
   - Relay engages (Demonstration lamp turns ON in normal mode).
   - Introduce test gas (e.g., butane from an unlit lighter held gently 5cm from sensor for 1-2 seconds; do NOT release large amounts).
   - System triggers: Buzzer pulses, Relay instantly opens (Lamp OFF), LCD shows `WARNING DETECTED`, SMS alert is submitted.
   - Once gas dissipates below recovery threshold, system confirms recovery, silences buzzer, sends recovery SMS, and re-engages normal lighting.

---

## 6. Safety & Educational Prototype Disclaimer

> [!CAUTION]
> **Educational Prototype Only**: This project is designed as an educational demonstration of microcontroller interfacing, sensor signal processing, GSM communications, and fail-safe switching.
> - The MQ-2 sensor is **NOT** a certified life-safety device and must **NEVER** be used as a primary smoke, fire, or gas protection system.
> - Never deliberately release hazardous quantities of combustible gases or smoke in enclosed spaces.
> - Never switch standard mains high-voltage lighting in an environment where combustible gases might accumulate. Sparks created across mechanical relay contacts can ignite flammable vapor. Always maintain isolated low-voltage demonstration loads.
