#ifndef MQTT_TYPES
#define MQTT_TYPES

struct subscribedMqttTopic {
    char* topic;
    void (*callback)(char*);
};

struct subscribedMqttTopicList {
    subscribedMqttTopic* entry;
    subscribedMqttTopicList* next;
};

#endif

void mqttSetup(void (*subscriptionCallback)());
void mqttLoop();
void subscribeToTopic(const char* topic, void (*callback)(char*));
void publishToMqtt(const char* topic, char* payload);