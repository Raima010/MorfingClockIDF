#include "Arduino.h"
#include "SmartMatrix.h"
#include "globals.h"
#include "config.h"
#include "clock.h"
#include "weather.h"
#include "Mem.h"
#include "SPIFFS.h"
#include "wifi_mqtt.h"
#include "esp_mac.h"

extern "C" void esp_timer_impl_update_apb_freq(uint32_t apb_freq) {} //for compatibility only!

static uint8_t lineCount = 0;
#define COLOR_DEPTH 24                  // Choose the color depth used for storing pixels in the layers: 24 or 48 (24 is good for most sketches - If the sketch uses type `rgb24` directly, COLOR_DEPTH must be 24)
const uint16_t kMatrixWidth = 64;       // Set to the width of your display, must be a multiple of 8
const uint16_t kMatrixHeight = 32;      // Set to the height of your display
const uint8_t kRefreshDepth = 36;       // Tradeoff of color quality vs refresh rate, max brightness, and RAM usage.  36 is typically good, drop down to 24 if you need to.  On Teensy, multiples of 3, up to 48: 3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45, 48.  On ESP32: 24, 36, 48
const uint8_t kDmaBufferRows = 4;       // known working: 2-4, use 2 to save RAM, more to keep from dropping frames and automatically lowering refresh rate.  (This isn't used on ESP32, leave as default)
const uint8_t kPanelType = SM_PANELTYPE_HUB75_32ROW_MOD16SCAN;   // Choose the configuration that matches your panels.  See more details in MatrixCommonHub75.h and the docs: https://github.com/pixelmatix/SmartMatrix/wiki
const uint32_t kMatrixOptions = (SM_HUB75_OPTIONS_NONE);         // see docs for options: https://github.com/pixelmatix/SmartMatrix/wiki
const uint8_t kBackgroundLayerOptions = (SM_BACKGROUND_OPTIONS_NONE);
const uint8_t kScrollingLayerOptions = (SM_SCROLLING_OPTIONS_NONE);
const uint8_t kIndexedLayerOptions = (SM_INDEXED_OPTIONS_NONE);
constexpr int MAX_CHARS  = kMatrixWidth / 6;

unsigned long prevEpoch = 0;
unsigned long lastNTPUpdate = 0;
unsigned long lastWeatherUpdate = 0;
char statusMessage[128];
struct tm timeinfo;
char use_ani = true;
bool clockStartingUp = true;
//------------------Animated icons------------------
uint8_t frame = 0;
unsigned long lastIconUpdate = 0;
unsigned long housekeeper_10 = 0;
unsigned long housekeeper_1 = 0;
//uint8_t NightMode = 0;
bool isAdviceScrolling = false;
//---------------------MQTT-------------------------
IPAddress MQTT_HOST(192,168,1,000); //Use your own!
const uint16_t MQTT_PORT = 1883;
const char* MQTT_SUB_TOPIC = "Topic";
char full_device_name[32];
//--------------------SERVER -----------------------
AsyncWebServer server(80);
//---------------------Matrix Allocations-----------
SMARTMATRIX_ALLOCATE_BUFFERS(matrix, kMatrixWidth, kMatrixHeight, kRefreshDepth, kDmaBufferRows, kPanelType, kMatrixOptions);
//SMARTMATRIX_ALLOCATE_BACKGROUND_LAYER(backgroundLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kBackgroundLayerOptions);
SMLayerBackground<rgb24, kBackgroundLayerOptions> backgroundLayer(kMatrixWidth, kMatrixHeight);
#ifdef USE_ADAFRUIT_GFX_LAYERS
  // there's not enough allocated memory to hold the long strings used by this sketch by default, this increases the memory, but it may not be large enough
  SMARTMATRIX_ALLOCATE_GFX_MONO_LAYER(scrollingLayer, kMatrixWidth, kMatrixHeight, 6*1024, 1, COLOR_DEPTH, kScrollingLayerOptions);
#else
  //SMARTMATRIX_ALLOCATE_SCROLLING_LAYER(scrollingLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kScrollingLayerOptions);
  SMLayerScrolling<rgb24, kScrollingLayerOptions> scrollingLayer(kMatrixWidth, kMatrixHeight);
