#include "Arduino.h"
#include "SmartMatrix.h"
#include "matrix_globals.h"
#include "weather.h"
#include "globals.h"
#include "digit.h"
#include "clock.h"
#include "config.h"



extern void fadeOutMatrix();
bool fadeNecessary = false;  //jau nenaudojamas, trinti?

//-------------Matrix rain
#define MAX_DROPS 15

struct Drop {
  int x, y, speed, len;
};
Drop drops[MAX_DROPS];

void initMatrixRain() {
  for (int i = 0; i < MAX_DROPS; i++) {
    drops[i].x = random(0, 64);
    drops[i].y = random(-64, 0);
    drops[i].speed = random(2, 5);
    drops[i].len = random(10, 30);
  }
}
//-------------------------
void drawMatrixRain() {
  // 1. Dim the screen slightly for the "trail" effect
  backgroundLayer.fillScreen({0, 0, 0}); // Or use a dimming function if you have one

  for (int i = 0; i < MAX_DROPS; i++) {
    // Draw the tail (fading green)
    for (int j = 0; j < drops[i].len; j++) {
      int py = drops[i].y - j;
      if (py >= 0 && py < 64) {
        uint8_t br = 255 - (j * (255 / drops[i].len));
        backgroundLayer.drawPixel(drops[i].x, py, {0, br, 0});
      }
      vTaskDelay(0);
    }
    
    // Draw the bright "head" (white-ish green)
    if (drops[i].y >= 0 && drops[i].y < 64) {
      backgroundLayer.drawPixel(drops[i].x, drops[i].y, {150, 255, 150});
    }

    // Move drop
    drops[i].y += drops[i].speed;

    // Reset if it leaves the screen
    if (drops[i].y - drops[i].len > 64) {
      drops[i].y = -random(10, 50);
      drops[i].x = random(0, 64);
    }
  }
  //backgroundLayer.swapBuffers();
}
//-------------------------
const char* dayShortNames[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
unsigned long messageDisplayMillis = 0;
bool logMessageActive = false;

// NOTE:
// MorphingClock Y axis is inverted, so we keep the same math
Digit digit5(0, CLOCK_X,                                                 PANEL_HEIGHT - CLOCK_Y - 2*(CLOCK_SEGMENT_HEIGHT) - 3, CLOCK_DIGIT_COLOR);
Digit digit4(0, CLOCK_X + (CLOCK_SEGMENT_WIDTH + CLOCK_SEGMENT_SPACING), PANEL_HEIGHT - CLOCK_Y - 2*(CLOCK_SEGMENT_HEIGHT) - 3, CLOCK_DIGIT_COLOR);
Digit digit3(0, CLOCK_X + 2*(CLOCK_SEGMENT_WIDTH + CLOCK_SEGMENT_SPACING) + 3, PANEL_HEIGHT - CLOCK_Y - 2*(CLOCK_SEGMENT_HEIGHT) - 3, CLOCK_DIGIT_COLOR);
Digit digit2(0, CLOCK_X + 3*(CLOCK_SEGMENT_WIDTH + CLOCK_SEGMENT_SPACING) + 3, PANEL_HEIGHT - CLOCK_Y - 2*(CLOCK_SEGMENT_HEIGHT) - 3, CLOCK_DIGIT_COLOR);
Digit digit1(0, CLOCK_X + 4*(CLOCK_SEGMENT_WIDTH + CLOCK_SEGMENT_SPACING) + 6, PANEL_HEIGHT - CLOCK_Y - 2*(CLOCK_SEGMENT_HEIGHT) - 3, CLOCK_DIGIT_COLOR);
Digit digit0(0, CLOCK_X + 5*(CLOCK_SEGMENT_WIDTH + CLOCK_SEGMENT_SPACING) + 6, PANEL_HEIGHT - CLOCK_Y - 2*(CLOCK_SEGMENT_HEIGHT) - 3, CLOCK_DIGIT_COLOR);

static int prevss = -1;
static int prevmm = -1;
static int prevhh = -1;

void displayClock() {

    int hh = timeinfo.tm_hour;
    int mm = timeinfo.tm_min;
    int ss = timeinfo.tm_sec;

    // Let SmartMatrix handle refresh timing
    // (do NOT delay here unless absolutely needed)

    if (clockStartingUp) {

        backgroundLayer.fillScreen({0, 0, 0});
        backgroundLayer.swapBuffers(true);
        matrix.setBrightness(defaultBrightness);   

        digit0.Draw(ss % 10);
        digit1.Draw(ss / 10);
        digit2.Draw(mm % 10);
        digit3.Draw(mm / 10);
        digit4.Draw(hh % 10);
        digit5.Draw(hh / 10);

        digit1.DrawColon(CLOCK_DIGIT_COLOR);
        digit3.DrawColon(CLOCK_DIGIT_COLOR);

        displayDate();
        drawNowWeather();

        prevss = ss;
        prevmm = mm;
        prevhh = hh;

        clockStartingUp = false;
        return;
    }

    // Seconds
    if (ss != prevss) {
        int s0 = ss % 10;
        int s1 = ss / 10;
        if (s0 != digit0.Value()) digit0.Morph(s0);
        if (s1 != digit1.Value()) digit1.Morph(s1);
        drawNowWeather(); //evry 1s - ensure redraw after messages
        prevss = ss;
    }

    // Minutes
    if (mm != prevmm) {
        int m0 = mm % 10;
        int m1 = mm / 10;
        if (m0 != digit2.Value()) digit2.Morph(m0);
        if (m1 != digit3.Value()) digit3.Morph(m1);
        displayDate();
        prevmm = mm;
    }

    // Hours
    if (hh != prevhh) {
        int h0 = hh % 10;
        int h1 = hh / 10;
        if (h0 != digit4.Value()) digit4.Morph(h0);
        if (h1 != digit5.Value()) digit5.Morph(h1);
        prevhh = hh;
    }
}

void displayDate() {
    // Set date font & Clear date area
    backgroundLayer.fillRectangle(
        DOW_X,
        DOW_Y,
        DOW_X + DATE_WIDTH - 1,
        DOW_Y + DATE_HEIGHT - 1,
        {0, 0, 0}
    );

    // Day of week
    backgroundLayer.drawString(
        DOW_X,
        DOW_Y,
        DATE_COLOR,
        dayShortNames[timeinfo.tm_wday]
    );

    // Date (DD.MM)
    char dateBuf[6];
    snprintf(dateBuf, sizeof(dateBuf), "%02d.%02d",
             timeinfo.tm_mday,
             timeinfo.tm_mon + 1);

    backgroundLayer.drawString(
        DATE_X,
        DATE_Y,
        DATE_COLOR,
        dateBuf
    );
}

void logStatusMessage(const char* message) {

    if (isAdviceScrolling) return; 
    
    // 1. Output to Serial for debugging
    Serial.println(message);

    // 2. Clear the Status Area (The "Last Line")
    backgroundLayer.fillRectangle(
        STATUS_X, 
        STATUS_Y, 
        STATUS_X + STATUS_WIDTH - 1, 
        STATUS_Y + STATUS_HEIGHT - 1, 
        {0, 0, 0} // Black
    );

    int len = strlen(message); 
    int textWidth = (len * 5) - 1; 
    int centeredX = (STATUS_WIDTH - textWidth) / 2;
    if (centeredX < 0) centeredX = 0;// Safety check: If message is too long, start at 0

    // 3. Draw the Message
    backgroundLayer.drawString(
        centeredX, 
        STATUS_Y, 
        STATUS_COLOR, 
        message
    );

    // 4. Update Timers
    messageDisplayMillis = millis();
    logMessageActive = true;
}

void clearStatusMessage() {
    backgroundLayer.fillRectangle(
        STATUS_X, 
        STATUS_Y, 
        STATUS_X + STATUS_WIDTH - 1, 
        STATUS_Y + STATUS_HEIGHT - 1, 
        {0, 0, 0} // Black
    );
    logMessageActive = false;
}
//===================================Display task==================

void animationTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    for(;;) {

        if (currentClockMode == MODE_RAIN) {
            drawMatrixRain();
            backgroundLayer.swapBuffers();
            vTaskDelay(pdMS_TO_TICKS(30)); // Fast rain!
        } else {  

            if (clockStartingUp) {
                    while(clockStartingUp) {
                    vTaskDelay(pdMS_TO_TICKS(10)); 
                }
                vTaskDelay(pdMS_TO_TICKS(20));
            }

        digit0.updateAnimation();
        digit1.updateAnimation();
        digit2.updateAnimation();
        digit3.updateAnimation();
        digit4.updateAnimation();
        digit5.updateAnimation();

        backgroundLayer.swapBuffers();

        // 30 FPS (30ms) - Solid timing regulated by RTOS
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(30));
    }
        vTaskDelay(pdMS_TO_TICKS(1)); //Ensure WDT isnt trigered
  }
}
