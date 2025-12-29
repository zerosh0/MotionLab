#ifndef UI_ACTION_H
#define UI_ACTION_H

#include "raylib.h"

struct TrackingSystem;
struct VideoEngine;
struct AutoTracker;


void Action_ExportCSV(struct TrackingSystem *ts, const char* videoName);
void Action_ExportRegressi(struct TrackingSystem *ts, const char* videoName);
void Action_CopyClipboard(struct TrackingSystem *ts);
void Action_SaveProject(struct TrackingSystem *ts, const char* videoPath);
bool Action_LoadProject(struct TrackingSystem *ts, struct VideoEngine *v, char* outVideoPath);

#endif