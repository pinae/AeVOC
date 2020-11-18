#include <Arduino.h>

void drawBarDiagram(int16_t y, float value, float minVal, float maxVal);
void displayMeasurements(float pm25, float pm10, 
        uint16_t co2, uint16_t voc, 
        float temperature, float humidity);