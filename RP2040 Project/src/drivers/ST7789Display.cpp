#include "drivers/ST7789Display.h"

ST7789Display::ST7789Display() : _tft() {}

bool ST7789Display::init() {
    _tft.init();
    _tft.setRotation(Config::Display::ROTATION);
    clear();
    return true;
}

void ST7789Display::clear() {
    _tft.fillScreen(Config::Display::COLOR_BG);
}

void ST7789Display::setHeader(const String& title) {
    _tft.fillRect(0, 0, Config::Display::WIDTH, Config::Display::HEADER_HEIGHT, Config::Display::COLOR_HEADER);
    _tft.setTextColor(Config::Display::COLOR_TITLE, Config::Display::COLOR_HEADER);
    _tft.setTextDatum(MC_DATUM);
    _tft.drawString(title, Config::Display::WIDTH / 2, Config::Display::HEADER_HEIGHT / 2, 4);
}

void ST7789Display::showMessage(const String& label, const String& message) {
    using namespace Config::Display;

    // Clear card area
    _tft.fillRoundRect(CARD_MARGIN_X, CARD_MARGIN_Y, CARD_WIDTH, CARD_HEIGHT, CARD_RADIUS, COLOR_CARD_BG);
    _tft.drawRoundRect(CARD_MARGIN_X, CARD_MARGIN_Y, CARD_WIDTH, CARD_HEIGHT, CARD_RADIUS, COLOR_CARD_BORDER);

    // Render section label
    _tft.setTextColor(COLOR_LABEL, COLOR_CARD_BG);
    _tft.setTextDatum(TC_DATUM);
    _tft.drawString(label, WIDTH / 2, CARD_MARGIN_Y + 10, 2);

    // Render payload text
    _tft.setTextColor(COLOR_PAYLOAD, COLOR_CARD_BG);
    _tft.setTextDatum(MC_DATUM);
    String truncated = truncateText(message, MAX_DISPLAY_CHARS);
    _tft.drawString(truncated, WIDTH / 2, CARD_MARGIN_Y + (CARD_HEIGHT / 2) + 10, 4);
}

void ST7789Display::updateStatus(uint32_t uptimeSec, uint32_t messageCount) {
    using namespace Config::Display;

    int16_t footerY = HEIGHT - FOOTER_HEIGHT;
    _tft.fillRect(0, footerY, WIDTH, FOOTER_HEIGHT, COLOR_FOOTER_BG);
    _tft.setTextColor(COLOR_STATUS, COLOR_FOOTER_BG);
    _tft.setTextDatum(MC_DATUM);

    String statusText = "Up: " + String(uptimeSec) + "s | Msg: " + String(messageCount);
    _tft.drawString(statusText, WIDTH / 2, footerY + (FOOTER_HEIGHT / 2), 2);
}

String ST7789Display::truncateText(const String& text, size_t maxLen) const {
    if (text.length() <= maxLen) {
        return text;
    }
    return text.substring(0, maxLen - 3) + "...";
}
