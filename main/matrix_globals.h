#ifndef MATRIX_GLOBALS_H
#define MATRIX_GLOBALS_H

#include "SmartMatrix.h"

// 1. Constants (Must match your main.cpp exactly)
#define K_WIDTH 64
#define K_HEIGHT 32
#define K_DEPTH 36
#define K_PANEL SMARTMATRIX_HUB75_32ROW_MOD16SCAN
#define K_OPTIONS SMARTMATRIX_OPTIONS_NONE

// 2. Typedefs
typedef SmartMatrixHub75Refresh<K_DEPTH, K_WIDTH, K_HEIGHT, K_PANEL, K_OPTIONS> RefreshType;
typedef SmartMatrixHub75Calc<K_DEPTH, K_WIDTH, K_HEIGHT, K_PANEL, K_OPTIONS> MatrixType;

// 3. Global Declarations
extern SMLayerBackground<rgb24, 0> backgroundLayer;
extern SMLayerScrolling<rgb24, 0> scrollingLayer;
//extern SMLayerIndexed<rgb24, 0> indexedLayer;

extern RefreshType matrixRefresh;
extern MatrixType matrix;
extern uint8_t defaultBrightness;

#endif