#endif
//SMARTMATRIX_ALLOCATE_INDEXED_LAYER(indexedLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kIndexedLayerOptions);
uint8_t defaultBrightness = (255); //(100*255)/100;        // full (100%) brightness
//const int defaultBrightness = (37); //(15*255)/100;       // dim: 15% brightness
const int defaultScrollOffset = 24;
const rgb24 defaultBackgroundColor = {0x30, 0, 0};
//==========================Tasks&Functions===================================
TaskHandle_t weatherTaskHandle = NULL;
TaskHandle_t wifiTaskHandle = NULL;
TaskHandle_t animationTaskHandle = NULL;
TaskHandle_t adviceTaskHandle = NULL;
SemaphoreHandle_t wifiSemaphore;
TimerHandle_t housekeepingTimer;
void housekeepingCallback(TimerHandle_t xTimer);
void drawCenteredTwoLineText(const char* text);
void draw_weather_animations(int frame);
void weatherTask(void *pvParameters);
void wifiTask(void *pvParameters);
void drawNowWeather();
void fadeOutMatrix();
void safetyrestart();
void scrollAdvice();
void nightmode();
void getadvice();


//-------------Matrix clock-----------------
volatile ClockMode_t currentClockMode = MODE_CLOCK;
TimerHandle_t rainTimer; // Handle for the auto-exit timer

void stopRainCallback(TimerHandle_t xTimer) {
    fadeOutMatrix();
    currentClockMode = MODE_CLOCK;
    clockStartingUp = true; 
    lastIconUpdate = 0;
}
//------------------------------------------

            void displayUpdater() {
                if(!getLocalTime(&timeinfo)){
                    Serial.println("Failed to obtain time"); //
                    return;
                }
                unsigned long epoch = mktime(&timeinfo);

                if (epoch != prevEpoch) {
                  if (currentClockMode == MODE_CLOCK) {
                    displayClock();}
                    prevEpoch = epoch;
                }
            }

// Clock Logic Task-------------------------
            void clockLogicTask(void *pvParameters) {

                for (;;) {
                    // 1. NTP Sync (every 6 hours)
                    if (millis() - lastNTPUpdate > 21600UL * 1000UL) {
                        configTzTime(TIMEZONE, "pool.ntp.org", "time.nist.gov");
                        lastNTPUpdate = millis();
                    }

                    // 2. Weather Update Trigger
                    if (millis() - lastWeatherUpdate > WEATHER_REFRESH_INTERVAL_SEC * 1000UL) {
                        if (weatherTaskHandle != NULL) xTaskNotifyGive(weatherTaskHandle);
                        lastWeatherUpdate = millis();
                    }

                    // 3. Status Message Timeout
                    if (logMessageActive) {
                        if (millis() > messageDisplayMillis + LOG_MESSAGE_PERSISTENCE_MSEC) {
                            logMessageActive = false;
                        }
                    }

                    // 4. Update time & Weather Animations (300ms)
                    if (millis() - lastIconUpdate > 300UL) {
                        lastIconUpdate = millis();
                        frame = (frame + 1) % 5;
                        if (currentClockMode == MODE_CLOCK) {
                        draw_weather_animations(frame);}
                        displayUpdater();
                    }
                  
                     //5. Helper functions (10s)
                    if (millis() - housekeeper_10 > 10000UL) {
                        housekeeper_10 = millis();
                        getadvice();
                        safetyrestart();
                    }

                    //6. Helper functions (1s)
                    if (millis() - housekeeper_1 > 1000UL) {
                        housekeeper_1 = millis();
                        nightmode();
                        scrollAdvice();                             
                    }
                    
                    vTaskDelay(pdMS_TO_TICKS(15)); // Small delay to prevent watchdog triggers
                }
            }
