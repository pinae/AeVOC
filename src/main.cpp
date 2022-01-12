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
#include <IotWebConfUsing.h>
#include "debugLogger.h"
#include "i2cHelpers.h"
#include "neopixelMatrix.h"
#include "oledFunctions.h"
#include "configHelpers.h"
#include "mqtt.h"

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
#define CONFIG_VERSION "v0.2.2"

DebugLogger logger;
SdsDustSensor sds(PM_SERIAL_RX, PM_SERIAL_TX);
CCS811 ccs(CCS811_ADDR);
DHT dht(D2, DHT22);
float temperature, humidity;
float pm25, pm10;
uint16_t co2, voc;
Adafruit_NeoPixel leds = Adafruit_NeoPixel(NUM_LEDS, D4, NEO_GRB + NEO_KHZ800);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
unsigned long lastShift = 0;
unsigned long lastMeasurement = 0;
DNSServer dnsServer;
WebServer server(80);
const char wifiInitialApPassword[] = "loving_ct";
DeviceName devName;
IotWebConf iotWebConf(devName.get(), &dnsServer, &server, wifiInitialApPassword, CONFIG_VERSION);
uint8_t matrixBrightness = 50;

void setup() {
  Serial.begin(115200);
  while(!Serial) {}  // Wait for Serial to start
  delay(750);
  devName.print();
  initWifiAP();
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
  leds.setBrightness(matrixBrightness);
  for (int i = 0; i < NUM_LEDS; i++) {
    leds.setPixelColor(i, leds.Color(0, 0, 0));
  }
  leds.show();
  Serial.println("Clearing Measurements...");
  shiftVocMeasurements(0.0f);
  shiftPmMeasurements(0.0f);
  Serial.println("Setup complete.");
  unsigned long now = millis();
  lastShift = now;
  lastMeasurement = now;
}

void measure() {
  Serial.println("---------------------------");
  temperature = dht.readTemperature(false);
  humidity = dht.readHumidity();
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println(F("Failed to read from DHT sensor!"));
  } else {
    char temperatureStr[50];
    sprintf(&temperatureStr[0], "%2.1f", temperature);
    publishToMqtt("temperature", &temperatureStr[0]);
    char humidityStr[50];
    sprintf(&humidityStr[0], "%2.1f", humidity);
    publishToMqtt("humidity", &humidityStr[0]);
  }
  float hic = dht.computeHeatIndex(temperature, humidity, false);
  Serial.printf("Temperature: %2.1f°C  Humidity: %2.1f%%  Heat index: %2.1f°C\n", 
                temperature, humidity, hic);
  ccs.setEnvironmentalData(humidity, temperature);
  PmResult pm = sds.readPm();
  if (pm.isOk()) {
    pm25 = pm.pm25;
    pm10 = pm.pm10;
    Serial.print("PM2.5 = ");
    Serial.print(pm25);
    Serial.print(", PM10 = ");
    Serial.println(pm10);
    addPmMeasurement(maxOf(
      normValue((float) pm25, PM25_MINIMUM, PM25_EXTREME), 
      normValue((float) pm10, PM10_MINIMUM, PM10_EXTREME)));
    char pmJson[80];
    sprintf(&pmJson[0], "{\"pm2,5\": %2.2f, \"pm10\": %2.2f}", pm25, pm10);
    publishToMqtt("particles_and_aerosoles", &pmJson[0]);
  } else {
    Serial.print("Could not read values from sensor, reason: ");
    Serial.println(pm.statusToString());
  }
  if (ccs.dataAvailable()) {
    ccs.readAlgorithmResults();
    Serial.printf("CCS baseline: %u\n", ccs.getBaseline());
    uint16_t newCo2 = ccs.getCO2();
    uint16_t newVoc = ccs.getTVOC();
    Serial.printf("tVOC = %u    CO2 (projection) = %u\n", newVoc, newCo2);
    if (sanityCheck(newCo2, 400, 5000)) co2 = newCo2;
    if (sanityCheck(newVoc, 0, 4000)) {
      voc = newVoc;
      addVocMeasurement(normValue((float) voc, VOC_MINIMUM, VOC_EXTREME));
    }
    char vocJson[80];
    sprintf(&vocJson[0], "{\"tVOC\": %u, \"CO2\": %u}", newVoc, newCo2);
    publishToMqtt("voc", &vocJson[0]);
  }
}

void loop() {
  unsigned long now = millis();
  iotWebConf.doLoop();
  loopWifiChecks();
  if ((now - lastMeasurement) > 1000) {
    measure();
    if ((now - lastShift) > (3 * 60 * 1000)) {
      shiftPmMeasurements(maxOf(
        normValue((float) pm25, PM25_MINIMUM, PM25_EXTREME), 
        normValue((float) pm10, PM10_MINIMUM, PM10_EXTREME)));
      shiftVocMeasurements(normValue((float) voc, VOC_MINIMUM, VOC_EXTREME));
      lastShift = now;
    }
    displayMeasurements(pm25, pm10, co2, voc, temperature, humidity);
    leds.setBrightness(matrixBrightness);
    displayMatrix();
    Serial.printf("Free Heap: %u\n", ESP.getFreeHeap());
    lastMeasurement = now;
  }
  logger.printAllWithSerial();
}
