#include <Arduino.h>
#include <TFT_eSPI.h>
#include "drivers/ButtonController.h"
#include "app/SnakeGame.h"

// =============================================================================
// Composition Root (Dependency Injection)
// =============================================================================

static TFT_eSPI tft;
static ButtonController buttonController;
static SnakeGame game(tft, buttonController);

void setup() {
    Serial.begin(115200);
    game.init();
}

void loop() {
    game.tick();
}