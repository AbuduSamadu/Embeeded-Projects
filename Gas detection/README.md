# IoT-Based Smoke Detection, Gas Leakage Alert and Automatic Light Control System

**Target Microcontroller:** ESP8266 NodeMCU  
**Cellular Module:** SIM800L GSM  
**Gas/Smoke Sensor:** MQ-2 Sensor  
**Display:** 16x2 / 20x4 I2C LCD (PCF8574)  
**Output Devices:** Active Buzzer, Relay Module & Demonstration LED Lamp  

---

## 1. Hardware Pinout & Wiring

| Component | Pin | NodeMCU Pin | GPIO | Electrical Notes |
|---|---|---|---|---|
| **MQ-2** | AO | **A0** | ADC0 | Resistor voltage divider (5V to ≤3.3V) |
| **MQ-2** | VCC | External 5V | - | Requires ~150-200mA for internal heater |
| **MQ-2** | GND | Common GND | - | Common Ground reference |
| **I2C LCD** | SDA | **D2** | GPIO4 | I2C Data line |
| **I2C LCD** | SCL | **D1** | GPIO5 | I2C Clock line |
| **I2C LCD** | VCC | 5V / 3.3V | - | Level-shifter recommended if 5V pull-ups |
| **I2C LCD** | GND | Common GND | - | Common Ground reference |
| **Relay** | IN | **D5** | GPIO14 | Default active-low (configurable) |
| **Relay** | VCC | External 5V | - | Separate supply recommended |
| **Relay** | GND | Common GND | - | Common Ground reference |
| **Buzzer** | IN | **D0** | GPIO16 | NPN / driver transistor recommended |
| **SIM800L** | TXD | **D6** | GPIO12 | ESP8266 SoftwareSerial RX |
| **SIM800L** | RXD | **D7** | GPIO13 | ESP8266 SoftwareSerial TX (via 3.3V level shift) |
| **SIM800L** | VCC | Regulated 4.0V | - | High-current burst capability (2A peak) |
| **SIM800L** | GND | Common GND | - | Common Ground reference |

---

## 2. Software Architecture & Separation of Concerns

The codebase follows the Single Responsibility Principle (SRP) and strict Separation of Concerns (SoC):

```
├── include/
│   ├── Config.h             <-- Centralized hardware mappings, thresholds & timings
│   └── SystemTypes.h        <-- Enums (SystemState, GSMStatus) and data models
├── src/
│   ├── BuzzerController.h/.cpp <-- Non-blocking audio cadence & beeper driver
│   ├── RelayController.h/.cpp  <-- Lighting control with safety lockout interlock
│   ├── SensorManager.h/.cpp    <-- MQ-2 sampling, EMA filtering, debounce & hysteresis
│   ├── GSMManager.h/.cpp       <-- SIM800L AT engine, network polling, rate-limited SMS
│   ├── DisplayManager.h/.cpp   <-- LiquidCrystal_I2C screen renderer & page rotator
│   ├── SystemController.h/.cpp <-- Master finite state machine orchestrator
│   └── main.cpp                <-- Minimal setup() & loop() application entry
└── platformio.ini           <-- Build environment and library dependencies
```

---

## 3. Finite State Machine (FSM) Workflow

1. **INITIALIZING**: Hardware peripherals booted; buzzer chirp confirmation; relay locked OFF; SIM800L checked.
2. **WARMUP**: Sensor heater stabilization countdown (default 20s for prototype). Alarm evaluation suppressed.
3. **NORMAL**: Real-time ADC monitoring with exponential moving average filtering. Demonstration lamp enabled. GSM network checked.
4. **ALARM**: Triggered when ADC ≥ Threshold for sustained samples.
   - **Immediate Safety Interlock:** Demonstration lamp is forcibly shut down!
   - Buzzer alerts with audible pulse cadence.
   - LCD displays danger alert and gas reading.
   - SMS notification dispatched to registered recipient (rate-limited / one-shot).
5. **RECOVERY**: Triggered when reading falls below `Threshold - Hysteresis` and remains stable for hold time. Buzzer silenced, optional recovery SMS sent, return to normal monitoring.

---

## 4. Interactive Serial Commands (115200 Baud)

For bench testing and validation without smoke or gas:
- `t` : Simulate gas spike (`ADC = 750`) to verify alarm trigger and safety lockout.
- `n` : Simulate clean air (`ADC = 180`) to verify hysteresis recovery.
- `c` : Clear simulation and return to physical MQ-2 reading.
- `s` : Test dispatching SMS to the configured recipient.
- `l` : Toggle demonstration lamp (only permitted during `NORMAL` state).
- `h` : Print interactive command help menu.
