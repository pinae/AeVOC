#ifndef DEV_NAME_CLASS
#define DEV_NAME_CLASS

class DeviceName {
    public:
        DeviceName();
        void print();
        char* get();

    private:
        char deviceName[23] = {'A', 'E', 'V', 'O', 'C', '-', 
                               0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
};

#endif

#ifndef EXTREMES
#define EXTREMES

#define PM25_MINIMUM     3.0f
#define PM25_EXTREME     13.0f
#define PM10_MINIMUM     4.0f
#define PM10_EXTREME     20.0f
#define VOC_MINIMUM      0.0f
#define VOC_EXTREME      850.0f
#define CO2_MINIMUM      400.0f
#define CO2_EXTREME      1500.0f
#define TEMP_MINIMUM     15.0f
#define TEMP_EXTREME     25.0f
#define HUMIDITY_MINIMUM 40.0f
#define HUMIDITY_EXTREME 68.0f

#endif

void initWifiAP();
char* getMqttServer();
unsigned long getMqttPort();
char* getMqttUsername();
char* getMqttPassword();
void loopWifiChecks();
void iotWebConfDelay(unsigned long duration);