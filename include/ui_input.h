#ifndef UI_INPUT_H
#define UI_INPUT_H

#include "raylib.h"




struct UIState; 

bool GuiFloatInput(struct UIState *ui, Rectangle bounds, float *value, const char* suffix);

#endif