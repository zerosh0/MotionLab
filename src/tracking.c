#include "tracking.h"
#include <math.h>

Vector2 PixelToPhysical(struct TrackingSystem *ts, Vector2 pixelPos) {
    if (!ts->calib.hasOriginSet || ts->calib.pxPerMeter <= 0) {
        return pixelPos; 
    }

    float pxPerM = ts->calib.pxPerMeter;
    float dxPx = pixelPos.x - ts->calib.origin.x;
    float dyPx = pixelPos.y - ts->calib.origin.y;

    int dirX = (ts->calib.config == AXIS_X_RIGHT_Y_UP || ts->calib.config == AXIS_X_RIGHT_Y_DOWN) ? 1 : -1;
    int dirY = (ts->calib.config == AXIS_X_RIGHT_Y_DOWN || ts->calib.config == AXIS_X_LEFT_Y_DOWN) ? 1 : -1;

    float valX = (dxPx * dirX) / pxPerM;
    float valY = (dyPx * dirY) / pxPerM;

    return (Vector2){ valX, valY };
}

void Tracking_Init(TrackingSystem *ts) {
    ts->calib.origin = (Vector2){0, 0};
    ts->calib.config = AXIS_X_RIGHT_Y_UP;
    ts->calib.isSettingOrigin = false;
    
    ts->calib.scalePointA = (Vector2){0, 0};
    ts->calib.scalePointB = (Vector2){100, 0};
    ts->calib.isSettingScale = false;
    ts->calib.scaleStep = 0;
    ts->calib.realDistance = 1.0f;
    ts->calib.pxPerMeter = 0.0f;
    ts->calib.hasOriginSet = false;
    ts->startFrame=0;
}

Vector2 ScreenToVideo(Vector2 screenPos, Rectangle destRect, float sourceW, float sourceH) {
    float scaleX = sourceW / destRect.width;
    float scaleY = sourceH / destRect.height;
    
    float vidX = (screenPos.x - destRect.x) * scaleX;
    float vidY = (screenPos.y - destRect.y) * scaleY;
    
    return (Vector2){ vidX, vidY };
}

Vector2 VideoToScreen(Vector2 videoPos, Rectangle destRect, float sourceW, float sourceH) {
    float scaleX = destRect.width / sourceW;
    float scaleY = destRect.height / sourceH;
    
    float scrX = destRect.x + (videoPos.x * scaleX);
    float scrY = destRect.y + (videoPos.y * scaleY);
    
    return (Vector2){ scrX, scrY };
}

void Tracking_AddPoint(TrackingSystem *ts, double time, Vector2 videoPos) {
    int existingIdx = -1;
    for (int i = 0; i < ts->count; i++) {
        if (fabs(ts->points[i].time - time) < 0.001) {
            existingIdx = i;
            break;
        }
    }

    if (existingIdx != -1) {
        ts->points[existingIdx].pixelPos = videoPos;
    } else {
        if (ts->count < MAX_POINTS) {
            ts->points[ts->count].time = time;
            ts->points[ts->count].pixelPos = videoPos;
            ts->count++;
        }
    }
}