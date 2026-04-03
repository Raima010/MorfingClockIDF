#pragma once
#include <Arduino.h>

void updateWeatherOpenMeteo();
void drawNowWeather();
void draw_weather_animations(int stp);
void weatherTask(void* parameter);
void adviceTask(void* parameter) ;
