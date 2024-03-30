#ifndef MYFUNC_H
#define MYFUNC_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "lib/ws2812.h"
#include "lib/sr04.h"
#include "lib/switch.h"
#include "lib/oled.h"

//websocket
void handleRoot(AsyncWebServerRequest *request);
void onEventHandle(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);

#endif
