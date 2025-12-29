#ifndef TRACKING_H
#define TRACKING_H

#include "raylib.h"

#define MAX_POINTS 4096

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    AXIS_X_RIGHT_Y_UP = 0,
    AXIS_X_RIGHT_Y_DOWN,
    AXIS_X_LEFT_Y_UP,
    AXIS_X_LEFT_Y_DOWN
} AxisConfiguration;


typedef struct {
    double time;
    Vector2 pixelPos;
    Vector2 worldPos;
} MeasurePoint;



typedef struct Calibration {
    Vector2 origin;
    AxisConfiguration config;
    bool isSettingOrigin;
    bool hasOriginSet;
    Vector2 scalePointA;
    Vector2 scalePointB;
    bool isSettingScale;
    int scaleStep;
    float realDistance;
    float pxPerMeter;
} Calibration;

typedef struct TrackingSystem {
    MeasurePoint points[MAX_POINTS];
    int count;
    
    Vector2 origin;
    float scale;
    bool isCalibrated;
    int startFrame;
    Calibration calib;
} TrackingSystem;


void Tracking_Init(TrackingSystem *ts);
Vector2 ScreenToVideo(Vector2 screenPos, Rectangle destRect, float sourceW, float sourceH);
Vector2 VideoToScreen(Vector2 videoPos, Rectangle destRect, float sourceW, float sourceH);
Vector2 PixelToPhysical(struct TrackingSystem *ts, Vector2 pixelPos);
void Tracking_AddPoint(TrackingSystem *ts, double time, Vector2 videoPos);
#ifdef __cplusplus
}
#endif

#endif