#include <Arduino.h>
#include <Wire.h>

void scanI2C() {
  Serial.println("Scanning for i2c devices...");
  for (byte address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address<16) Serial.print("0");
      Serial.print(address, HEX);
      Serial.println(" !");
    } else if (error==4) {
      Serial.print("Unknow error at address 0x");
      if (address<16) Serial.print("0");
      Serial.println(address, HEX);
    }    
  }
}