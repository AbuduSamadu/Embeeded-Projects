# RP2040 Zero + ST7789 Display Node

A production-grade, modular embedded project demonstrating clean software architecture (**SOLID principles**, **Separation of Concerns**, **Dependency Injection**) running on an **RP2040 Zero** with an **ST7789 IPS LCD** (240x240, CS-less module).

---

## 📖 Table of Contents
1. [Project Overview](#project-overview)
2. [Hardware Requirements](#hardware-requirements)
3. [Wiring & Pinout Reference](#wiring--pinout-reference)
4. [Software Architecture & Design Principles](#software-architecture--design-principles)
5. [Prerequisites & Environment Setup](#prerequisites--environment-setup)
6. [Building & Uploading](#building--uploading)
7. [Serial Monitor & Usage](#serial-monitor--usage)
8. [Configuration & Customization](#configuration--customization)
9. [Troubleshooting & Common Pitfalls](#troubleshooting--common-pitfalls)

---

## 🚀 Project Overview

* **Target Microcontroller:** Raspberry Pi RP2040 (RP2040-Zero form factor).
* **Display Controller:** ST7789 SPI IPS LCD (240x240).
* **Graphics Library:** `TFT_eSPI` configured directly for RP2040 hardware SPI0 registers.
* **Architecture:** Enterprise-grade decoupling. Business logic is isolated from hardware drivers and configuration.

---

## 🛠 Hardware Requirements

| Item | Details |
| :--- | :--- |
| **Microcontroller** | Waveshare RP2040-Zero (or compatible RP2040 board) |
| **Display Module** | ST7789 IPS SPI LCD (1.3" / 1.54", 240x240, 7-pin or 8-pin without CS pin) |
| **Interconnects** | Dupont jumper wires (Female-to-Female or Breadboard jumpers) |
| **Cable** | USB Type-C data cable |

---

## 🔌 Wiring & Pinout Reference

> **Important Hardware Note:** Many common ST7789 modules do not expose a Chip Select (`CS`) pin because `CS` is permanently grounded internally on the PCB.

| LCD Pin Name | RP2040 Zero Pin | Function / Hardware Role | Notes |
| :--- | :--- | :--- | :--- |
| **GND** | **GND** | Ground | Common system ground |
| **VCC** | **3V3** or **5V** | Power Supply | If module has an onboard 662K 3.3V LDO regulator, connect to **5V**; otherwise connect to **3V3**. |
| **SCL / SCK** | **GP2** | Hardware SPI0 SCK | Clock line |
| **SDA / MOSI**| **GP3** | Hardware SPI0 TX | Data Out (Master Out, Slave In) |
| **DC / A0** | **GP4** | Data / Command Select | High = Data, Low = Command |
| **RES / RST** | **GP6** | Hardware Reset | Active-low display reset |
| **CS** | *N/A* | Chip Select | Leave disconnected (module has CS hardwired to GND). |
| **BL / BLK** | **3V3** or **GND** | Backlight LED | Most boards: connect to **3V3** (Active-HIGH). Some Chinese modules: connect to **GND** (Active-LOW) or leave floating. |

---

## 🏛 Software Architecture & Design Principles

This codebase strictly adheres to the **SOLID** design principles and clean architecture:

```text
               +-----------------------------+
               |     Config::AppConfig       |
               |  (Centralized parameters)   |
               +--------------+--------------+
                              |
+----------------------+      |      +-------------------------+
|     IDisplay         |      |      |  ICommunicationChannel  |
|  (Abstract Interface)|      |      |   (Abstract Interface)  |
+-----------^----------+      |      +------------^------------+
            |                 |                   |
       [Implements]           |              [Implements]
            |                 |                   |
+-----------+----------+      |      +------------+------------+
|    ST7789Display     |      |      |      SerialChannel      |
|  (TFT_eSPI Driver)   |      |      |     (Hardware Serial)   |
+----------------------+      |      +-------------------------+
            \                 |                  /
             \                v                 /
           +--\--------------------------------/--+
           |             Application              |
           |      (Pure Domain Business Logic)    |
           +------------------+-------------------+
                              |
                              v
                      [src/main.cpp]
                   (Composition Root)
```

### Key Principles Applied:
* **Single Responsibility Principle (SRP):**
  * `AppConfig.h`: System constants, geometry, palette, timing.
  * `Application`: Core state machine, uptime, message routing.
  * `ST7789Display`: Low-level pixel layout and drawing primitives.
  * `SerialChannel`: Raw stream reading and response formatting.
* **Open / Closed Principle (OCP):**
  * Add an OLED display (`SSD1306Display`) or a Bluetooth channel (`BLEChannel`) simply by implementing the interfaces. `Application.cpp` remains untouched.
* **Liskov Substitution Principle (LSP):**
  * Concrete drivers can be substituted anywhere their interface is expected without breaking behavior.
* **Interface Segregation Principle (ISP):**
  * Interfaces (`IDisplay`, `ICommunicationChannel`) are minimal, focused, and purposeful.
* **Dependency Inversion Principle (DIP):**
  * The application does not instantiate or directly reference `TFT_eSPI` or `Serial`. It receives abstract references via constructor injection in `src/main.cpp` (Composition Root).

### Directory Layout
```text
├── include/
│   ├── config/
│   │   └── AppConfig.h                # All system constants and UI coordinates
│   ├── interfaces/
│   │   ├── IDisplay.h                 # Abstract display interface
│   │   └── ICommunicationChannel.h    # Abstract communication channel
│   ├── drivers/
│   │   ├── ST7789Display.h            # TFT_eSPI driver wrapper
│   │   └── SerialChannel.h            # Serial communication wrapper
│   └── app/
│       └── Application.h              # Application business logic header
├── src/
│   ├── drivers/
│   │   ├── ST7789Display.cpp
│   │   └── SerialChannel.cpp
│   ├── app/
│   │   └── Application.cpp
│   └── main.cpp                       # Composition Root (Dependency Injection)
├── platformio.ini                     # Build flags, SPI pins, and library dependencies
└── README.md
```

---

## 💻 Prerequisites & Environment Setup

### 1. PlatformIO CLI or VS Code
* Install [PlatformIO IDE extension](https://platformio.org/install/ide?install=vscode) in VS Code, or install PlatformIO Core CLI:
  ```bash
  pip install platformio
  ```

### 2. Linux USB Permissions (udev rules)
If building on Linux, ensure your user account has access to the USB serial ports:
```bash
sudo usermod -a -G dialout $USER
# Optional: Install PlatformIO udev rules
curl -fsSL https://raw.githubusercontent.com/platformio/platformio-core/develop/platformio/assets/system/99-platformio-udev.rules | sudo tee /etc/udev/rules.d/99-platformio-udev.rules
sudo udevadm control --reload-rules
sudo udevadm trigger
```

---

## 🔨 Building & Uploading

### 1. Build the Project
Compile the firmware and verify dependencies:
```bash
pio run
```

### 2. Flash to RP2040 Zero
* If your RP2040 is already running Arduino firmware with USB CDC enabled, PlatformIO can reset it automatically:
  ```bash
  pio run -t upload
  ```
* **Manual Bootloader Mode (if auto-reset fails):**
  1. Hold down the **BOOT** button on the RP2040 Zero.
  2. Plug the USB cable into your computer (or press and release the **RESET** button while holding **BOOT**).
  3. Release the **BOOT** button. The board mounts as a USB drive (`RPI-RP2`).
  4. Run `pio run -t upload`.

---

## 💬 Serial Monitor & Usage

1. Open the PlatformIO Serial Monitor at **115200 baud**:
   ```bash
   pio device monitor
   ```
2. Type any text and press **Enter**.
3. **Behavior:**
   * The text will immediately be transmitted to the RP2040.
   * The display updates the central card with the new message and increments the message counter.
   * The RP2040 will echo an acknowledgment back over serial:
     ```text
     [ACK] Received (1): Hello World!
     ```

---

## ⚙️ Configuration & Customization

### Changing Display Layout or Colors
All layout coordinates, font scales, colors, and timing parameters are isolated in [`include/config/AppConfig.h`](include/config/AppConfig.h):
```cpp
namespace Config {
    namespace Display {
        constexpr uint32_t COLOR_HEADER   = TFT_NAVY;
        constexpr uint32_t COLOR_PAYLOAD  = TFT_GREEN;
        constexpr size_t   MAX_DISPLAY_CHARS = 16;
        // ...
    }
}
```

### Changing Pin Assignments
Pin assignments for `TFT_eSPI` are configured as compile-time macros in [`platformio.ini`](platformio.ini):
```ini
build_flags =
    -D USER_SETUP_LOADED=1
    -D ST7789_DRIVER=1
    -D TFT_WIDTH=240
    -D TFT_HEIGHT=240
    -D TFT_MOSI=3        ; Hardware SPI0 TX
    -D TFT_SCLK=2        ; Hardware SPI0 SCK
    -D TFT_DC=4          ; Data/Command pin
    -D TFT_RST=6         ; Reset pin
    ; TFT_CS omitted intentionally for CS-less modules
```

---

## ❓ Troubleshooting & Common Pitfalls

| Symptom | Cause | Solution |
| :--- | :--- | :--- |
| **Screen is completely dark / no light** | Backlight (`BLK`) not powered or inverted polarity | • Try connecting `BLK` to **3V3**.<br>• If still dark, connect `BLK` to **GND** (some boards are active-low).<br>• On some boards, disconnect `BLK` completely (has internal pull-up). |
| **Backlight is on, but screen is blank/white** | Voltage drop on onboard 3.3V LDO regulator | Many ST7789 modules contain an onboard 3.3V regulator (chip labeled `662K`). Connect the display's **VCC** pin to **5V** instead of **3V3**. |
| **Text colors look inverted (e.g. blue instead of red)** | Panel RGB/BGR order mismatch | Add `-D TFT_RGB_ORDER=TFT_BGR` (or `TFT_RGB`) to `build_flags` in `platformio.ini`. |
| **Display shows noise / random pixels on edges** | Wrong resolution defined | If your screen is 135x240 or 240x320, adjust `TFT_WIDTH` and `TFT_HEIGHT` in `platformio.ini` and `AppConfig.h`. |
| **Upload error: `No port found`** | RP2040 not in bootloader mode | Hold the **BOOT** button while plugging the USB-C cable in, then re-run `pio run -t upload`. |
