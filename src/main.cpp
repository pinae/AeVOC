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
#include "neopixelMatrix.h"

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
  Serial.print("CCS811.begin() exited with: ");
  Serial.println(ccs.statusString(returnCode));
  leds.begin();
  leds.setBrightness(25);
  for (int i = 0; i < NUM_LEDS; i++) {
    leds.setPixelColor(i, leds.Color(0, 0, 0));
  }
  leds.show();
  Serial.println("Setup complete.");
}

void loop() {
  Serial.println("Starting loop...");
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
  PmResult pm = sds.readPm();
  if (pm.isOk()) {
    pm25 = pm.pm25;
    pm10 = pm.pm10;
    Serial.print("PM2.5 = ");
    Serial.print(pm25);
    Serial.print(", PM10 = ");
    Serial.println(pm10);
  } else {
    Serial.print("Could not read values from sensor, reason: ");
    Serial.println(pm.statusToString());
  }
  if (ccs.dataAvailable()) {
    ccs.readAlgorithmResults();
    co2 = ccs.getCO2();
    voc = ccs.getTVOC();
    Serial.print("CO2 = ");
    Serial.print(co2);
    Serial.print(" tVOC = ");
    Serial.println(voc);
  }
  displayMeasurements(pm25, pm10, co2, voc, temperature, humidity);
  for (int i = 0; i < NUM_LEDS; i++) {
    leds.setPixelColor(i, leds.Color(0, 0, 255));
    leds.show(); 
    delay(500/NUM_LEDS);
  }
  digitalWrite(LED, LOW);
  for (int i = 0; i < NUM_LEDS; i++) {
    leds.setPixelColor(i, leds.Color(255, 255, 255));
    leds.show(); 
    delay(100/NUM_LEDS);
  }
  delay(400);
  digitalWrite(LED, HIGH);
}