#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

namespace Config {

    // Serial Communication Configuration
    namespace SerialComm {
        constexpr uint32_t BAUD_RATE = 115200;
        constexpr uint32_t BOOT_DELAY_MS = 1000;
    }

    // Display & UI Configuration
    namespace Display {
        constexpr uint16_t WIDTH  = 240;
        constexpr uint16_t HEIGHT = 240;
        constexpr uint8_t  ROTATION = 0;

        // Theme Palette
        constexpr uint32_t COLOR_BG       = TFT_BLACK;
        constexpr uint32_t COLOR_HEADER   = TFT_NAVY;
        constexpr uint32_t COLOR_TITLE    = TFT_WHITE;
        constexpr uint32_t COLOR_LABEL    = TFT_YELLOW;
        constexpr uint32_t COLOR_CARD_BORDER = TFT_CYAN;
        constexpr uint32_t COLOR_CARD_BG  = TFT_BLACK;
        constexpr uint32_t COLOR_PAYLOAD  = TFT_GREEN;
        constexpr uint32_t COLOR_FOOTER_BG = TFT_DARKGREY;
        constexpr uint32_t COLOR_STATUS   = TFT_WHITE;

        // Layout Geometry
        constexpr int16_t HEADER_HEIGHT = 48;
        constexpr int16_t FOOTER_HEIGHT = 44;
        constexpr int16_t CARD_MARGIN_X = 10;
        constexpr int16_t CARD_MARGIN_Y = 56;
        constexpr int16_t CARD_WIDTH    = WIDTH - (CARD_MARGIN_X * 2);
        constexpr int16_t CARD_HEIGHT   = HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT - 16;
        constexpr int16_t CARD_RADIUS   = 8;

        constexpr size_t MAX_DISPLAY_CHARS = 16;
    }

    // Application Timing Intervals
    namespace Timing {
        constexpr uint32_t HEARTBEAT_INTERVAL_MS = 1000;
    }

} // namespace Config
