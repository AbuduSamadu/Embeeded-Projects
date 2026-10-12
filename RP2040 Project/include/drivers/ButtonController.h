#pragma once
#include <Arduino.h>
#include "interfaces/IInputController.h"
#include "config/AppConfig.h"

class ButtonController : public IInputController {
public:
    void init() override {
        pinMode(Config::Pins::BTN_UP, INPUT_PULLUP);
        pinMode(Config::Pins::BTN_DOWN, INPUT_PULLUP);
        pinMode(Config::Pins::BTN_LEFT, INPUT_PULLUP);
        pinMode(Config::Pins::BTN_RIGHT, INPUT_PULLUP);
        pinMode(Config::Pins::BTN_SELECT, INPUT_PULLUP);
    }

    ButtonAction getAction() override {
        // Debounce / single-press detection for SELECT
        bool upPressed    = (digitalRead(Config::Pins::BTN_UP) == LOW);
        bool downPressed  = (digitalRead(Config::Pins::BTN_DOWN) == LOW);
        bool leftPressed  = (digitalRead(Config::Pins::BTN_LEFT) == LOW);
        bool rightPressed = (digitalRead(Config::Pins::BTN_RIGHT) == LOW);
        bool selPressed   = (digitalRead(Config::Pins::BTN_SELECT) == LOW);

        // Edge detection for SELECT to toggle pause cleanly
        if (selPressed && !_lastSelectState) {
            _lastSelectState = true;
            return ButtonAction::SELECT;
        } else if (!selPressed) {
            _lastSelectState = false;
        }

        // Direction buttons
        if (upPressed)    return ButtonAction::UP;
        if (downPressed)  return ButtonAction::DOWN;
        if (leftPressed)  return ButtonAction::LEFT;
        if (rightPressed) return ButtonAction::RIGHT;

        return ButtonAction::NONE;
    }

private:
    bool _lastSelectState = false;
};
