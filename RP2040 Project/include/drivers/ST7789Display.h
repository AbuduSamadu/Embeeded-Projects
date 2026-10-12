#pragma once

#include "interfaces/IDisplay.h"
#include "config/AppConfig.h"
#include <TFT_eSPI.h>

/**
 * @brief Concrete ST7789 display implementation using TFT_eSPI
 */
class ST7789Display : public IDisplay {
public:
    ST7789Display();

    bool init() override;
    void clear() override;
    void setHeader(const String& title) override;
    void showMessage(const String& label, const String& message) override;
    void updateStatus(uint32_t uptimeSec, uint32_t messageCount) override;

private:
    TFT_eSPI _tft;
    String truncateText(const String& text, size_t maxLen) const;
};
