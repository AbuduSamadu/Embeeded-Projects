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
          _lastSelectTime(0),
          _lastDebugTime(0),
          _lastDebugAction(ButtonAction::NONE),
          _lastWasActive(false) {}

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
        int16_t measuredX = static_cast<int16_t>(sumX / CALIB_SAMPLES);
        int16_t measuredY = static_cast<int16_t>(sumY / CALIB_SAMPLES);

        // Fallback protection in case stick was pushed off-center during boot
        bool fallbackX = (measuredX < 200 || measuredX > 824);
        bool fallbackY = (measuredY < 200 || measuredY > 824);
        _centerX = fallbackX ? 512 : measuredX;
        _centerY = fallbackY ? 512 : measuredY;

        if (Config::Joystick::DEBUG_SERIAL) {
            printStartupBanner(measuredX, measuredY, fallbackX || fallbackY);
        }
    }

    ButtonAction getAction() override {
        // --- 1. Select Button (GP7) & Optional Joystick Switch (GP8) ---
        bool selPressed = (digitalRead(Config::Pins::BTN_SELECT) == LOW);
        bool swPressed  = (Config::Joystick::ENABLE_SW && digitalRead(Config::Pins::JOYSTICK_SW) == LOW);
        bool anySelect  = selPressed || swPressed;

        unsigned long now = millis();
        bool selectTriggered = false;

        // Hardware debounce & clean edge detection for single trigger
        if (anySelect && !_lastSelectState && (now - _lastSelectTime > 150)) {
            _lastSelectState = true;
            _lastSelectTime = now;
            selectTriggered = true;
        } else if (!anySelect && (now - _lastSelectTime > 50)) {
            _lastSelectState = false;
        }

        // --- 2. Analog Joystick Reading (GP26 / GP27) ---
        int16_t rawX = analogRead(Config::Pins::JOYSTICK_VRX);
        int16_t rawY = analogRead(Config::Pins::JOYSTICK_VRY);
        int16_t raw28 = analogRead(28);

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

        ButtonAction detectedAction = ButtonAction::NONE;

        if (selectTriggered) {
            detectedAction = ButtonAction::SELECT;
        } else {
            // Schmitt-trigger hysteresis: higher threshold to enter active, lower to exit
            int16_t threshold = (_lastAction != ButtonAction::NONE && _lastAction != ButtonAction::SELECT)
                                ? (Config::Joystick::DEADZONE - 35)
                                : Config::Joystick::DEADZONE;

            if (absX >= threshold || absY >= threshold) {
                // Determine whether to prioritize horizontal (X) or vertical (Y).
                // Use directional axis hysteresis: if already moving along an axis (e.g. DOWN),
                // require the perpendicular axis (X) to exceed it by AXIS_HYSTERESIS before switching.
                // This prevents electrical crosstalk, over-voltage leakage, or diagonal thumb tilt
                // from spuriously triggering RIGHT while holding DOWN.
                bool preferHorizontal = false;
                if (_lastAction == ButtonAction::LEFT || _lastAction == ButtonAction::RIGHT) {
                    preferHorizontal = (absY <= absX + Config::Joystick::AXIS_HYSTERESIS);
                } else if (_lastAction == ButtonAction::UP || _lastAction == ButtonAction::DOWN) {
                    preferHorizontal = (absX > absY + Config::Joystick::AXIS_HYSTERESIS);
                } else {
                    // From rest (NONE): dominant displacement wins (strict > prevents arbitrary tie favor)
                    preferHorizontal = (absX > absY);
                }

                if (preferHorizontal) {
                    detectedAction = (dx > 0) ? ButtonAction::RIGHT : ButtonAction::LEFT;
                } else {
                    // Negative displacement represents forward/up push on standard joysticks
                    detectedAction = (dy < 0) ? ButtonAction::UP : ButtonAction::DOWN;
                }
            }
        }
        _lastAction = detectedAction;

        // --- 3. Serial Monitor Real-Time Debugging ---
        if (Config::Joystick::DEBUG_SERIAL) {
            printDebug(now, rawX, rawY, raw28, dx, dy, absX, absY, selPressed, swPressed, detectedAction);
        }

        return detectedAction;
    }

    int16_t getCenterX() const { return _centerX; }
    int16_t getCenterY() const { return _centerY; }

    static const char* actionToString(ButtonAction action) {
        switch (action) {
            case ButtonAction::NONE:   return "NONE";
            case ButtonAction::UP:     return "UP";
            case ButtonAction::DOWN:   return "DOWN";
            case ButtonAction::LEFT:   return "LEFT";
            case ButtonAction::RIGHT:  return "RIGHT";
            case ButtonAction::SELECT: return "SELECT";
            default:                   return "UNKNOWN";
        }
    }

