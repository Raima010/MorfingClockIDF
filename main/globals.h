#pragma once

#include <ESPAsyncWebServer.h>
#include "config.h"
#include "digit.h"

extern TaskHandle_t animationTaskHandle;
extern TaskHandle_t weatherTaskHandle;
extern TaskHandle_t adviceTaskHandle;
extern TaskHandle_t wifiTaskHandle;
extern SemaphoreHandle_t wifiSemaphore;
extern TimerHandle_t rainTimer;
extern AsyncWebServer server;
extern volatile int WEATHER_REFRESH_INTERVAL_SEC;
extern struct tm timeinfo;
extern bool clockStartingUp;
extern bool logMessageActive;
extern bool fadeNecessary;
extern bool adviceValid;
extern bool isAdviceScrolling;
extern char use_ani;
extern char statusMessage[128];
extern char full_device_name[32];
extern char currentAdvice[256];
extern unsigned long messageDisplayMillis;
extern Digit digit0, digit1, digit2, digit3, digit4, digit5;

// MQTT
extern IPAddress MQTT_HOST;
extern const uint16_t MQTT_PORT;
extern const char* MQTT_SUB_TOPIC;

// Define the available display modes
typedef enum {
    MODE_CLOCK,     // Normal morphing clock
    MODE_RAIN       // Matrix digital rain
} ClockMode_t;

extern volatile ClockMode_t currentClockMode;