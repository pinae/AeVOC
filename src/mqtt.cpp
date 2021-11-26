#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "debugLogger.h"
#include "mqtt.h"
#include "configHelpers.h"

#ifndef MQTT_GLOBALS
#define MQTT_GLOBALS

extern DeviceName devName;
extern DebugLogger logger;
WiFiClient espClient;
PubSubClient mqttClient(espClient);
SubscribedMqttTopicList* subscribedTopicsList = NULL;
void (*savedSubscriptionCallback)();
bool setupDone = false;
extern bool wifiIsConnected;

#endif

SubscribedMqttTopic::SubscribedMqttTopic(
        char* fullTopicStr, 
        void (*callbackFunction)(char*)) {
    topic = fullTopicStr;
    callback = callbackFunction;
}

SubscribedMqttTopicList::SubscribedMqttTopicList(
        SubscribedMqttTopic* newTopic, 
        SubscribedMqttTopicList* nextElement) {
    entry = newTopic;
    next = nextElement;
}

char* SubscribedMqttTopic::getTopic() {
    return topic;
}

char* SubscribedMqttTopicList::getTopic() {
    return entry->getTopic();
}

SubscribedMqttTopic* SubscribedMqttTopicList::getEntry() {
    return entry;
}

SubscribedMqttTopicList* SubscribedMqttTopicList::getNext() {
    return next;
}

void onMqttMessage(char* topic, byte* payload, unsigned int length) {
    char* plStr = (char*) calloc(length+1, sizeof(char));
    strncpy(plStr, (char*) payload, length);
    *(plStr + length) = 0;
    logger.printf("MQTT: %s (len: %d) - Payload: %s\n", 
                  topic, length, plStr);
    SubscribedMqttTopicList* stl = subscribedTopicsList;
    while(stl != NULL) {
        if (strcmp(stl->getTopic(), topic) == 0) {
            logger.printf("Found topic: %s\n", stl->getTopic());
            stl->getEntry()->callback(plStr);
            break;
        }
        stl = stl->getNext();
    }
    logger.printf("-----------------\n");
    free(plStr);
}

char* createFullTopicStr(const char* topic) {
    char* deviceName = devName.get();
    unsigned int topicLen = strlen(deviceName)+1+strlen(topic);
    char* fullTopicStr = (char*) calloc(topicLen+1, sizeof(char));
    strncpy(fullTopicStr, deviceName, strlen(deviceName));
    strncpy(fullTopicStr+strlen(deviceName), "/", 2);
    strncpy(fullTopicStr+strlen(deviceName)+1, topic, strlen(topic));
    *(fullTopicStr + topicLen+1) = '\0';
    return fullTopicStr;
}

void subscribeToTopic(const char* topic, void (*callback)(char*)) {
    if (!wifiIsConnected || !mqttClient.connected()) return;
    char* fullTopicStr = createFullTopicStr(topic);
    mqttClient.subscribe(fullTopicStr);
    SubscribedMqttTopic* newTopic = new SubscribedMqttTopic(fullTopicStr, callback);
    SubscribedMqttTopicList* newTopicListElem = new SubscribedMqttTopicList(
        newTopic, subscribedTopicsList);
    subscribedTopicsList = newTopicListElem;
    logger.printf("Subscribed to MQTT-topic: %s\n", fullTopicStr);
}

void connectToMqtt(void (*subscriptionCallback)()) {
    boolean successfullyConnected = false;
    while (!mqttClient.connected()) {
        //Serial.printf("Attempting MQTT connection... ");
        logger.printf("Attempting MQTT connection... ");
        if (strlen(getMqttUsername()) > 0 && strlen(getMqttPassword())) {
            //Serial.printf("\nMQTT: Using credentials: %s PW: %s\n", 
            //              getMqttUsername(), getMqttPassword());
            logger.printf("\nMQTT: Using credentials: %s PW: %s\n", 
                          getMqttUsername(), getMqttPassword());
            successfullyConnected = mqttClient.connect(
                devName.get(), getMqttUsername(), getMqttPassword());
        } else {
            //Serial.printf("Connecting without credentials. Name: %s\n", devName.get());
            successfullyConnected = mqttClient.connect(devName.get());
        }
        if (successfullyConnected) {
            //Serial.printf("connected!\n");
            logger.printf("connected!\n");
            subscriptionCallback();
        } else {
            //Serial.printf("failed, rc=%d \ntry again in 1 second.\n", mqttClient.state());
            logger.printf("failed, rc=%d \ntry again in 1 second.\n", mqttClient.state());
            // Wait at least 1 second before retrying
            iotWebConfDelay(999);
            return;
        }
    }
}

void mqttSetup(void (*subscriptionCallback)()) {
    savedSubscriptionCallback = subscriptionCallback;
    if (!setupDone) {
        Serial.printf("mqttSetup: %s:%lu\n", getMqttServer(), getMqttPort());
        mqttClient.setServer(getMqttServer(), getMqttPort());
        mqttClient.setCallback(onMqttMessage);
        setupDone = true;
    }
}

void publishToMqtt(const char* topic, char* payload) {
    if (!wifiIsConnected || !mqttClient.connected()) return;
    char* fullTopicStr = createFullTopicStr(topic);
    mqttClient.publish(fullTopicStr, payload);
    free(fullTopicStr);
}

bool mqttConnected() {
    return mqttClient.connected();
}

void mqttLoop() {
    if (!setupDone) return;
    if (wifiIsConnected && !mqttClient.connected()) {
        connectToMqtt(savedSubscriptionCallback);
    }
    mqttClient.loop();
}
