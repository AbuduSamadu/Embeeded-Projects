#include "drivers/SerialChannel.h"

SerialChannel::SerialChannel() {}

bool SerialChannel::init() {
    Serial.begin(Config::SerialComm::BAUD_RATE);
    delay(Config::SerialComm::BOOT_DELAY_MS);

    Serial.println("\n==============================================");
    Serial.println("  Decoupled Architecture - RP2040 ST7789 Node ");
    Serial.println("==============================================");
    Serial.println("Send any text to update display...");
    Serial.println("----------------------------------------------");
    return true;
}

bool SerialChannel::hasMessage() {
    return Serial.available() > 0;
}

String SerialChannel::readMessage() {
    String message = Serial.readStringUntil('\n');
    message.trim();
    return message;
}

void SerialChannel::sendResponse(const String& response) {
    Serial.println(response);
}
