#pragma once
#include <Arduino.h>
#include "interfaces/IInputController.h"
#include "config/AppConfig.h"

class JoystickController : public IInputController {
public:
    JoystickController()
        : _centerX(512),
          _centerY(512),
          _lastSelectState(false),
          _lastSelectTime(0) {}

    void init() override {
        analogReadResolution(10); // Standard 10-bit range (0 - 1023)
        pinMode(Config::Pins::JOYSTICK_VRX, INPUT);
        pinMode(Config::Pins::JOYSTICK_VRY, INPUT);
        pinMode(Config::Pins::JOYSTICK_SW, INPUT_PULLUP);
        pinMode(Config::Pins::BTN_SELECT, INPUT_PULLUP);

        // Calibrate resting center position by averaging multiple samples
        long sumX = 0;
        long sumY = 0;
        constexpr int CALIB_SAMPLES = 8;
        for (int i = 0; i < CALIB_SAMPLES; ++i) {
            sumX += analogRead(Config::Pins::JOYSTICK_VRX);
            sumY += analogRead(Config::Pins::JOYSTICK_VRY);
            delay(5);
        }
        _centerX = static_cast<int16_t>(sumX / CALIB_SAMPLES);
        _centerY = static_cast<int16_t>(sumY / CALIB_SAMPLES);

        // Fallback protection in case stick was pushed off-center during boot
        if (_centerX < 200 || _centerX > 824) _centerX = 512;
        if (_centerY < 200 || _centerY > 824) _centerY = 512;
    }

    ButtonAction getAction() override {
        // --- 1. Select Button (GP7) & Optional Joystick Switch (GP8) ---
        bool selPressed = (digitalRead(Config::Pins::BTN_SELECT) == LOW);
        bool swPressed  = (Config::Joystick::ENABLE_SW && digitalRead(Config::Pins::JOYSTICK_SW) == LOW);
        bool anySelect  = selPressed || swPressed;

        unsigned long now = millis();
        // Hardware debounce & clean edge detection for single trigger
        if (anySelect && !_lastSelectState && (now - _lastSelectTime > 150)) {
            _lastSelectState = true;
            _lastSelectTime = now;
            return ButtonAction::SELECT;
        } else if (!anySelect && (now - _lastSelectTime > 50)) {
            _lastSelectState = false;
        }

        // --- 2. Analog Joystick Reading (GP26 / GP27) ---
        int16_t rawX = analogRead(Config::Pins::JOYSTICK_VRX);
        int16_t rawY = analogRead(Config::Pins::JOYSTICK_VRY);

        int16_t dx = rawX - _centerX;
        int16_t dy = rawY - _centerY;

        if (Config::Joystick::INVERT_X) dx = -dx;
        if (Config::Joystick::INVERT_Y) dy = -dy;

        if (Config::Joystick::SWAP_AXES) {
            int16_t temp = dx;
            dx = dy;
            dy = temp;
        }

        int16_t absX = abs(dx);
        int16_t absY = abs(dy);

        // Dominant axis detection with deadzone filter
        if (absX >= Config::Joystick::DEADZONE || absY >= Config::Joystick::DEADZONE) {
            if (absX >= absY) {
                return (dx > 0) ? ButtonAction::RIGHT : ButtonAction::LEFT;
            } else {
                // Negative displacement represents forward/up push on standard joysticks
                return (dy < 0) ? ButtonAction::UP : ButtonAction::DOWN;
            }
        }

        return ButtonAction::NONE;
    }

    int16_t getCenterX() const { return _centerX; }
    int16_t getCenterY() const { return _centerY; }

private:
    int16_t _centerX;
    int16_t _centerY;
    bool _lastSelectState;
    unsigned long _lastSelectTime;
};
