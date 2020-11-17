#include <Adafruit_NeoPixel.h>

extern Adafruit_NeoPixel leds;
float pmMatrix[64] = {0.0f};
float co2Matrix[64] = {0.0f};
uint16_t pmValCount = 0;
uint16_t co2ValCount = 0;

void shiftPmMeasurements(float pm) {
    for (uint8_t i = 55; i >= 0; i--) {
        pmMatrix[i+8] = pmMatrix[i];
    }
    pmValCount = 1;
    for (uint8_t i = 0; i < 8; i++) {
        if (round(pm * 8.0f) > i) {
            pmMatrix[i] = 1.0f;
        } else {
            pmMatrix[i] = 0.0f;
        }
    }
}

void addPmMeasurement(float pm) {
    pmValCount++;
    for (uint8_t i = 0; i < 8; i++) {
        float measFrac = 1.0f/((float) pmValCount);
        pmMatrix[i] = (1.0f-measFrac) * pmMatrix[i];
        if (round(pm * 8.0f) > i) pmMatrix[i] += measFrac;
    }
}

void shiftCO2Measurements(float co2) {
    for (uint8_t i = 55; i >= 0; i--) {
        co2Matrix[i+8] = co2Matrix[i];
    }
    co2ValCount = 1;
    for (uint8_t i = 0; i < 8; i++) {
        if (round(co2 * 8.0f) > i) {
            co2Matrix[i] = 1.0f;
        } else {
            co2Matrix[i] = 0.0f;
        }
    }
}

void addCO2Measurement(float co2) {
    co2ValCount++;
    for (uint8_t i = 0; i < 8; i++) {
        float measFrac = 1.0f/((float) co2ValCount);
        co2Matrix[i] = (1.0f-measFrac) * co2Matrix[i];
        if (round(co2 * 8.0f) > i) co2Matrix[i] += measFrac;
    }
}

void displayMatrix() {
    uint8_t pmSeverity = 0;
    for (uint8_t i = 0; i < 8; i++) {
        if (pmMatrix[i] > 0.01f) {
            pmSeverity = i;
        } else {
            break;
        }
    }
    for (uint8_t i = 0; i < 64; i++) {
        leds.setPixelColor(i, leds.Color(
            (uint8_t) round(pmMatrix[i]*(1.0f-pmSeverity/8.0f)*255), 
            (uint8_t) round(pmMatrix[i]*(pmSeverity/8.0f)*255), 
            (uint8_t) round(co2Matrix[i]*255)));
    }
    leds.show();
}