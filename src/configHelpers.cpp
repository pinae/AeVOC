#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <IotWebConf.h>
#include "conversionHelpers.h"
#include "configHelpers.h"
#include "webHandlers.h"
#include "mqtt.h"

#ifndef CONFIG_GLOBALS
#define CONFIG_GLOBALS
#define STRING_LEN 128

char mqttServerValue[STRING_LEN];
char mqttPortValue[6] = {'1', '8', '8', '3', '\0', '\0'};
char mqttUserNameValue[STRING_LEN];
char mqttUserPasswordValue[STRING_LEN];
extern WebServer server;
HTTPUpdateServer httpUpdater;
extern IotWebConf iotWebConf;
IotWebConfParameter mqttServerParam = IotWebConfParameter(
    "MQTT server", "mqttServer", mqttServerValue, STRING_LEN);
IotWebConfParameter mqttPortParam = IotWebConfParameter(
    "MQTT port", "mqttPort", mqttPortValue, 6, "text", "1883", "1883");
IotWebConfParameter mqttUserNameParam = IotWebConfParameter(
    "MQTT user", "mqttUser", mqttUserNameValue, STRING_LEN);
IotWebConfParameter mqttUserPasswordParam = IotWebConfParameter(
    "MQTT password", "mqttPass", mqttUserPasswordValue, STRING_LEN, "password");
boolean needMqttConnect = false;
boolean needReset = false;
extern uint8_t matrixBrightness;

#endif

DeviceName::DeviceName() {
    byte mac[6]; WiFi.macAddress(mac);
    char macStr[18];
    sprintf(&macStr[0], "%x:%x:%x:%x:%x:%x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    strncpy(&deviceName[6], macStr, 18);
}

void DeviceName::print() {
    byte mac[6]; WiFi.macAddress(mac);
    char macStr[18];
    sprintf(&macStr[0], "%x:%x:%x:%x:%x:%x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    /*Serial.printf("MAC:\t\t%s\n", macStr);
    Serial.printf("Device Name:\t%s\n", deviceName);*/
}

char* DeviceName::get() {
    return &deviceName[0];
}

void wifiConnected() {
    needMqttConnect = true;
}

void configSaved() {
    //Serial.println("Configuration was updated.");
    needReset = true;
}

void initWifiAP() {
    //Serial.println("Initializing WiFi...");
    iotWebConf.addParameter(&mqttServerParam);
    iotWebConf.addParameter(&mqttPortParam);
    iotWebConf.addParameter(&mqttUserNameParam);
    iotWebConf.addParameter(&mqttUserPasswordParam);
    iotWebConf.setConfigSavedCallback(&configSaved);
    iotWebConf.setWifiConnectionCallback(&wifiConnected);
    iotWebConf.setupUpdateServer(&httpUpdater);
    boolean validConfig = iotWebConf.init();
    if (!validConfig) {
        mqttServerValue[0] = '\0';
        mqttUserNameValue[0] = '\0';
        mqttUserPasswordValue[0] = '\0';
    }
    server.on("/", handleRoot);
    server.on("/config", []{ iotWebConf.handleConfig(); });
    server.onNotFound([](){ iotWebConf.handleNotFound(); });
}

char* getMqttServer() {
    return mqttServerValue;
}

unsigned long getMqttPort() {
    int portVal = strToInt(mqttPortValue);
    if (portVal < 0) return 1883;
    return portVal;
}

char* getMqttUsername() {
    return mqttUserNameValue;
}

char* getMqttPassword() {
    return mqttUserPasswordValue;
}

void setMatrixBrightness(char* payload) {
    //Serial.printf("Setting the brightness of the LED matrix to %s.\n", payload);
    matrixBrightness = strToInt(payload);
}

void mqttSubscribe() {
  subscribeToTopic("setMatrixBrightness", setMatrixBrightness);
}

void loopWifiChecks() {
    if (needMqttConnect) {
        mqttSetup(mqttSubscribe);
        needMqttConnect = false;
    }
    if (needReset) {
        Serial.println("Rebooting after 1 second.");
        iotWebConf.delay(1000);
        ESP.restart();
    }
}