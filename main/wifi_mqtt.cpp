#define ASYNC_MQTT_GENERIC_DEBUG      0
#include <AsyncMqtt_Generic.h>
#include <Arduino.h>
#include "WiFiSettings.h"
#include "wifi.h"
#include "wifi_mqtt.h"
#include "globals.h"
#include "web_ota.h"
#include "config.h"
#include "clock.h"

AsyncMqttClient mqttClient;
TimerHandle_t mqttReconnectTimer;
bool mqqt_fail_message = false;
bool mqttIsAttemptingConnection = false;  

//==================================MQTT=======================================

static void onMqttConnect(bool sessionPresent) {
    Serial.printf("MQTT Connected!\n");
    mqttIsAttemptingConnection = false; // Connection successful
    xSemaphoreGive(wifiSemaphore);      // RELEASE the lock for others
    mqttClient.subscribe(MQTT_SUB_TOPIC, 2);
}

static void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
    if (mqqt_fail_message == false) {
        logStatusMessage("Mqtt Stop");
        mqqt_fail_message = true;
    }
    // Only give the semaphore if WE were the ones holding it
    if (mqttIsAttemptingConnection) {
        mqttIsAttemptingConnection = false;
        xSemaphoreGive(wifiSemaphore);
    }

    if (WiFi.isConnected()) {
        // If server is off, wait 30s before trying again to keep CPU free
        xTimerStart(mqttReconnectTimer, pdMS_TO_TICKS(30000));
    }
}

void connectToMqtt() {
    // Try to lock the network. Wait 5s if Weather is running.
    if (xSemaphoreTake(wifiSemaphore, pdMS_TO_TICKS(5000)) == pdTRUE) {
        mqttIsAttemptingConnection = true; // We now OWN the lock
        //Serial.printf("Connecting to MQTT...");
        mqttClient.connect();
    } else {
        // Network is busy (Weather/Advice is running). 
        // Don't lock anything. Just try again in 10 seconds.
        xTimerStart(mqttReconnectTimer, pdMS_TO_TICKS(10000));
    }
}

static void onMqttMessage(char*, char* payload,
                          const AsyncMqttClientMessageProperties&,
                          const size_t& len,
                          const size_t&,
                          const size_t&)
{
    char message_buffer[128];

    // Limit copy to buffer size
    size_t copyLen = (len < sizeof(message_buffer) - 1)
                     ? len
                     : sizeof(message_buffer) - 1;

    memcpy(message_buffer, payload, copyLen);
    message_buffer[copyLen] = '\0';   // Null-terminate

    Serial.print(message_buffer);
    logStatusMessage(message_buffer);
}

void initMqtt(){

    Serial.println("Init MQTT");
    mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(30000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
  
    mqttClient.setServer(MQTT_HOST, MQTT_PORT);
    mqttClient.setClientId(full_device_name);
    mqttClient.onConnect(onMqttConnect);
    mqttClient.onDisconnect(onMqttDisconnect);
    mqttClient.onMessage(onMqttMessage);
    
    mqttClient.setKeepAlive(60);
    mqttClient.setCleanSession(true);
  }

//==================================WiFi=======================================

void connectToWifi() {
      while (!WiFi.isConnected())
      {
          Serial.printf("Connecting to Wi-Fi...\n");
          WiFiSettings.connect(true, 30);
          vTaskDelay(pdMS_TO_TICKS(2000));
      }
      }
  
void WiFiEvent(WiFiEvent_t event)
{
    switch (event)
    {
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        {
            IPAddress ip = WiFi.localIP();
            snprintf(statusMessage, sizeof(statusMessage),
                     "%u.%u.%u.%u",
                     ip[0], ip[1], ip[2], ip[3]);
            setupWebServer(server);
            setupOTA(server);

            #if USE_MQTT
            xTimerStart(mqttReconnectTimer, pdMS_TO_TICKS(10000)); //Enable MQTT-dont start if disabled
            #endif

            break;
        }

        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        {
            logStatusMessage("No WiFi");
            xTaskNotifyGive(wifiTaskHandle);
            break;
        }

        default:
            break;
    }
}

void wifiTask(void* parameter)
{
    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (WiFiSettings.connect(true, 30) != true)
        {
            Serial.printf("ReConnecting to Wi-Fi...\n");
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }
}

//==================================END=======================================
