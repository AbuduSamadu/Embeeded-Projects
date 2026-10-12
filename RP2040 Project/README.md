# RP2040 Zero Snake Arcade Game

A high-performance, modular arcade Snake game with rich retro sound effects and vibrant graphics, running on a **Raspberry Pi RP2040 Zero** with an **ST7789 IPS LCD** (240x240), an **analog 2-axis joystick**, a dedicated **Select button**, and an **audio buzzer**.

Strictly engineered with clean software architecture (**SOLID principles**, **Separation of Concerns**, **Dependency Injection**).

---

## 📖 Table of Contents
1. [Hardware Bill of Materials](#hardware-bill-of-materials)
2. [Wiring & Pinout Reference](#wiring--pinout-reference)
3. [Game Features & Audio](#game-features--audio)
4. [Software Architecture & Design Principles](#software-architecture--design-principles)
5. [Configuration & Tuning](#configuration--tuning)
6. [Building & Uploading](#building--uploading)
7. [Troubleshooting & Calibration](#troubleshooting--calibration)

---

## 🛠 Hardware Bill of Materials

| Component | Description / Role |
| :--- | :--- |
| **Microcontroller** | Waveshare RP2040-Zero (or Raspberry Pi Pico) |
| **Display** | ST7789 IPS SPI TFT LCD (240 × 240, CS-less or standard) |
| **Joystick** | 2-Axis Analog Thumb Joystick (5-pin: VCC, GND, VRx, VRy, SW) |
| **Select Button** | Momentary tactile push button (SPST normally-open) |
| **Buzzer** | Passive or Active Piezo Buzzer |
| **Interconnects** | Breadboard & Dupont jumper wires |
| **Cable** | USB Type-C data cable |

---

## 🔌 Wiring & Pinout Reference

### Complete System Pin Map

```text
       RP2040-Zero
      +-----------+
  GP2 | o       o | GP3  ----> ST7789 SDA/MOSI
  GP4 | o       o | GP6  ----> ST7789 RES/RST
  GP7 | o       o | GP8  <---- Joystick SW (Optional)
 GP15 | o       o | GP26 <---- Joystick VRx (ADC0)
  3V3 | o       o | GP27 <---- Joystick VRy (ADC1)
  GND | o       o | 5V
      +-----------+
```

### Pin Connection Details

| Module | Pin Label | RP2040 Zero Pin | Function / Hardware Role | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **ST7789 LCD** | **GND** | **GND** | Common Ground | System reference ground |
| | **VCC** | **3V3** or **5V** | Display Power | 3V3 directly; use 5V if module has onboard 662K LDO regulator |
| | **SCL / SCK** | **GP2** | Hardware SPI0 SCK | SPI Clock line |
| | **SDA / MOSI**| **GP3** | Hardware SPI0 TX | SPI Data (MOSI) |
| | **DC / A0** | **GP4** | Data / Command Select | HIGH = Data, LOW = Command |
| | **RES / RST** | **GP6** | Display Reset | Active-low hardware reset |
| | **CS** | *N/A* | Chip Select | Leave disconnected (tied to GND internally on CS-less panels) |
| | **BLK / BL** | **3V3** | Backlight LED | Connect to 3V3 (or GND on active-low panels) |
| **Analog Joystick**| **VCC** | **3V3** | Potentiometer Reference | Powered from RP2040 3.3V rail |
| | **GND** | **GND** | Ground | Common system ground |
| | **VRx** | **GP26 / ADC0** | Horizontal Axis | Analog input (0–1023, center ~512) |
| | **VRy** | **GP27 / ADC1** | Vertical Axis | Analog input (0–1023, center ~512) |
| | **SW** | **GP8** | Built-in Thumb Switch | Active LOW with internal pull-up (optional secondary Select) |
| **Select Button** | **Signal** | **GP7** | Primary Select Button | Active LOW with internal pull-up (`INPUT_PULLUP`) |
| | **Other contact**| **GND** | Button Return | Connected directly to Ground |
| **Buzzer** | **Positive (+)**| **GP15** | Audio Signal | Driven with non-blocking square-wave tones |
| | **Negative (−)**| **GND** | Buzzer Return | Common system ground |

---

## 🎮 Game Features & Audio

### Gameplay
- **Fluid Analog Movement:** Joystick input utilizes real-time dominant axis selection with center deadband filtering and automatic power-on center calibration.
- **Dynamic Speed Progression:** As you eat apples, your score increases and each level ramps up the snake's velocity.
- **Arcade Visuals:** Neon Cyberpunk styling, drop shadows, level-up banners, and high score tracking.
- **Pause & Resume:** Press the Select button at any time during gameplay to pause and resume.

### Retro Arcade Sound Effects
| Sound Event | Trigger | Musical / Acoustic Characteristic |
| :--- | :--- | :--- |
| **Game Start** | Press Select on Title or Game Over screen | Upbeat 4-note ascending arcade chime (C5 → E5 → G5 → C6) |
| **Food Eaten** | Snake swallows an apple | Snappy two-tone 8-bit bite chirp (1046 Hz → 1318 Hz) |
| **Level Up** | Every 5 apples consumed | Triumphant victory fanfare (E5 → G5 → B5 → E6) |
| **Game Over** | Snake hits a wall or itself | Descending defeat crunch sequence (A4 → F4 → C4 → F3) |
| **Pause / Click** | Toggle Pause or UI interaction | Crisp UI notification clicks |

> [!NOTE]
> All buzzer routines are executed non-blockingly using asynchronous note sequencing, ensuring zero frame rate drop or stuttering during snake movement.

---

## 🏛 Software Architecture & Design Principles

```text
                  +-----------------------------+
                  |     Config::AppConfig       |
                  |  (Centralized parameters)   |
                  +--------------+--------------+
                                 |
  +--------------------+         |         +--------------------+
  |  IInputController  |         |         |      IBuzzer       |
  | (Abstract Interface)         |         | (Abstract Interface)
  +---------^----------+         |         +---------^----------+
            |                    |                   |
       [Implements]              |              [Implements]
            |                    |                   |
  +---------+----------+         |         +---------+----------+
  | JoystickController |         |         |  BuzzerController  |
  |  (ADC0, ADC1, GP7) |         |         |  (Non-blocking GP15)|
  +--------------------+         |         +--------------------+
            \                    |                    /
             \                   v                   /
              +-------------------------------------+
              |              SnakeGame              |
              |       (Decoupled Arcade Logic)      |
              +------------------+------------------+
                                 |
                                 v
                         [src/main.cpp]
                       (Composition Root)
```

- **Single Responsibility Principle (SRP):**
  - `JoystickController`: Dedicated to analog sampling, zero-point calibration, deadzone filtering, and button debouncing.
  - `BuzzerController`: Dedicated to asynchronous audio sequencing and pitch management.
  - `SnakeGame`: Contains only core game mechanics, rendering calls, and state transitions.
- **Dependency Inversion Principle (DIP):**
  - `SnakeGame` depends only on abstract interfaces (`IInputController`, `IBuzzer`), never on concrete hardware classes. All dependencies are injected at the Composition Root in `src/main.cpp`.
- **Open / Closed Principle (OCP):**
  - Input or audio hardware can be swapped (e.g., rotary encoder, I2S DAC) without touching a single line of `SnakeGame`.

---

## ⚙️ Configuration & Tuning

All hardware mappings and gameplay tuning parameters live in [`include/config/AppConfig.h`](include/config/AppConfig.h):

### Joystick Settings
```cpp
namespace Config::Joystick {
    constexpr int16_t DEADZONE    = 180;   // Deadband threshold around resting center (0-512)
    constexpr bool    INVERT_X    = false; // Flip Left/Right if your module is inverted
    constexpr bool    INVERT_Y    = false; // Flip Up/Down if your module is inverted
    constexpr bool    SWAP_AXES   = false; // Set to true if mounted rotated 90 degrees
    constexpr bool    ENABLE_SW   = true;  // Treat joystick thumb click (SW) as secondary SELECT
}
```

### Audio Toggle
```cpp
namespace Config::BuzzerConfig {
    constexpr bool    ENABLED     = true;  // Master audio mute switch (true = sound on)
}
```

---

## 🔨 Building & Uploading

### 1. Build the Firmware
Compile the project with PlatformIO:
```bash
pio run
```

### 2. Upload to RP2040 Zero
* Connect your RP2040 Zero via USB-C.
* Run:
  ```bash
  pio run -t upload
  ```
* **Bootloader Mode (if auto-upload does not detect board):**
  1. Hold down the **BOOT** button on the RP2040 Zero.
  2. Press and release the **RESET** button (or plug in the USB-C cable while holding **BOOT**).
  3. Release the **BOOT** button (board mounts as `RPI-RP2` drive).
  4. Run `pio run -t upload` or copy `.pio/build/rp2040zero/firmware.uf2` directly onto the mounted drive.

---

## ❓ Troubleshooting & Calibration

| Symptom | Cause | Solution |
| :--- | :--- | :--- |
| **Snake moves in the opposite direction on one axis** | Potentiometer wiper polarity differs by vendor | Set `INVERT_X = true` or `INVERT_Y = true` in `AppConfig.h`. |
| **Snake moves diagonally or turns when stick is barely moved** | Deadzone too small or stick mechanical drift | Increase `DEADZONE` (e.g., from `180` to `220`) in `AppConfig.h`. |
| **Select button not responding** | Wiring not pulling GP7 to GND when pressed | Verify one leg of the button connects to **GP7** and the other connects to **GND**. |
| **No sound from buzzer** | Polarity reversed or pin disconnected | Ensure positive (+) leg is connected to **GP15** and negative (−) is connected to **GND**. If using an active buzzer with fixed voltage requirements, check `BuzzerConfig::ENABLED`. |
| **Display backlight is dark** | Backlight pin not powered | Connect display **BLK** pin to **3V3** (or **GND** on active-LOW panels). |
