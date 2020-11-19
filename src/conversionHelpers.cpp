#include <Arduino.h>

int strToInt(char* str) {
    char** conversionErrorPos = 0;
    int convertedNumber = strtoul(str, conversionErrorPos, 10);
    Serial.printf("Converting \"%s\" to %d.\n", str, convertedNumber);
    if (conversionErrorPos == 0) {
        return convertedNumber;
    } else {
        Serial.print("Error converting char* to number: "); Serial.println(str);
    }
    return -1;
}