#ifndef UI_MENU_H
#define UI_MENU_H

#include "raylib.h"
#include "ui_core.h"
#include "video_engine.h"
#include "tracking.h"
#include "auto_tracker.h"

typedef enum {
    MENU_NONE = -1,
    MENU_FILE,
    MENU_EXPORT,
    MENU_GRAPHS,
    MENU_HELP,
    MENU_COUNT
} MenuType;

void DrawMenuBar(UIState *ui, VideoEngine *v, TrackingSystem *ts, AutoTracker *at);
void HandleShortcuts(struct UIState *ui, struct VideoEngine *v, struct TrackingSystem *ts, struct AutoTracker *autoTracker);
#endif