#ifndef MQTT_TYPES
#define MQTT_TYPES

class SubscribedMqttTopic {
    public:
        SubscribedMqttTopic(char* fullTopicStr, void (*callbackFunction)(char*));
        char* getTopic();
        void (*callback)(char*);

    private:
        char* topic;
};

class SubscribedMqttTopicList {
    public:
        SubscribedMqttTopicList(
            SubscribedMqttTopic* newTopic, 
            SubscribedMqttTopicList* nextElement);
        char* getTopic();
        SubscribedMqttTopic* getEntry();
        SubscribedMqttTopicList* getNext();

    private:
        SubscribedMqttTopic* entry;
        SubscribedMqttTopicList* next;
};

#endif

void mqttSetup(void (*subscriptionCallback)());
void mqttLoop();
bool mqttConnected();
void subscribeToTopic(const char* topic, void (*callback)(char*));
void publishToMqtt(const char* topic, char* payload);