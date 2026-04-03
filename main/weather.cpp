#include <ArduinoJson.h>
#include "matrix_globals.h"
#include "globals.h"
#include "config.h"
#include "weather.h"
#include "TinyIcons.h"
#include "clock.h"
#include <HTTPClient.h>
#include "esp_http_client.h"
#include "esp_crt_bundle.h"


extern TaskHandle_t weatherTaskHandle;
const rgb24 WEATHER_WARM = {80, 40, 40};  // warm amber
volatile int8_t currentTemp = 0;
volatile uint8_t windSpeed = 0;
volatile bool isDay = true;
volatile bool weatherValid = false;
volatile uint8_t condM=0;
volatile int WEATHER_REFRESH_INTERVAL_SEC = 600;
volatile int WEATHER_REFRESH_INTERVAL_SEC_FAIL = 600;
extern SMLayerBackground<rgb24, 0> * ptrBackgroundLayer;
char currentAdvice[256] = "Loading advice..."; // Fixed buffer
bool adviceValid = false;

//==============================Functions=================================
uint8_t mapWMOCodeToCondM(uint8_t code) {
    if (code <= 1) return 1;                                                  // Clear/Mainly Clear -> SUN
    if (code == 2) return 2;                                                  // Partly Cloudy -> CLOUD
    if (code == 3 || code == 45 || code == 48) return 3;                      // Overcast/Fog -> OVERCAST
    if ((code >= 51 && code <= 67) || (code >= 80 && code <= 82)) return 4;   // Rain/Drizzle -> RAIN
    if (code >= 95) return 5;                                                 // Thunderstorm -> THUNDER
    if ((code >= 71 && code <= 77) || code >= 85) return 6;                   // Snow -> SNOW

    return 1; // Fallback to Sun
}
//-------------------------RGB565 to RGB24-----------
static inline rgb24 rgb565_to_rgb24(uint16_t c) {
    rgb24 out;
    out.red   = ((c >> 11) & 0x1F) * 255 / 31;  //(c & 0xF800) >> 8; //Jeigu reikia greiciau bet kokybe blogesne
    out.green = ((c >> 5)  & 0x3F) * 255 / 63;  //(c & 0x07E0) >> 3; //
    out.blue  = ( c        & 0x1F) * 255 / 31;  //(c & 0x001F) << 3; //
    return out;
}
//--------------------------Draw Icon--------------
void drawIconRGB565(
    const uint16_t* ico,
    uint8_t xo,
    uint8_t yo,
    uint8_t cols,
    uint8_t rows
) {
    for (uint8_t y = 0; y < rows; y++) {
        for (uint8_t x = 0; x < cols; x++) {
            uint16_t c = ico[y * cols + x];
            if (c) {  // treat 0 as transparent
                backgroundLayer.drawPixel(
                    xo + x,
                    yo + y,
                    rgb565_to_rgb24(c)
                );
            }
        }
    }
}

//--------------------------Draw Animated icon--------
void draw_weather_animations(int stp) {
    if (!use_ani) return;
    const uint16_t *af = nullptr;
    int xo = 53;
    int yo = 0;

    if (!isDay && condM < 4) {
        af = mony_ani[stp % 5];
    } else {
        switch (condM) {
            case 0: af = mony_ani[stp % 5]; break;
            case 1: af = suny_ani[stp % 5]; break;
            case 2: af = clod_ani[stp % 5]; break;
            case 3: af = ovct_ani[stp % 5]; break;
            case 4: af = rain_ani[stp % 5]; break;
            case 5: af = thun_ani[stp % 5]; break;
            case 6: af = snow_ani[stp % 5]; break;
        }
    }

    if (af){
        backgroundLayer.fillRectangle(xo, yo, xo + 9, yo + 4, {0, 0, 0}); //clear icon area
        drawIconRGB565(af, xo, yo, 10, 5);
    }
}
//---------------------------------Status line icon----------

void drawBitmap8x8(int x, int y, const uint8_t* bmp, rgb24 color) {
  for (int row = 0; row < 8; row++) {
    uint8_t bits = bmp[row];
    for (int col = 0; col < 8; col++) {
      if (bits & (1 << (7 - col))) {
        backgroundLayer.drawPixel(x + col, y + row, color);
      }
    }
  }
}
//========================Staus line====================
void drawNowWeather() {

   if (logMessageActive || isAdviceScrolling){
    return;} //Do not overdraw log message untill LOG_MESSAGE_PERSISTENCE_MSEC

  const int textY = 25;
  const int fontH = 10;

  const int iconX = 64 - 10;
  const int iconY = textY - 1;

  // Clear sub-line
  backgroundLayer.fillRectangle(
    0,
    textY,
    63,
    textY + fontH - 1,
    {0, 0, 0}
  );

  if (!weatherValid) {
    backgroundLayer.drawString(0, textY, WEATHER_WARM, "No weather");
    return;
  }
  
   char buf[20];

  snprintf(buf, sizeof(buf), "Now %dC %dms", currentTemp, windSpeed);
  backgroundLayer.drawString(0, textY, WEATHER_WARM, buf);

  //const uint8_t* icon = getWeatherIconBitmap(condM);
  //drawBitmap8x8(iconX, iconY, icon, WEATHER_WARM);
}
//======================= GET WEATHER FROM OpenMeteo ============


