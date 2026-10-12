#include <Arduino.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

String lastMessage = "Waiting for input...";
unsigned long lastUpdate = 0;
int counter = 0;

void drawHeader() {
  tft.fillRect(0, 0, 240, 50, TFT_NAVY);
  tft.setTextColor(TFT_WHITE, TFT_NAVY);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Serial Terminal", 120, 25, 4);
}

void drawMessage(const String& msg) {
  // Clear message area (middle box)
  tft.fillRect(10, 60, 220, 110, TFT_BLACK);
  tft.drawRoundRect(10, 60, 220, 110, 8, TFT_CYAN);

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextDatum(TC_DATUM);
  tft.drawString("Last Serial Msg:", 120, 70, 2);

  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  // Truncate if too long to prevent screen overflow
  String displayStr = msg.length() > 16 ? msg.substring(0, 13) + "..." : msg;
  tft.drawString(displayStr, 120, 120, 4);
}

void drawFooter() {
  tft.fillRect(0, 180, 240, 60, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
  tft.setTextDatum(MC_DATUM);
  String stat = "Uptime: " + String(counter) + "s";
  tft.drawString(stat, 120, 210, 2);
}

void setup() {
  Serial.begin(115200);

  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

  drawHeader();
  drawMessage(lastMessage);
  drawFooter();

  // Serial prompt banner
  Serial.println("\n==============================================");
  Serial.println("   RP2040 Zero + ST7789 Serial Terminal       ");
  Serial.println("==============================================");
  Serial.println("Type any message and press ENTER to display it!");
  Serial.println("----------------------------------------------");
}

void loop() {
  // Check for incoming serial input
  if (Serial.available() > 0) {
    String incoming = Serial.readStringUntil('\n');
    incoming.trim(); // Remove whitespace and \r

    if (incoming.length() > 0) {
      lastMessage = incoming;
      drawMessage(lastMessage);

      // Echo back confirmation to Serial Monitor
      Serial.print("[LCD Updated]: ");
      Serial.println(lastMessage);
    }
  }

  // Update uptime counter every 1 second
  if (millis() - lastUpdate >= 1000) {
    lastUpdate = millis();
    counter++;
    drawFooter();
  }
}