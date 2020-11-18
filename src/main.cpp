#include <Arduino.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <SdsDustSensor.h>
#include <SparkFunCCS811.h>
#include <DHT.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <IotWebConf.h>
#include "i2cHelpers.h"
#include "neopixelMatrix.h"
#include "oledFunctions.h"

#define LED D0
#define PM_SERIAL_RX D7
#define PM_SERIAL_TX D8
#define I2C_PIN_SCL D5
#define I2C_PIN_SDA D6
#define I2C_2_PIN_SCL D9
#define I2C_2_PIN_SDA D10
#define CCS811_ADDR 0x5A
#define NUM_LEDS    64
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     0

SdsDustSensor sds(PM_SERIAL_RX, PM_SERIAL_TX);
CCS811 ccs(CCS811_ADDR);
DHT dht(D2, DHT22);
float temperature, humidity;
float pm25, pm10;
uint16_t co2, voc;
Adafruit_NeoPixel leds = Adafruit_NeoPixel(NUM_LEDS, D4, NEO_GRB + NEO_KHZ800);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
unsigned long lastShift = 0;

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  while(!Serial) {}  // Wait for Serial to start
  Serial.println("Initializing i2c...");
  Wire.begin(I2C_PIN_SDA, I2C_PIN_SCL);
  Wire.setClockStretchLimit(500);
  scanI2C();
  Serial.println("Initializing Display...");
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
  } else {
    display.display();
  }
  delay(800);
  Serial.println("Initializing SDS011 particle sensor...");
  sds.begin();
  Serial.println(sds.queryFirmwareVersion().toString()); // prints firmware version
  Serial.println(sds.setActiveReportingMode().toString()); // ensures sensor is in 'active' reporting mode
  Serial.println(sds.setContinuousWorkingPeriod().toString()); // ensures sensor has continuous working period - default but not recommended
  Serial.println("Initializing DHT22 temperature and humidity sensor...");
  dht.begin();
  temperature = dht.readTemperature(false);
  humidity = dht.readHumidity();
  Serial.println("Initializing CCS811 VOC air quality and CO2 sensor...");
  CCS811Core::CCS811_Status_e returnCode = ccs.beginWithStatus(Wire);
  Serial.printf("CCS811.begin() exited with: %s\n", ccs.statusString(returnCode));
  ccs.setDriveMode(1); // Measure every second.
  ccs.setEnvironmentalData(humidity, temperature);
  leds.begin();
  leds.setBrightness(255);
  for (int i = 0; i < NUM_LEDS; i++) {
    leds.setPixelColor(i, leds.Color(0, 0, 0));
  }
  leds.show();
  Serial.println("Clearing Measurements...");
  shiftCO2Measurements(0.0f);
  shiftPmMeasurements(0.0f);
  Serial.println("Setup complete.");
  lastShift = millis();
}

void loop() {
  Serial.println("---------------------------");
  unsigned long now = millis();
  temperature = dht.readTemperature(false);
  humidity = dht.readHumidity();
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println(F("Failed to read from DHT sensor!"));
  }
  float hic = dht.computeHeatIndex(temperature, humidity, false);
  Serial.print(F("Temperature: "));
  Serial.print(temperature);
  Serial.print(F("°C  Humidity: "));
  Serial.print(humidity);
  Serial.print(F("% Heat index: "));
  Serial.print(hic);
  Serial.println(F("°C"));
  ccs.setEnvironmentalData(humidity, temperature);
  PmResult pm = sds.readPm();
  if (pm.isOk()) {
    pm25 = pm.pm25;
    pm10 = pm.pm10;
    Serial.print("PM2.5 = ");
    Serial.print(pm25);
    Serial.print(", PM10 = ");
    Serial.println(pm10);
    float normedPm25 = ((float) pm25 - 4.0f) / 21.0f;
    float normedPm10 = ((float) pm10 - 7.0f) / 43.0f;
    float normedPm = normedPm25;
    if (normedPm10 > normedPm) normedPm = normedPm10;
    if (normedPm < 0.0f) normedPm = 0.0f;
    if (normedPm > 1.0f) normedPm = 1.0f;
    addPmMeasurement(normedPm);
  } else {
    Serial.print("Could not read values from sensor, reason: ");
    Serial.println(pm.statusToString());
  }
  if (ccs.dataAvailable()) {
    ccs.readAlgorithmResults();
    Serial.printf("CCS baseline: %u\n", ccs.getBaseline());
    co2 = ccs.getCO2();
    voc = ccs.getTVOC();
    Serial.print("CO2 = ");
    Serial.print(co2);
    Serial.print(" tVOC = ");
    Serial.println(voc);
    float normedCO2 = ((float) co2 - 400.0f) / 1100.0f;
    if (normedCO2 < 0.0f) normedCO2 = 0.0f;
    if (normedCO2 > 1.0f) normedCO2 = 1.0f;
    if (co2 > 350 && co2 < 5000) 
      addCO2Measurement(normedCO2);
  }
  if ((now - lastShift) > (3 * 60 * 1000)) {
    float normedPm25 = ((float) pm25 - 4.0f) / 21.0f;
    float normedPm10 = ((float) pm10 - 7.0f) / 43.0f;
    float normedPm = normedPm25;
    if (normedPm10 > normedPm) normedPm = normedPm10;
    if (normedPm < 0.0f) normedPm = 0.0f;
    if (normedPm > 1.0f) normedPm = 1.0f;
    shiftPmMeasurements(normedPm);
    float normedCO2 = ((float) co2 - 400.0f) / 1100.0f;
    if (normedCO2 < 0.0f) normedCO2 = 0.0f;
    if (normedCO2 > 1.0f) normedCO2 = 1.0f;
    shiftCO2Measurements(normedCO2);
    lastShift = now;
  }
  displayMeasurements(pm25, pm10, co2, voc, temperature, humidity);
  displayMatrix();
  delay(1000);
  digitalWrite(LED, HIGH);
}