#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

void initMqtt();
void connectToWifi();
void WiFiEvent(WiFiEvent_t event);
void wifiTask(void* parameter);
