#include <Arduino.h>
#include <TFT_eSPI.h>
#include "drivers/JoystickController.h"
#include "drivers/BuzzerController.h"
#include "app/SnakeGame.h"

// =============================================================================
// Composition Root (Dependency Injection)
// =============================================================================

static TFT_eSPI tft;
static JoystickController joystickController;
static BuzzerController buzzerController;
static SnakeGame game(tft, joystickController, buzzerController);

void setup() {
    Serial.begin(Config::SerialComm::BAUD_RATE);
    delay(Config::SerialComm::BOOT_DELAY_MS);
    game.init();
}

void loop() {
    game.tick();
}