private:
    void printStartupBanner(int16_t measuredX, int16_t measuredY, bool usedFallback) const {
        Serial.println();
        Serial.println(F("============================================================"));
        Serial.println(F("            JOYSTICK SERIAL MONITOR DEBUG & TEST            "));
        Serial.println(F("============================================================"));
        Serial.print(F(" Calibrated Rest Center : X = "));
        Serial.print(_centerX);
        Serial.print(F(", Y = "));
        Serial.print(_centerY);
        if (usedFallback) {
            Serial.print(F(" [Note: Boot read was X="));
            Serial.print(measuredX);
            Serial.print(F(", Y="));
            Serial.print(measuredY);
            Serial.print(F("; defaulted to 512]"));
        }
        Serial.println();
        if (_centerX > 650 || _centerY > 650) {
            Serial.println(F(" [!] HARDWARE WARNING: Resting center is ~770 (2.5V)!"));
            Serial.println(F("     Your joystick VCC is likely wired to 5V (VBUS)."));
            Serial.println(F("     RP2040 ADC is 3.3V. Pushing DOWN/RIGHT sends 5V into"));
            Serial.println(F("     the chip, saturating ADC and leaking into adjacent pins!"));
            Serial.println(F("     --> FIX: Move Joystick VCC wire from 5V to 3.3V (3V3)."));
        }
        Serial.print(F(" Deadzone Threshold     : "));
        Serial.println(Config::Joystick::DEADZONE);
        Serial.print(F(" Pins Assigned          : VRX=GP"));
        Serial.print(Config::Pins::JOYSTICK_VRX);
        Serial.print(F(", VRY=GP"));
        Serial.print(Config::Pins::JOYSTICK_VRY);
        Serial.print(F(", SW=GP"));
        Serial.print(Config::Pins::JOYSTICK_SW);
        Serial.print(F(", BTN_SELECT=GP"));
        Serial.println(Config::Pins::BTN_SELECT);
        Serial.print(F(" Active Config          : INVERT_X="));
        Serial.print(Config::Joystick::INVERT_X ? "true" : "false");
        Serial.print(F(", INVERT_Y="));
        Serial.print(Config::Joystick::INVERT_Y ? "true" : "false");
        Serial.print(F(", SWAP_AXES="));
        Serial.println(Config::Joystick::SWAP_AXES ? "true" : "false");
        Serial.println(F("------------------------------------------------------------"));
        Serial.println(F(" DIRECTION FIX CHEAT SHEET:"));
        Serial.println(F(" - Push UP physically:"));
        Serial.println(F("     Detected DOWN        --> Set INVERT_Y = true in AppConfig.h"));
        Serial.println(F("     Detected LEFT/RIGHT  --> Set SWAP_AXES = true in AppConfig.h"));
        Serial.println(F(" - Push RIGHT physically:"));
        Serial.println(F("     Detected LEFT        --> Set INVERT_X = true in AppConfig.h"));
        Serial.println(F("     Detected UP/DOWN     --> Set SWAP_AXES = true in AppConfig.h"));
        Serial.println(F(" - Click Thumbstick (SW) or GP7:"));
        Serial.println(F("     Should detect SELECT"));
        Serial.println(F("============================================================"));
        Serial.println();
    }

    void printDebug(unsigned long now, int16_t rawX, int16_t rawY, int16_t raw28,
                    int16_t dx, int16_t dy, int16_t absX, int16_t absY,
                    bool selPressed, bool swPressed, ButtonAction action) {
        bool isActive = (action != ButtonAction::NONE);
        bool inDeadzoneBand = (!isActive && (absX > 60 || absY > 60));
        bool actionChanged = (action != _lastDebugAction);

        if (action == ButtonAction::SELECT) {
            Serial.print(F("[JOYSTICK BUTTON] SELECT Triggered | GP8_SW: "));
            Serial.print(swPressed ? F("PRESSED") : F("RELEASED"));
            Serial.print(F(", GP7_SEL: "));
            Serial.println(selPressed ? F("PRESSED") : F("RELEASED"));
            _lastDebugTime = now;
            _lastDebugAction = action;
            _lastWasActive = true;
            return;
        }

        if (isActive) {
            if (actionChanged || (now - _lastDebugTime >= Config::Joystick::DEBUG_INTERVAL_MS)) {
                _lastDebugTime = now;
                _lastDebugAction = action;
                _lastWasActive = true;

                Serial.print(F("[JOYSTICK ACTIVE] GP26="));
                Serial.print(rawX);
                Serial.print(F(", GP27="));
                Serial.print(rawY);
                Serial.print(F(", GP28="));
                Serial.print(raw28);
                Serial.print(F(" | dX="));
                if (dx >= 0) Serial.print('+');
                Serial.print(dx);
                Serial.print(F(", dY="));
                if (dy >= 0) Serial.print('+');
                Serial.print(dy);
                Serial.print(F(" | Action: "));
                Serial.println(actionToString(action));
            }
        } else if (_lastWasActive) {
            if (now - _lastDebugTime >= Config::Joystick::DEBUG_INTERVAL_MS) {
                _lastWasActive = false;
                _lastDebugAction = ButtonAction::NONE;
                _lastDebugTime = now;

                Serial.print(F("[JOYSTICK] -> Released (NONE) | GP26="));
                Serial.print(rawX);
                Serial.print(F(", GP27="));
                Serial.print(rawY);
                Serial.print(F(", GP28="));
                Serial.println(raw28);
            }
        } else if (inDeadzoneBand && (now - _lastDebugTime >= Config::Joystick::DEBUG_INTERVAL_MS)) {
            _lastDebugTime = now;
            Serial.print(F("[JOYSTICK DEADZONE] GP26="));
            Serial.print(rawX);
            Serial.print(F(", GP27="));
            Serial.print(rawY);
            Serial.print(F(", GP28="));
            Serial.print(raw28);
            Serial.print(F(" | dX="));
            if (dx >= 0) Serial.print('+');
            Serial.print(dx);
            Serial.print(F(", dY="));
            if (dy >= 0) Serial.print('+');
            Serial.print(dy);
            Serial.print(F(" | (Within deadzone < "));
            Serial.print(Config::Joystick::DEADZONE);
            Serial.println(F(") -> Action: NONE"));
        } else if (now - _lastDebugTime >= Config::Joystick::DEBUG_IDLE_MS) {
            _lastDebugTime = now;
            Serial.print(F("[JOYSTICK IDLE] Center: ("));
            Serial.print(_centerX);
            Serial.print(F(", "));
            Serial.print(_centerY);
            Serial.print(F(") | GP26="));
            Serial.print(rawX);
            Serial.print(F(", GP27="));
            Serial.print(rawY);
            Serial.print(F(", GP28="));
            Serial.print(raw28);
            Serial.println(F(" | Action: NONE"));
        }
    }

    int16_t _centerX;
    int16_t _centerY;
    bool _lastSelectState;
    unsigned long _lastSelectTime;

    unsigned long _lastDebugTime;
    ButtonAction  _lastDebugAction;
    bool          _lastWasActive;
    ButtonAction  _lastAction = ButtonAction::NONE;
};
