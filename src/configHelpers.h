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

void initWifiAP();
char* getMqttServer();
unsigned long getMqttPort();
char* getMqttUsername();
char* getMqttPassword();
void loopWifiChecks();