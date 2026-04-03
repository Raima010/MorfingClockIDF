#include "matrix_globals.h"
#include "globals.h"
#include "digit.h"

static const rgb24 black = {0, 0, 0};

// Segment indices
const byte sA = 0;
const byte sB = 1;
const byte sC = 2;
const byte sD = 3;
const byte sE = 4;
const byte sF = 5;
const byte sG = 6;

const int segHeight = CLOCK_SEGMENT_HEIGHT;
const int segWidth  = CLOCK_SEGMENT_WIDTH;
const uint16_t height = PANEL_HEIGHT - 1;
const uint16_t width  = PANEL_WIDTH - 1;

// Segment bit patterns
byte digitBits[] = {
  B11111100, // 0 ABCDEF--
  B01100000, // 1 -BC-----
  B11011010, // 2 AB-DE-G-
  B11110010, // 3 ABCD--G-
  B01100110, // 4 -BC--FG-
  B10110110, // 5 A-CD-FG-
  B10111110, // 6 A-CDEFG-
  B11100000, // 7 ABC-----
  B11111110, // 8 ABCDEFG-
  B11110110  // 9 ABCD-FG-
};


void Digit::Morph(byte newValue) {
  if (newValue == _value) return;
  _targetValue = newValue;
  _animStep = 0;
  _isMorphing = true;
}

// ---- Constructor ----

Digit::Digit(byte value, uint16_t xo, uint16_t yo, rgb24 color) {
  _value   = value;
  xOffset  = xo;
  yOffset  = yo;
  _color   = color;
}

// ---- Getter ----
byte Digit::Value() {
  return _value;
}

// ---- Drawing primitives ----
void Digit::drawPixel(uint16_t x, uint16_t y, rgb24 c) {
  ::backgroundLayer.drawPixel(xOffset + x, height - (y + yOffset), c);
}

void Digit::drawLine(uint16_t x, uint16_t y, uint16_t x2, uint16_t y2, rgb24 c) {
  ::backgroundLayer.drawLine(
    xOffset + x,
    height - (y + yOffset),
    xOffset + x2,
    height - (y2 + yOffset),
    c
  );
}

void Digit::drawFillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, rgb24 c) {
  ::backgroundLayer.fillRectangle(
    xOffset + x,
    height - (y + yOffset),
    xOffset + x + w - 1,
    height - (y + yOffset) + h - 1,
    c
  );
}

// ---- Colon ----
void Digit::DrawColon(rgb24 c) {
  drawFillRect(-2 - (int)(CLOCK_SEGMENT_SPACING / 2), segHeight - 1, 2, 2, c);
  drawFillRect(-2 - (int)(CLOCK_SEGMENT_SPACING / 2), segHeight + 1 + 3, 2, 2, c);
}

// ---- Segment drawing ----
void Digit::drawSeg(byte seg) {
  switch (seg) {
    case sA: drawLine(1, segHeight * 2 + 2, segWidth, segHeight * 2 + 2, _color); break;
    case sB: drawLine(segWidth + 1, segHeight * 2 + 1, segWidth + 1, segHeight + 2, _color); break;
    case sC: drawLine(segWidth + 1, 1, segWidth + 1, segHeight, _color); break;
    case sD: drawLine(1, 0, segWidth, 0, _color); break;
    case sE: drawLine(0, 1, 0, segHeight, _color); break;
    case sF: drawLine(0, segHeight * 2 + 1, 0, segHeight + 2, _color); break;
    case sG: drawLine(1, segHeight + 1, segWidth, segHeight + 1, _color); break;
  }
}

// ---- Static draw ----
void Digit::Draw(byte value) {
  byte pattern = digitBits[value];
  if (bitRead(pattern, 7)) drawSeg(sA);
  if (bitRead(pattern, 6)) drawSeg(sB);
  if (bitRead(pattern, 5)) drawSeg(sC);
  if (bitRead(pattern, 4)) drawSeg(sD);
  if (bitRead(pattern, 3)) drawSeg(sE);
  if (bitRead(pattern, 2)) drawSeg(sF);
  if (bitRead(pattern, 1)) drawSeg(sG);
  _value = value;
}

// ---- Morph functions ----
void Digit::updateAnimation() {
  if (!_isMorphing) return;
  
  // Direct frame drawing based on target
  switch (_targetValue) {
    case 0: Morph0_Frame(_animStep); break;
    case 1: Morph1_Frame(_animStep); break;
    case 2: Morph2_Frame(_animStep); break;
    case 3: Morph3_Frame(_animStep); break;
    case 4: Morph4_Frame(_animStep); break;
    case 5: Morph5_Frame(_animStep); break;
    case 6: Morph6_Frame(_animStep); break;
    case 7: Morph7_Frame(_animStep); break;
    case 8: Morph8_Frame(_animStep); break;
    case 9: Morph9_Frame(_animStep); break;
  }

  _animStep++;
  if (_animStep > segWidth + 1) {
    _value = _targetValue;
    _isMorphing = false;
  }
}

void Digit::Morph1_Frame(int i) {
  if (i <= (segWidth + 1)) {
    drawLine(0 + i - 1, 1, 0 + i - 1, segHeight, black);
    drawLine(0 + i, 1, 0 + i, segHeight, _color);
    drawLine(0 + i - 1, segHeight * 2 + 1, 0 + i - 1, segHeight + 2, black);
    drawLine(0 + i, segHeight * 2 + 1, 0 + i, segHeight + 2, _color);
    drawPixel(1 + i, segHeight * 2 + 2, black);
    drawPixel(1 + i, 0, black);
    drawPixel(1 + i, segHeight + 1, black);
  }
}

