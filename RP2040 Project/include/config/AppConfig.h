#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

namespace Config {

    namespace Pins {
        // --- Joystick Pins ---
        constexpr uint8_t JOYSTICK_VRX = 26; // Analog X-axis (GP26 / ADC0)
        constexpr uint8_t JOYSTICK_VRY = 27; // Analog Y-axis (GP27 / ADC1)
        constexpr uint8_t JOYSTICK_SW  = 8;  // Optional push switch (GP8, active LOW)

        // --- Push Buttons ---
        constexpr uint8_t BTN_SELECT   = 7;  // Primary Select button (GP7, active LOW)

        // --- Buzzer Pin ---
        constexpr uint8_t BUZZER       = 15; // Audio buzzer positive (+) pin (GP15)

        // --- Legacy 4-Button D-pad (preserved for reference/backwards-compatibility) ---
        constexpr uint8_t BTN_UP       = 7;
        constexpr uint8_t BTN_DOWN     = 8;
        constexpr uint8_t BTN_LEFT     = 9;
        constexpr uint8_t BTN_RIGHT    = 10;
    }

    namespace Joystick {
        constexpr int16_t DEADZONE    = 180;   // Deadband threshold around resting center (0-512)
        constexpr bool    INVERT_X    = false; // Set to true if Left / Right directions are flipped
        constexpr bool    INVERT_Y    = false; // Set to true if Up / Down directions are flipped
        constexpr bool    SWAP_AXES   = false; // Set to true if joystick is mounted rotated 90 degrees
        constexpr bool    ENABLE_SW   = true;  // Treat joystick thumb click (SW) as secondary SELECT
    }

    namespace BuzzerConfig {
        constexpr bool    ENABLED     = true;  // Master audio toggle
    }

    namespace SerialComm {
        constexpr uint32_t BAUD_RATE = 115200;
        constexpr uint32_t BOOT_DELAY_MS = 1000;
    }

    namespace Timing {
        constexpr uint32_t HEARTBEAT_INTERVAL_MS = 1000;
    }

    namespace Display {
        constexpr uint16_t WIDTH  = 240;
        constexpr uint16_t HEIGHT = 240;
        constexpr uint8_t  ROTATION = 0;

        constexpr uint32_t COLOR_BG       = TFT_BLACK;
        constexpr uint32_t COLOR_HEADER   = TFT_NAVY;
        constexpr uint32_t COLOR_TITLE    = TFT_WHITE;
        constexpr uint32_t COLOR_LABEL    = TFT_YELLOW;
        constexpr uint32_t COLOR_CARD_BORDER = TFT_CYAN;
        constexpr uint32_t COLOR_CARD_BG  = TFT_BLACK;
        constexpr uint32_t COLOR_PAYLOAD  = TFT_GREEN;
        constexpr uint32_t COLOR_FOOTER_BG = TFT_DARKGREY;
        constexpr uint32_t COLOR_STATUS   = TFT_WHITE;

        constexpr int16_t HEADER_HEIGHT = 48;
        constexpr int16_t FOOTER_HEIGHT = 44;
        constexpr int16_t CARD_MARGIN_X = 10;
        constexpr int16_t CARD_MARGIN_Y = 56;
        constexpr int16_t CARD_WIDTH    = WIDTH - (CARD_MARGIN_X * 2);
        constexpr int16_t CARD_HEIGHT   = HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT - 16;
        constexpr int16_t CARD_RADIUS   = 8;

        constexpr size_t MAX_DISPLAY_CHARS = 16;
    }

    namespace Game {
        constexpr uint16_t SCREEN_W   = 240;
        constexpr uint16_t SCREEN_H   = 240;
        constexpr uint16_t HEADER_H   = 32; // Generous height for crisp typography

        constexpr uint8_t  CELL_SIZE  = 13; // 13x13 grid cells
        constexpr uint8_t  GRID_W     = 18; // 18 columns = 234px (centered with 3px margins)
        constexpr uint8_t  GRID_H     = 16; // 16 rows = 208px
        constexpr uint16_t OFFSET_X   = (SCREEN_W - (GRID_W * CELL_SIZE)) / 2;
        constexpr uint16_t OFFSET_Y   = HEADER_H;
        constexpr uint16_t MAX_LENGTH = GRID_W * GRID_H;

        constexpr uint32_t INITIAL_SPEED_MS = 160;
        constexpr uint32_t MIN_SPEED_MS     = 60;
        constexpr uint32_t SPEED_STEP_MS    = 15;
        constexpr uint8_t  FOOD_PER_LEVEL   = 5;
    }

    namespace Colors {
        // High-contrast arcade CRT / Cyberpunk palette
        constexpr uint32_t BG          = 0x0821; // Midnight Obsidian
        constexpr uint32_t GRID_BG     = 0x0000; // Pitch Black
        constexpr uint32_t HEADER_BG   = 0x10A2; // Deep Steel Blue
        constexpr uint32_t HEADER_LINE = 0x07FF; // Cyan Neon Divider
        constexpr uint32_t TEXT_WHITE  = 0xFFFF; // Pure White
        constexpr uint32_t ACCENT_CYAN = 0x07FF; // Electric Cyan
        constexpr uint32_t GOLD_YELLOW = 0xFFE0; // Arcade Gold
        constexpr uint32_t SNAKE_HEAD  = 0x07E0; // Neon Green
        constexpr uint32_t SNAKE_BODY  = 0x05E0; // Vivid Emerald
        constexpr uint32_t SNAKE_EYE   = 0x0000; // Black pupil
        constexpr uint32_t FOOD_APPLE  = 0xF800; // Candy Red
        constexpr uint32_t FOOD_STEM   = 0x07E0; // Leaf Green
        constexpr uint32_t MODAL_BG    = 0x0002; // Dark Navy Glass
        constexpr uint32_t MODAL_EDGE  = 0x07FF; // Neon Cyan Border
        constexpr uint32_t DANGER_EDGE = 0xF800; // Neon Red
    }
}
