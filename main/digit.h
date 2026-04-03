#pragma once
#ifndef GPIOPINOUT
    #define GPIOPINOUT ESP32_FORUM_PINOUT 
#endif
#include "SmartMatrix.h" 
#include <Arduino.h>
#include "config.h"


class Digit {

  public:
    Digit(byte value, uint16_t xo, uint16_t yo, rgb24 color);
    
    void Draw(byte value);
    void Morph(byte newValue);
    byte Value();
    void DrawColon(rgb24 c);
    void updateAnimation();    
    bool isMorphing() { return _isMorphing; }

  private:
    SMLayerBackground<rgb24, 0> * backgroundLayer;
    byte   _value;
    rgb24  _color;
    uint16_t xOffset;
    uint16_t yOffset;
    uint8_t _animStep = 0;
    int8_t _targetValue = -1;
    bool _isMorphing = false;

    int animSpeed = CLOCK_ANIMATION_DELAY_MSEC;

    // Drawing primitives (SmartMatrix backend)
    void drawPixel(uint16_t x, uint16_t y, rgb24 c);
    void drawFillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, rgb24 c);
    void drawLine(uint16_t x, uint16_t y, uint16_t x2, uint16_t y2, rgb24 c);

    // Segment & morph helpers
    void drawSeg(byte seg);
    void Morph0_Frame(int i);
    void Morph1_Frame(int i);
    void Morph2_Frame(int i);
    void Morph3_Frame(int i);
    void Morph4_Frame(int i);
    void Morph5_Frame(int i);
    void Morph6_Frame(int i);
    void Morph7_Frame(int i);
    void Morph8_Frame(int i);
    void Morph9_Frame(int i);
};