//===============================MAIN=========================
extern "C" void app_main()
{
    initArduino();

    Serial.begin(115200);
    delay(500);                     // Give the terminal time to connect

    if(!SPIFFS.begin(true)){printf("SPIFFS Mount Failed\n");} else {
        printf("SPIFFS Mounted successfully\n");
    }

    wifiSemaphore = xSemaphoreCreateCounting(1, 1);

    uint8_t mac[6];                 //Read MAC to make unique full_device_name
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(full_device_name, sizeof(full_device_name), "%s_%02X%02X%02X", DEVICE_NAME_PREFIX, mac[3], mac[4], mac[5]);

 // 1. Initialize the Matrix
  matrix.addLayer(&backgroundLayer); 
  matrix.addLayer(&scrollingLayer); 
  matrix.begin();
  matrix.setBrightness(defaultBrightness);

  scrollingLayer.setOffsetFromTop(STATUS_Y);
  scrollingLayer.setMode(wrapForward); // Continues in one direction
  scrollingLayer.setSpeed(20);         // Lower is faster
  scrollingLayer.setFont(DOW_FONT);    //Date Font defined in config
  scrollingLayer.setColor(STATUS_COLOR);

  backgroundLayer.enableColorCorrection(true);
  backgroundLayer.setFont(DOW_FONT);    //Date Font defined in config

  WiFi.onEvent(WiFiEvent);
  #if USE_MQTT
  initMqtt();
  #endif

  xTaskCreatePinnedToCore(wifiTask, "WiFiTask", 4096, NULL, 1, &wifiTaskHandle, 1);

    drawCenteredTwoLineText("Connecting to WiFi");
    backgroundLayer.swapBuffers();

    connectToWifi(); 

    drawCenteredTwoLineText(statusMessage); //display IPadress
    backgroundLayer.swapBuffers();

// 2. Initial Time Sync
    configTzTime(TIMEZONE, "pool.ntp.org", "time.nist.gov");
    lastNTPUpdate = millis();
    vTaskDelay(pdMS_TO_TICKS(500));

    backgroundLayer.setFont(font5x7);       //Change font now to smaller after IP display

// 2. Create Background Tasks
    xTaskCreatePinnedToCore(clockLogicTask, "ClockLogic", 4096, NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(animationTask, "AnimTask", 4096, NULL, 1, &animationTaskHandle, 1);
    xTaskCreatePinnedToCore(adviceTask, "AdviceTask", 4096, NULL, 1, &adviceTaskHandle, 0);
    
    rainTimer = xTimerCreate("RainTimer", pdMS_TO_TICKS(MATRIX_RAIN_MS), pdFALSE, (void*)0, stopRainCallback);

//3. Init weather updates
    xTaskCreatePinnedToCore(weatherTask, "WeatherTask", 4096, NULL, 1, &weatherTaskHandle, 0);
    if (weatherTaskHandle != NULL) xTaskNotifyGive(weatherTaskHandle);
    lastWeatherUpdate = millis();
    vTaskDelay(pdMS_TO_TICKS(500));

//4. Memory safety task
    xTaskCreate(memory_monitor_task, "mem_mon", 2048, NULL, 1, NULL);
    
    Serial.println(full_device_name);

}

//================================HELPER functions============================

void drawCenteredTwoLineText(const char* text) {
  static char cachedText[256];
  static char lines[32][32];
  static uint8_t lineCount = 0;
  static uint8_t page = 0;
  static unsigned long lastSwitch = 0;

  const unsigned long PAGE_TIME = 2500;

  const int FONT_W = 6;
  const int FONT_H = 8;
  const int SCREEN_W = kMatrixWidth;
  const int SCREEN_H = kMatrixHeight;

  const int MAX_CHARS = SCREEN_W / FONT_W;
  const int LINE_GAP = 1;
  const int Y_OFFSET = -2;

  const int CENTER_Y = (SCREEN_H - FONT_H) / 2 + Y_OFFSET;
  const int LINE1_Y  = (SCREEN_H - (FONT_H * 2 + LINE_GAP)) / 2 + Y_OFFSET;
  const int LINE2_Y  = LINE1_Y + FONT_H + LINE_GAP;

  // ---------- rebuild only if text changed ----------
  if (strncmp(text, cachedText, sizeof(cachedText) - 1) != 0) {
    strncpy(cachedText, text, sizeof(cachedText) - 1);
    cachedText[sizeof(cachedText) - 1] = '\0';

    memset(lines, 0, sizeof(lines));
    lineCount = 0;

    uint8_t col = 0;
    const char* p = cachedText;

    while (*p && lineCount < 32) {
      char word[32];
      uint8_t wl = 0;

      // extract word
      while (*p && *p != ' ' && wl < sizeof(word) - 1) {
        word[wl++] = *p++;
      }
      word[wl] = '\0';
      if (*p == ' ') p++;

      // hyphenate only if longer than full line
      while (wl > MAX_CHARS && lineCount < 32) {
        memcpy(lines[lineCount], word, MAX_CHARS - 1);
        lines[lineCount][MAX_CHARS - 1] = ' '; // '-' >can be hyphen sign
        lines[lineCount][MAX_CHARS] = '\0';
        lineCount++;

        memmove(word, word + MAX_CHARS - 1, wl - (MAX_CHARS - 1));
        wl -= (MAX_CHARS - 1);
        word[wl] = '\0';
        col = 0;
      }

      // fit word into line
      if (col == 0) {
        memcpy(lines[lineCount], word, wl);
        col = wl;
      }
      else if (col + 1 + wl <= MAX_CHARS) {
        lines[lineCount][col++] = ' ';
        memcpy(&lines[lineCount][col], word, wl);
        col += wl;
      }
      else {
        lineCount++;
        col = 0;
        if (lineCount >= 32) break;
        memcpy(lines[lineCount], word, wl);
        col = wl;
      }
    }

    lineCount++;
    page = 0;
  }

  // ---------- page timing ----------
  if (millis() - lastSwitch > PAGE_TIME) {
    page++;
    lastSwitch = millis();
  }

  int pages = (lineCount + 1) / 2;
  int idx = (page % pages) * 2;

  // ---------- render ----------------
  backgroundLayer.fillScreen(defaultBackgroundColor);
  backgroundLayer.setFont(gohufont11);
  rgb24 color = {100, 100, 100};

  auto drawLine = [&](int i, int y) {
    if (i >= lineCount) return;
    int len = strlen(lines[i]);
    int x = (SCREEN_W - len * FONT_W) / 2;
    backgroundLayer.drawString(x, y, color, lines[i]);
  };

  // single line
  if (idx + 1 >= lineCount) {
    drawLine(idx, CENTER_Y);
  }
  // two lines
  else {
    drawLine(idx,     LINE1_Y);
    drawLine(idx + 1, LINE2_Y);
  }
}

void fadeOutMatrix() {
    for (int b = defaultBrightness; b >= 0; b -= (defaultBrightness / 10)) {
        matrix.setBrightness(b); 
        vTaskDelay(pdMS_TO_TICKS(100)); 
    }
}

void scrollAdvice(){
    if (adviceValid ) {
    adviceValid = false;
    isAdviceScrolling = true; 
    clearStatusMessage();
    scrollingLayer.stop();
    scrollingLayer.start(currentAdvice, 2);   
    }

    if (isAdviceScrolling && scrollingLayer.getStatus() == 0) {
    isAdviceScrolling = false;
}

}

void safetyrestart(){
    // If time is 04:00:00
if (timeinfo.tm_hour == 4 && timeinfo.tm_min == 0 && timeinfo.tm_sec <= 5) {
    logStatusMessage("SafetyRestart");
    vTaskDelay(pdMS_TO_TICKS(3000)); 
    esp_restart();
}

}

void getadvice() {
    // 1. Check if it's exactly 19:00
    if (timeinfo.tm_hour == 19 && timeinfo.tm_min == 0) {
         static int lastTriggerDay = -1; // Stores the day of the month (1-31)

        // 2. Only trigger if we haven't already triggered today
        if (timeinfo.tm_mday != lastTriggerDay) {
            xTaskNotifyGive(adviceTaskHandle);
            lastTriggerDay = timeinfo.tm_mday; // Lock it for the rest of today
           // Serial.printf("[Clock] Daily Advice Triggered at %02d:%02d\n", 
                         // timeinfo.tm_hour, timeinfo.tm_min);
        }
    }
}

void nightmode() {

    uint8_t targetBrightness;
    static uint8_t lastSetBrightness = 255;

    if (timeinfo.tm_hour < 1) {
        targetBrightness = 37;
    } else if (timeinfo.tm_hour < 2) {
        targetBrightness = 25;
    } else if (timeinfo.tm_hour >= 6) {
        targetBrightness = 255;
    } else {
        // Handle the gap between 02:00 and 06:00 (optional but safe)
        return; 
    }

            if (targetBrightness != lastSetBrightness) {
            matrix.setBrightness(targetBrightness);
            lastSetBrightness = targetBrightness;
    }
}