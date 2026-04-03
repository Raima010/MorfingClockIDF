#pragma once

// DEVICE
#define DEVICE_NAME_PREFIX "MorfingClock"

// MQTT
#define USE_MQTT 0                              // 1- MQTT enabled, set credentials in main.cpp

// TIME
#define NTP_REFRESH_INTERVAL_SEC 28800          // How often we refresh the time from the NTP server - 8h
#define TIMEZONE "CET-1CEST,M3.5.0,M10.5.0/3"   //Check for your country GMT and daylight savings string

//Define MATRIX
#define MATRIX_RAIN_MS 15000                    //Matrix rain time

// STATUS MESSAGE
#define LOG_MESSAGE_PERSISTENCE_MSEC 2000       // How long are informational messages kept on screen

// Screen positioning settings
// Panel size
#define PANEL_WIDTH 64
#define PANEL_HEIGHT 32

// Clock
#define CLOCK_X 2
#define CLOCK_Y 8
#define CLOCK_SEGMENT_HEIGHT 6
#define CLOCK_SEGMENT_WIDTH 6
#define CLOCK_SEGMENT_SPACING 3
#define CLOCK_WIDTH 6*(CLOCK_SEGMENT_WIDTH+CLOCK_SEGMENT_SPACING)+4
#define CLOCK_HEIGHT 2*CLOCK_SEGMENT_HEIGHT+3
#define CLOCK_DIGIT_COLOR {150, 0, 0}
#define CLOCK_ANIMATION_DELAY_MSEC 20            //Delay in ms for clock animation - should be below 30ms for a segment size of 8

// Day of week
#define DOW_X 2
#define DOW_Y 0
#define DOW_COLOR {0x00, 0x40, 0x10}
#define DOW_FONT font5x7
// Date
#define DATE_X DOW_X + 20
#define DATE_Y DOW_Y
#define DATE_COLOR DOW_COLOR
#define DATE_HEIGHT 7
#define DATE_WIDTH 32

// Log messages at the bottom
#define STATUS_X 0
#define STATUS_Y 25                                // Position for the bottom line
#define STATUS_WIDTH 64
#define STATUS_HEIGHT 8
#define STATUS_COLOR {100,100,100}                 // Status message color