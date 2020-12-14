#include <Arduino.h>
#include <IotWebConf.h>
#include "debugLogger.h"

extern WebServer server;
extern IotWebConf iotWebConf;
extern DebugLogger logger;

void handleRoot() {
  logger.printf("HTTP Request to /\n");
  // -- Let IotWebConf test and handle captive portal requests.
  if (iotWebConf.handleCaptivePortal())
  {
    // -- Captive portal request were already served.
    return;
  }
  String s = "<!DOCTYPE html><html lang=\"en\"><head>"
  "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>"
  "<title>IotWebConf 01 Minimal</title></head><body>Hello world!"
  "Go to <a href='config'>configure page</a> to change settings."
  "</body></html>\n";

  server.send(200, "text/html", s);
}