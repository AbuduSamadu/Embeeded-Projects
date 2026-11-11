#include <Arduino.h>
#include "SystemController.h"

// Instantiate the master system controller
static SystemController systemController;

void setup() {
    systemController.setup();
}

void loop() {
    systemController.update();
}
