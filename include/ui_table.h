#ifndef UI_TABLE_H
#define UI_TABLE_H

#include "raylib.h"


struct AutoTracker; 
struct UIState;
struct TrackingSystem;
struct VideoEngine;

void DrawMeasurementTable(struct UIState *ui, struct TrackingSystem *ts, struct VideoEngine *v, Rectangle bounds);
void DrawCommonSettingsWithTS(struct UIState *ui, struct TrackingSystem *ts, struct AutoTracker *tracker, Rectangle bounds);

#endif