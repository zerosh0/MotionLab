#ifndef UI_CANVAS_H
#define UI_CANVAS_H

#include "raylib.h"



struct VideoEngine;
struct UIState;
struct TrackingSystem;
struct AutoTracker;

void UpdateVideoCanvas(struct VideoEngine *v, struct UIState *ui, struct TrackingSystem *ts, struct AutoTracker *autoTracker, Rectangle area);
void DrawVideoCanvas(struct VideoEngine *v, struct UIState *ui, struct TrackingSystem *ts, struct AutoTracker *autoTracker, Rectangle area);
void DrawMagnifierWindow(struct VideoEngine *v, struct UIState *ui, Rectangle videoArea);
#endif