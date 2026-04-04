<<<<<<< HEAD
This is just another vertion of Morfing clock as initially written ny bogd (https://github.com/bogd/esp32-morphing-clock).
Uses SmartMatrix flicker-free Led matrix library
IDF code version
Compiled for HUB75 64x32 Lded matrix
Credits to all other authors as this is just modification of other original code adn porting to IDF

Prerequisites:

Hardware:
ESP-32 wroom
HUB75 Led matrix 64x32 (can me any but some code asdjustments needed then)
Power supply
Case optional

Software:
To compile code problem-free make sure compiler/python vertions are not very different as some functions can be deprecated.
IDF vertion 1.03
Python vertion 3.11.2
Precompiled .bin files can be found in /build directory, but pinout of your system must match: 
R1_PIN  GPIO_NUM_27
G1_PIN  GPIO_NUM_26
B1_PIN  GPIO_NUM_14
R2_PIN  GPIO_NUM_12
G2_PIN  GPIO_NUM_25
B2_PIN  GPIO_NUM_15
A_PIN   GPIO_NUM_32
B_PIN   GPIO_NUM_17
C_PIN   GPIO_NUM_33
D_PIN   GPIO_NUM_16
E_PIN   GPIO_NUM_5
LAT_PIN GPIO_NUM_2
OE_PIN  GPIO_NUM_4
CLK_PIN GPIO_NUM_0

When compiling your own configuration, change these values in MatrixHardware_ESP32_V0.h under "GPIOPINOUT == ESP32_FORUM_PINOUT"

Functions:
Morfing digits
Weather data/forecast via open server OpenMeteo, no subscription needed
Advice function (Tip of the day) which is is give once per day (feel free to modify) at 19:00
Eastern Egg via webpage when you click 5 times on SSID
Memory health tracking
Safety restart att night hours (memory defragmentation if used 24/7)
Dimming att night to lower luminescence
OTA update option via Web-page, no cable/dissassembly needed
Mqtt support, if used enable via config.h and enter you credentials in main.cpp. Messages are displayed on Status line.

Credits:
To all contributors of original code as this is just modification and own implementation of original code.
=======
# MorfingClockIDF
Morfing clock IDF code vertion using flicker free SmartMatrix LED matrix display library
>>>>>>> 1eb4633dd4d5153f5e9301103c6a3115e5800e5a
