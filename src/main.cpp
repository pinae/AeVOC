#include <Arduino.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <SdsDustSensor.h>
#include <SparkFunCCS811.h>
#include <DHT.h>
#include <Adafruit_NeoPixel.h>
#include <LiquidCrystal_PCF8574.h>

#define LED D0
#define PM_SERIAL_RX D7
#define PM_SERIAL_TX D8
#define I2C_PIN_SCL D5
#define I2C_PIN_SDA D6
#define CCS811_ADDR 0x5A
#define NUM_LEDS    64

SdsDustSensor sds(PM_SERIAL_RX, PM_SERIAL_TX);
CCS811 ccs(CCS811_ADDR);
DHT dht(D2, DHT22);
float temperature, humidity;
float pm25, pm10;
uint16_t co2, voc;
Adafruit_NeoPixel leds = Adafruit_NeoPixel(NUM_LEDS, D4, NEO_GRB + NEO_KHZ800);
LiquidCrystal_PCF8574 lcd(0x27);

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  while(!Serial) {}  // Wait for Serial to start
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
  Serial.print("Initializing LCD");
  Wire.beginTransmission(0x27);
  int error = Wire.endTransmission();
  if (error == 0) {
    lcd.begin(16, 2);
    Serial.println(": LCD found.");
  } else {
    Serial.println(": LCD not found.");
  }
  lcd.setBacklight(255);
  lcd.home();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("ESP started...");
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
  leds.setBrightness(255);
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
  lcd.home();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("PM2,5: ");
  lcd.print(pm25);
  lcd.print(" PM10: ");
  lcd.print(pm10);
  lcd.setCursor(0, 1);
  lcd.print(temperature);
  lcd.print("°C ");
  lcd.print(humidity);
  lcd.print("% ");
  lcd.print(co2);
  lcd.print("ppm");
  /*for (int i = 0; i < NUM_LEDS; i++) {
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
  delay(400);*/
  delay(500);
  lcd.setBacklight(0);
  delay(500);
  digitalWrite(LED, HIGH);
}