void updateWeatherOpenMeteo() {
  // 1. Safety Check: If already busy or no WiFi, exit immediately
  if (WiFi.status() != WL_CONNECTED) return;
  
  Serial.println(F("\n[Weather] Starting update (Stream)..."));
  
  WiFiClient client;
  HTTPClient http;
  http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
  http.useHTTP10(true); 
  http.setTimeout(5000);

  const char* weatherUrl = "http://api.open-meteo.com/v1/forecast?latitude=57.2645&longitude=16.4484&current_weather=true&wind_speed_unit=ms";

  // 2. Start Connection
  if (http.begin(client, weatherUrl)) { 
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
      StaticJsonDocument<512> localDoc;
      StaticJsonDocument<128> filter;
      filter["current_weather"]["temperature"] = true;
      filter["current_weather"]["windspeed"] = true;
      filter["current_weather"]["weathercode"] = true;
      filter["current_weather"]["is_day"] = true;

      DeserializationError err = deserializeJson(localDoc, http.getStream(), DeserializationOption::Filter(filter));

      if (!err) {
        JsonObject cw = localDoc["current_weather"];
        if (!cw.isNull()) {
          currentTemp = round(cw["temperature"].as<float>());
          windSpeed   = round(cw["windspeed"].as<float>());
          uint8_t wmoCode = cw["weathercode"].as<int>();
          isDay = cw["is_day"].as<int>() == 1;
          condM = mapWMOCodeToCondM(wmoCode);
          
          weatherValid = true;
          WEATHER_REFRESH_INTERVAL_SEC = 600;
          Serial.printf("[Weather] OK T:%d W:%d Code:%d\n", currentTemp, windSpeed, wmoCode);
        }
      } else {
        Serial.printf("[Weather] JSON Error: %s\n", err.c_str());
        weatherValid = false;
        WEATHER_REFRESH_INTERVAL_SEC = 30;
      }
    } else {
      // This handles your -11 and 302 errors
      Serial.printf("[Weather] HTTP Error: %d\n", httpCode);
      weatherValid = false;
      WEATHER_REFRESH_INTERVAL_SEC = 30; 
    }

    http.end(); // Clean up the HTTP object
    client.flush();
    vTaskDelay(pdMS_TO_TICKS(10));
    client.stop(); // Clean up the WiFi socket
  } else {
    Serial.println(F("[Weather] Connection Failed"));
    weatherValid = false;
    WEATHER_REFRESH_INTERVAL_SEC = 30;
  }

  vTaskDelay(pdMS_TO_TICKS(100)); 

}

//===========================Advice function==================================

void updateAdvice() {
    if (WiFi.status() != WL_CONNECTED) return;

    Serial.println(F("\n[Advice] Fetching advice (ESP-IDF Direct)..."));

    // 1. Setup config (No event handler needed for this method)
    esp_http_client_config_t config = {};
    config.url = "https://api.adviceslip.com/advice";
    config.crt_bundle_attach = esp_crt_bundle_attach;
    config.method = HTTP_METHOD_GET;
    config.timeout_ms = 5000;

    esp_http_client_handle_t client = esp_http_client_init(&config);

    // 2. Open the connection
    esp_err_t err = esp_http_client_open(client, 0); 
    
    if (err == ESP_OK) {
        // 3. Mandatory: Fetch headers and get the response length
        int content_length = esp_http_client_fetch_headers(client);
        
        // 4. Read the raw data into currentAdvice
        int read_len = esp_http_client_read_response(client, currentAdvice, sizeof(currentAdvice) - 1);
        
        if (read_len > 0) {
            currentAdvice[read_len] = 0; // Null terminate raw JSON string

            // 5. Parse the JSON
            StaticJsonDocument<256> doc;
            DeserializationError error = deserializeJson(doc, currentAdvice);

            if (!error) {
                const char* adviceText = doc["slip"]["advice"];
                if (adviceText) {
                    snprintf(currentAdvice, sizeof(currentAdvice), "ADVICE: %s", adviceText);
                    adviceValid = true; 
                    printf("Advice: %s\n", currentAdvice);
                }
            } else {
                printf("JSON Parse failed: %s\n", error.c_str());
            }
        }
    } else {
        printf("HTTPS Open Error: %s\n", esp_err_to_name(err));
    }

    // 6. Cleanup
    esp_http_client_cleanup(client);
}

//===========================Weather update task=============================
void weatherTask(void* parameter) {
    for (;;) {
        // 1. Wait for a trigger (from your timer/loop)
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // 2. Try to grab the "WiFi Key" (Wait up to 10 seconds if Advice is using it)
        if (xSemaphoreTake(wifiSemaphore, pdMS_TO_TICKS(10000)) == pdTRUE) {
         
            if (WiFi.status() == WL_CONNECTED) {
                updateWeatherOpenMeteo();
            }
            // 3. Give the "WiFi Key" back so AdviceTask can use it
            xSemaphoreGive(wifiSemaphore); 
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Small breather
    }
}

//=============================adviceTask=====================
void adviceTask(void* parameter) {
    for (;;) {
        
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (xSemaphoreTake(wifiSemaphore, pdMS_TO_TICKS(45000)) == pdTRUE) {
            
            if (WiFi.status() == WL_CONNECTED) {
                 updateAdvice();
            }
            xSemaphoreGive(wifiSemaphore); 
        }       
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
//=================================END=========================}