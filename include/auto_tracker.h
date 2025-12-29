#ifndef AUTO_TRACKER_H
#define AUTO_TRACKER_H

#include "raylib.h"
#include "video_engine.h"
#include "tracking.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TRACKER_IDLE,
    TRACKER_SELECTING,
    TRACKER_INITIALIZING,
    TRACKER_READY,
    TRACKER_TRACKING,
    TRACKER_LOST
} TrackerState;

typedef struct AutoTracker {
    TrackerState state;
    Rectangle targetRect;
    Vector2 centerPos;
    Vector2 velocity;
    Color targetColor;
    float colorTolerance;
    float searchWindowScale;
    bool needsToAdvance;
    bool pendingInit;
    void* internal_ptr; 
    
} AutoTracker;

void AutoTracker_Init(AutoTracker *tracker);
void AutoTracker_Update(AutoTracker *tracker, VideoEngine *video, TrackingSystem *ts);
void AutoTracker_StartSelection(AutoTracker *tracker);
void AutoTracker_ConfirmSelection(AutoTracker *tracker, VideoEngine *video);
void AutoTracker_StartTracking(AutoTracker *tracker);
void AutoTracker_Stop(AutoTracker *tracker);
void AutoTracker_Cancel(AutoTracker *tracker);
void AutoTracker_Free(AutoTracker *tracker);
#ifdef __cplusplus
}
#endif

#endif