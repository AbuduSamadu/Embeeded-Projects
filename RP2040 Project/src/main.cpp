#include <Arduino.h>
#include "drivers/ST7789Display.h"
#include "drivers/SerialChannel.h"
#include "app/Application.h"

// =============================================================================
// Composition Root (Dependency Injection)
// Instantiates concrete hardware adapters and injects them into application core.
// =============================================================================

static ST7789Display hardwareDisplay;
static SerialChannel serialChannel;

// Core business logic depends ONLY on abstractions
static Application app(hardwareDisplay, serialChannel);

void setup() {
    app.initialize();
}

void loop() {
    app.tick();
}