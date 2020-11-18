#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

extern Adafruit_SSD1306 display;

void drawBarDiagram(int16_t y, float value, float minVal, float maxVal) {
  display.drawRect(73, y, 128-73, 7, WHITE);
  display.fillRect(73, y, 
    (int) round((value-minVal)/(maxVal-minVal)*(128-73)), 
    7, WHITE);
}

void displayMeasurements(float pm25, float pm10, 
        uint16_t co2, uint16_t voc, 
        float temperature, float humidity) {
  display.clearDisplay();
  display.setTextSize(1); 
  display.setTextColor(WHITE);
  display.setCursor(0, 16);
  display.cp437(true);
  display.printf("PM2,5: %2.2f\n", pm25);
  drawBarDiagram(16, pm25, 4.0f, 25.0f);
  display.printf("PM10: %2.2f\n", pm10);
  drawBarDiagram(24, pm10, 7.0f, 50.0f);
  display.printf("CO2: %u ppm\n", co2);
  drawBarDiagram(32, (float) co2, 400.0f, 1500.0f);
  display.printf("VOC: %u\n", voc);
  drawBarDiagram(38, (float) voc, 0.0f, 2000.0f);
  display.printf("%2.1f\n", temperature);
  display.drawCircle(26, 49, 1, WHITE);
  display.drawChar(29, 48, 'C', WHITE, BLACK, 1);
  drawBarDiagram(46, temperature, 10.0f, 28.0f);
  display.drawLine(3, 56, 0, 59, WHITE);
  display.drawLine(0, 59, 0, 61, WHITE);
  display.drawLine(0, 61, 2, 63, WHITE);
  display.drawLine(2, 63, 4, 63, WHITE);
  display.drawLine(4, 63, 6, 61, WHITE);
  display.drawLine(6, 61, 6, 59, WHITE);
  display.drawLine(6, 59, 3, 56, WHITE);
  display.printf("  %2.0f%%", humidity);
  drawBarDiagram(54, humidity, 40.0f, 68.0f);
  display.setTextSize(2);
  display.setCursor(0, 0);
  String topText = "Alles OK.";
  if (humidity > 67) topText = "Schimmel!";
  if (voc > 800) topText = "Gestank!";
  if (co2 > 1000) topText = "CO2: zzz..";
  if (co2 > 1500) topText = "Zuviel CO2";
  if (pm25 > 10.0f || pm10 > 20.0f) topText = "Aerosole!";
  display.println(topText);
  display.display();
}