void Digit::Morph2_Frame(int i) {
  if (i <= segWidth) {
    if (i < segWidth) {
      drawPixel(segWidth - i, segHeight * 2 + 2, _color);
      drawPixel(segWidth - i, segHeight + 1, _color);
      drawPixel(segWidth - i, 0, _color);
    }
    drawLine(segWidth + 1 - i, 1, segWidth + 1 - i, segHeight, black);
    drawLine(segWidth - i, 1, segWidth - i, segHeight, _color);
  }
}

void Digit::Morph3_Frame(int i) {
  if (i <= segWidth) {
    drawLine(0 + i, 1, 0 + i, segHeight, black);
    drawLine(1 + i, 1, 1 + i, segHeight, _color);
  }
}

void Digit::Morph4_Frame(int i) {
  if (i < segWidth) {
    drawPixel(segWidth - i, segHeight * 2 + 2, black);
    drawPixel(0, segHeight * 2 + 1 - i, _color);
    drawPixel(1 + i, 0, black);
  }
}

void Digit::Morph5_Frame(int i) {
  if (i < segWidth) {
    drawPixel(segWidth + 1, segHeight + 2 + i, black);
    drawPixel(segWidth - i, segHeight * 2 + 2, _color);
    drawPixel(segWidth - i, 0, _color);
  }
}

void Digit::Morph6_Frame(int i) {
  if (i <= segWidth) {
    drawLine(segWidth - i, 1, segWidth - i, segHeight, _color);
    if (i > 0) drawLine(segWidth - i + 1, 1, segWidth - i + 1, segHeight, black);
  }
}

void Digit::Morph7_Frame(int i) {
  if (i <= (segWidth + 1)) {
    drawLine(0 + i - 1, 1, 0 + i - 1, segHeight, black);
    drawLine(0 + i, 1, 0 + i, segHeight, _color);
    drawLine(0 + i - 1, segHeight * 2 + 1, 0 + i - 1, segHeight + 2, black);
    drawLine(0 + i, segHeight * 2 + 1, 0 + i, segHeight + 2, _color);
    drawPixel(1 + i, 0, black);
    drawPixel(1 + i, segHeight + 1, black);
  }
}

void Digit::Morph8_Frame(int i) {
  if (i <= segWidth) {
    drawLine(segWidth - i, segHeight * 2 + 1, segWidth - i, segHeight + 2, _color);
    if (i > 0) drawLine(segWidth - i + 1, segHeight * 2 + 1, segWidth - i + 1, segHeight + 2, black);
    drawLine(segWidth - i, 1, segWidth - i, segHeight, _color);
    if (i > 0) drawLine(segWidth - i + 1, 1, segWidth - i + 1, segHeight, black);
    if (i < segWidth) {
      drawPixel(segWidth - i, 0, _color);
      drawPixel(segWidth - i, segHeight + 1, _color);
    }
  }
}

void Digit::Morph9_Frame(int i) {
  if (i <= (segWidth + 1)) {
    drawLine(0 + i - 1, 1, 0 + i - 1, segHeight, black);
    drawLine(0 + i, 1, 0 + i, segHeight, _color);
  }
}

void Digit::Morph0_Frame(int i) {
  if (i <= segWidth) {
    if (_value == 1) {
      drawLine(segWidth - i, segHeight * 2 + 1, segWidth - i, segHeight + 2, _color);
      if (i > 0) drawLine(segWidth - i + 1, segHeight * 2 + 1, segWidth - i + 1, segHeight + 2, black);
      drawLine(segWidth - i, 1, segWidth - i, segHeight, _color);
      if (i > 0) drawLine(segWidth - i + 1, 1, segWidth - i + 1, segHeight, black);
      if (i < segWidth) drawPixel(segWidth - i, segHeight * 2 + 2, _color);
      if (i < segWidth) drawPixel(segWidth - i, 0, _color);
    }
    if (_value == 2) {
      drawLine(segWidth - i, segHeight * 2 + 1, segWidth - i, segHeight + 2, _color);
      if (i > 0) drawLine(segWidth - i + 1, segHeight * 2 + 1, segWidth - i + 1, segHeight + 2, black);
      drawPixel(1 + i, segHeight + 1, black);
      if (i < segWidth) drawPixel(segWidth + 1, segHeight + 1 - i, _color);
    }
    if (_value == 3) {
      drawLine(segWidth - i, segHeight * 2 + 1, segWidth - i, segHeight + 2, _color);
      if (i > 0) drawLine(segWidth - i + 1, segHeight * 2 + 1, segWidth - i + 1, segHeight + 2, black);
      drawLine(segWidth - i, 1, segWidth - i, segHeight, _color);
      if (i > 0) drawLine(segWidth - i + 1, 1, segWidth - i + 1, segHeight, black);
      drawPixel(segWidth - i, segHeight + 1, black);
    }
    if (_value == 5) {
      if (i < segWidth) {
        if (i > 0) drawLine(1 + i, segHeight * 2 + 1, 1 + i, segHeight + 2, black);
        drawLine(2 + i, segHeight * 2 + 1, 2 + i, segHeight + 2, _color);
      }
    }
    if (_value == 5 || _value == 9) {
      if (i < segWidth) drawPixel(segWidth - i, segHeight + 1, black);
      if (i < segWidth) drawPixel(0, segHeight - i, _color);
    }
  }
}
