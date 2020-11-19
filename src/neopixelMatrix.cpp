#include <Adafruit_NeoPixel.h>

extern Adafruit_NeoPixel leds;
float pmMatrix[64] = {0.0f};
float vocMatrix[64] = {0.0f};
uint16_t pmValCount = 0;
uint16_t vocValCount = 0;

float normValue(float val, float minVal, float maxVal) {
    float normedVal = (val - minVal) / (maxVal - minVal);
    if (normedVal < 0.0f) normedVal = 0.0f;
    if (normedVal > 1.0f) normedVal = 1.0f;
    return normedVal;
}

bool sanityCheck(float val, float minVal, float maxVal) {
    if (val < minVal) return false;
    if (val > maxVal) return false;
    return true;
}

bool sanityCheck(uint16_t val, uint16_t minVal, uint16_t maxVal) {
    if (val < minVal) return false;
    if (val > maxVal) return false;
    return true;
}

float maxOf(float a, float b) {
    if (b > a) return b;
    return a;
}

void shiftPmMeasurements(float pm) {
    uint8_t i;
    for (i = 7*8+7; i >= 8; i--) {
        pmMatrix[i] = pmMatrix[i-8];
    }
    pmValCount = 1;
    for (i = 0; i < 8; i++) {
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

void shiftVocMeasurements(float voc) {
    uint8_t i;
    for (i = 7*8+7; i >= 8; i--) {
        vocMatrix[i] = vocMatrix[i-8];
    }
    vocValCount = 1;
    for (uint8_t i = 0; i < 8; i++) {
        if (round(voc * 8.0f) > i) {
            vocMatrix[i] = 1.0f;
        } else {
            vocMatrix[i] = 0.0f;
        }
    }
}

void addVocMeasurement(float voc) {
    vocValCount++;
    for (uint8_t i = 0; i < 8; i++) {
        float measFrac = 1.0f/((float) vocValCount);
        vocMatrix[i] = (1.0f-measFrac) * vocMatrix[i];
        if (round(voc * 8.0f) > i) vocMatrix[i] += measFrac;
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
            (uint8_t) round(pmMatrix[i]*(pmSeverity/8.0f)*255), 
            (uint8_t) round(pmMatrix[i]*(1.0f-pmSeverity/8.0f)*255), 
            (uint8_t) round(vocMatrix[i]*255)));
    }
    leds.show();
}