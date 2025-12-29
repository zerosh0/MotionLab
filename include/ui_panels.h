#ifndef UI_PANELS_H
#define UI_PANELS_H

#include "raylib.h"

struct AutoTracker; 
struct UIState;
struct TrackingSystem;
struct VideoEngine;

bool GuiButton(struct UIState *ui, Rectangle bounds, const char* text);
void DrawSidebar(struct UIState *ui);
void DrawRightPanel(struct UIState *ui, struct TrackingSystem *ts, struct VideoEngine *v, struct AutoTracker *tracker, const char* filename);
void DrawBottomBar(struct UIState *ui, struct VideoEngine *v, Rectangle videoArea);
void DrawHelpWindow(struct UIState *ui);

#endif