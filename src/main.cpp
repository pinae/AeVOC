#include <Arduino.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <SdsDustSensor.h>
#include <SparkFunCCS811.h>

#define LED D0
#define PM_SERIAL_RX D5
#define PM_SERIAL_TX D6
#define I2C_PIN_SCL D3
#define I2C_PIN_SDA D4
#define CCS811_ADDR 0x5A

SdsDustSensor sds(PM_SERIAL_RX, PM_SERIAL_TX);
CCS811 ccs(CCS811_ADDR);

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  while(!Serial) {}  // Wait for Serial to start
  delay(800);
  Serial.println("Initializing SDS011 particle sensor...");
  sds.begin();
  Serial.println(sds.queryFirmwareVersion().toString()); // prints firmware version
  Serial.println(sds.setActiveReportingMode().toString()); // ensures sensor is in 'active' reporting mode
  Serial.println(sds.setContinuousWorkingPeriod().toString()); // ensures sensor has continuous working period - default but not recommended
  Serial.println("Initializing i2c...");
  Wire.begin(I2C_PIN_SDA, I2C_PIN_SCL);
  Wire.setClockStretchLimit(500);
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
  Serial.println("Initializing CCS811 VOC air quality and CO2 sensor...");
  CCS811Core::CCS811_Status_e returnCode = ccs.beginWithStatus(Wire);
  Serial.print("CCS811.begin() exited with: ");
  Serial.println(ccs.statusString(returnCode));
  Serial.println("Setup complete.");
}

void loop() {
  Serial.println("Starting loop...");
  PmResult pm = sds.readPm();
  if (pm.isOk()) {
    Serial.print("PM2.5 = ");
    Serial.print(pm.pm25);
    Serial.print(", PM10 = ");
    Serial.println(pm.pm10);
  } else {
    Serial.print("Could not read values from sensor, reason: ");
    Serial.println(pm.statusToString());
  }
  if (ccs.dataAvailable()) {
    ccs.readAlgorithmResults();
    Serial.print("CO2[");
    Serial.print(ccs.getCO2());
    Serial.print("] tVOC[");
    Serial.print(ccs.getTVOC());
    Serial.print("] millis[");
    Serial.print(millis());
    Serial.print("]");
    Serial.println();
  }
  digitalWrite(LED, LOW);
  delay(500);
  digitalWrite(LED, HIGH);
  delay(500);
}