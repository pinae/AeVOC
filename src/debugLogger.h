#ifndef DEBUG_LOGGER_CLASS
#define DEBUG_LOGGER_CLASS
#define MAX_DEBUG_STR_LEN 256
#define MIN_FREE_HEAP 10000

class DebugLoggerListItem {
    public:
        DebugLoggerListItem(char* str);
        void setNext(DebugLoggerListItem* newNext);
        DebugLoggerListItem* getNext();
        char* getContent();

    private:
        char* content;
        DebugLoggerListItem* nextItem = NULL;
};

class DebugLogger {
    public:
        DebugLogger();
        char* getBuffer();
        char* resizeBuffer(char* str);
        void print(char* str);
        void printf(const char * format, ...);
        void printAllWithSerial();

    private:
        bool locked = false;
        DebugLoggerListItem* list = NULL;
        bool lockList();
        void unlockList();
};

#endif