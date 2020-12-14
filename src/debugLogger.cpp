#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <Arduino.h>
#include "debugLogger.h"

DebugLoggerListItem::DebugLoggerListItem(char* str) {
    content = str;
}

void DebugLoggerListItem::setNext(DebugLoggerListItem* newNext) {
    nextItem = newNext;
}

DebugLoggerListItem* DebugLoggerListItem::getNext() {
    return nextItem;
}

char* DebugLoggerListItem::getContent() {
    return content;
}

void DebugLogger::lockList() {
    while (locked) { delay(1); }
    locked = true;
}

void DebugLogger::unlockList() {
    locked = false;
}

void DebugLogger::print(char* str) {
    DebugLoggerListItem* newItem = new DebugLoggerListItem(str);
    lockList();
    DebugLoggerListItem* i = list;
    if (!i) { list = newItem; }
    else {
        while (i->getNext()) { i = i->getNext(); }
        i->setNext(newItem);
    }
    unlockList();
}

char* DebugLogger::getBuffer() {
    char* str = (char*) malloc(MAX_DEBUG_STR_LEN);
    return str;
}

char* DebugLogger::resizeBuffer(char* str) {
    return (char*) realloc(str, strlen(str) + 1);
}

void DebugLogger::printf(const char* format, ...) {
    char* str = getBuffer();
    va_list argptr;
    va_start(argptr, format);
    vsprintf(str, format, argptr);
    va_end(argptr);
    str = resizeBuffer(str);
    print(str);
}

void DebugLogger::printAllWithSerial() {
    lockList();
    DebugLoggerListItem* i = list;
    while (i) {
        Serial.print(i->getContent());
        DebugLoggerListItem* old = i;
        i = i->getNext();
        free(old->getContent());
        delete old;
    }
    list = NULL;
    unlockList();
}