#include <Arduino.h>

void setup() {
  // Initialize the built-in LED pin as an output.
  // Note: On ESP8266 NodeMCU, the built-in LED is typically active-low.
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Active LOW)
  delay(1000);                      // Wait for 1 second
  digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off
  delay(1000);                      // Wait for 1 second
}