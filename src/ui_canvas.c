#include "raylib.h"
#include "ui_canvas.h"
#include "theme.h"
#include "video_engine.h"
#include "ui_core.h"
#include "tracking.h"
#include <math.h>
#include "auto_tracker.h"
#include "lang.h"

void DrawOutlinedLine(Vector2 start, Vector2 end, float thick, Color color) {
    DrawLineEx(start, end, thick + 2.0f, (Color){0, 0, 0, 150}); 
    DrawLineEx(start, end, thick, color);
}

void DrawModernArrow(Vector2 endPos, Vector2 direction, Color color) {
    float arrowSize = 12.0f;
    Vector2 p1 = endPos;
    float angle = atan2f(direction.y, direction.x);
    Vector2 p2 = { endPos.x + cosf(angle + 2.6f) * arrowSize, endPos.y + sinf(angle + 2.6f) * arrowSize };
    Vector2 p3 = { endPos.x + cosf(angle - 2.6f) * arrowSize, endPos.y + sinf(angle - 2.6f) * arrowSize };
    DrawTriangle((Vector2){p1.x+1, p1.y+1}, (Vector2){p3.x+1, p3.y+1}, (Vector2){p2.x+1, p2.y+1}, (Color){0,0,0,150});
    DrawTriangle(p1, p3, p2, color); 
}

void DrawMagnifierWindow(struct VideoEngine *v, struct UIState *ui, Rectangle videoArea) {
    MagnifierState *mag = &ui->magnifier;
    if (!mag->isVisible || !v->isLoaded) return;

    int headerHeight = 30;
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    Vector2 mouse = GetMousePosition();
    Rectangle headerRectDraw = { mag->bounds.x, mag->bounds.y, mag->bounds.width, (float)headerHeight };
    Rectangle closeRectDraw  = { mag->bounds.x + mag->bounds.width - headerHeight, mag->bounds.y, (float)headerHeight, (float)headerHeight };
    
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, headerRectDraw) && !CheckCollisionPointRec(mouse, closeRectDraw)) {
        mag->isDragging = true;
        mag->dragOffset = (Vector2){ mouse.x - mag->bounds.x, mouse.y - mag->bounds.y };
    }
    if (mag->isDragging) {
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            mag->bounds.x = mouse.x - mag->dragOffset.x;
            mag->bounds.y = mouse.y - mag->dragOffset.y;
        } else mag->isDragging = false;
    }
    
    if (mag->bounds.x < 0) mag->bounds.x = 0;
    if (mag->bounds.y < TITLEBAR_HEIGHT) mag->bounds.y = (float)TITLEBAR_HEIGHT;

    DrawRectangleRoundedCustom(headerRectDraw, 0.4f, 10, (Color){40, 40, 40, 255}, true, true, false, false);
    DrawTextApp(ui, L(T_MAGNIFIER), (int)mag->bounds.x + 10, (int)mag->bounds.y + 7, 16, LIGHTGRAY);
    bool hoverClose = CheckCollisionPointRec(mouse, closeRectDraw);
    DrawRectangleRoundedCustom(closeRectDraw, 0.4f, 10, hoverClose?RED:BLANK, false, true, false, false);
    DrawTextApp(ui, "X", (int)closeRectDraw.x + 9, (int)closeRectDraw.y + 7, 16, WHITE);
    if (hoverClose && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) mag->isVisible = false;

    Rectangle contentRect = { mag->bounds.x, mag->bounds.y + headerHeight, mag->bounds.width, mag->bounds.height - headerHeight };
    DrawRectangleRec(contentRect, (Color){20, 20, 20, 255});
    

    float scale = fminf(videoArea.width / (float)v->width, videoArea.height / (float)v->height);
    float offsetX = videoArea.x + (videoArea.width - (float)v->width * scale) / 2.0f;
    float offsetY = videoArea.y + (videoArea.height - (float)v->height * scale) / 2.0f;
    float mVidX = (mouse.x - offsetX) / scale;
    float mVidY = (mouse.y - offsetY) / scale;
    

    Rectangle destV = { offsetX, offsetY, v->width*scale, v->height*scale };

    if (CheckCollisionPointRec(mouse, destV)) {
        float srcW = contentRect.width / mag->zoomLevel;
        float srcH = contentRect.height / mag->zoomLevel;
        
        BeginScissorMode((int)contentRect.x, (int)contentRect.y, (int)contentRect.width, (int)contentRect.height);
            DrawTexturePro(v->texture, (Rectangle){mVidX - srcW/2, mVidY - srcH/2, srcW, srcH}, contentRect, (Vector2){0,0}, 0.0f, WHITE);
            DrawLine((int)(contentRect.x+contentRect.width/2), (int)contentRect.y, (int)(contentRect.x+contentRect.width/2), (int)(contentRect.y+contentRect.height), (Color){255,255,255,100});
            DrawLine((int)contentRect.x, (int)(contentRect.y+contentRect.height/2), (int)(contentRect.x+contentRect.width), (int)(contentRect.y+contentRect.height/2), (Color){255,255,255,100});
        EndScissorMode();
    } else {
        const char* msg = L(T_OUT_OF_ZONE);
        int tw = MeasureText(msg, 14);
        DrawTextApp(ui, msg, (int)(contentRect.x + contentRect.width/2 - tw/2), (int)(contentRect.y + contentRect.height/2 - 7), 14, GRAY);
    }
    DrawRectangleLinesEx(contentRect, 1, (Color){60,60,60,255});
}


void UpdateVideoCanvas(struct VideoEngine *v, struct UIState *ui, struct TrackingSystem *ts, struct AutoTracker *autoTracker, Rectangle area) {
    if (!v->isLoaded || ui->isMenuOpen) return;

    float scale = fminf(area.width / (float)v->width, area.height / (float)v->height);
    float displayW = (float)v->width * scale;
    float displayH = (float)v->height * scale;
    float offsetX = area.x + (area.width - displayW) / 2.0f;
    float offsetY = area.y + (area.height - displayH) / 2.0f;
    Rectangle destRec = { offsetX, offsetY, displayW, displayH };

    Vector2 mouse = GetMousePosition();
    bool mouseInVideo = CheckCollisionPointRec(mouse, destRec);
    bool isOverMagnifier = (ui->magnifier.isVisible && CheckCollisionPointRec(mouse, ui->magnifier.bounds));
    if (!mouseInVideo || isOverMagnifier || ui->showGraphWindow) return;

    Vector2 mouseVideo = ScreenToVideo(mouse, destRec, (float)v->width, (float)v->height);

    if (ts->calib.isSettingOrigin && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        ts->calib.origin = mouseVideo;
        ts->calib.isSettingOrigin = false;
        ts->calib.hasOriginSet = true;
    }
    if (ts->calib.isSettingScale && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (ts->calib.scaleStep == 0) {
            ts->calib.scalePointA = mouseVideo;
            ts->calib.scalePointB = mouseVideo;
            ts->calib.scaleStep = 1;
        } else if (ts->calib.scaleStep == 1) {
            ts->calib.scalePointB = mouseVideo;
            ts->calib.scaleStep = 2;
            ts->calib.isSettingScale = false;
        }
    }
    if (ts->calib.isSettingScale && ts->calib.scaleStep == 1) {
        ts->calib.scalePointB = mouseVideo;
    }

    if (ui->currentTool == TOOL_POINT && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (!ts->calib.isSettingOrigin && !ts->calib.isSettingScale) {
            int currentFrame = (int)(v->currentTime * v->fps + 0.5);
            if (currentFrame >= ts->startFrame) {
                Tracking_AddPoint(ts, v->currentTime, mouseVideo);
                if (ui->autoAdvance) {
                    Video_NextFrame(v);
                    v->isPlaying = false;
                }
            }
        }
    }

    if (ui->currentTool == TOOL_TRACK) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (autoTracker->state == TRACKER_IDLE || autoTracker->state == TRACKER_READY || autoTracker->state == TRACKER_LOST) {
                AutoTracker_StartSelection(autoTracker);
                autoTracker->targetRect.x = mouseVideo.x;
                autoTracker->targetRect.y = mouseVideo.y;
                autoTracker->targetRect.width = 0;
                autoTracker->targetRect.height = 0;
            }
        }
        if (autoTracker->state == TRACKER_SELECTING && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            autoTracker->targetRect.width = mouseVideo.x - autoTracker->targetRect.x;
            autoTracker->targetRect.height = mouseVideo.y - autoTracker->targetRect.y;
        }
        if (autoTracker->state == TRACKER_SELECTING && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            if (autoTracker->targetRect.width < 0) { autoTracker->targetRect.x += autoTracker->targetRect.width; autoTracker->targetRect.width *= -1; }
            if (autoTracker->targetRect.height < 0) { autoTracker->targetRect.y += autoTracker->targetRect.height; autoTracker->targetRect.height *= -1; }

            if (autoTracker->targetRect.width > 5 && autoTracker->targetRect.height > 5) {
                autoTracker->state = TRACKER_INITIALIZING;
                autoTracker->pendingInit = true;
            } else {
                AutoTracker_Stop(autoTracker);
            }
        }
    }
}


void DrawVideoCanvas(struct VideoEngine *v, struct UIState *ui, struct TrackingSystem *ts, struct AutoTracker *autoTracker, Rectangle area) {
    if (!v->isLoaded) {
        const char* msg = L(T_DRAG_VIDEO);
        int textW = MeasureText(msg, 20);
        DrawTextApp(ui, msg, (int)(area.x + area.width/2 - textW/2), (int)(area.y + area.height/2), 20, GRAY);
        return;
    }

    float scale = fminf(area.width / (float)v->width, area.height / (float)v->height);
    float displayW = (float)v->width * scale;
    float displayH = (float)v->height * scale;
    float offsetX = area.x + (area.width - displayW) / 2.0f;
    float offsetY = area.y + (area.height - displayH) / 2.0f;

    Rectangle destRec = { offsetX, offsetY, displayW, displayH };
    Rectangle sourceRec = { 0.0f, 0.0f, (float)v->width, (float)v->height };

    DrawTexturePro(v->texture, sourceRec, destRec, (Vector2){0,0}, 0.0f, WHITE);

    Vector2 mouse = GetMousePosition();
    bool mouseInVideo = CheckCollisionPointRec(mouse, destRec);


    if (autoTracker->state != TRACKER_IDLE) {
        Rectangle r = autoTracker->targetRect;
        Vector2 p1 = VideoToScreen((Vector2){r.x, r.y}, destRec, (float)v->width, (float)v->height);
        Vector2 p2 = VideoToScreen((Vector2){r.x + r.width, r.y + r.height}, destRec, (float)v->width, (float)v->height);
        Rectangle screenR = { p1.x, p1.y, p2.x - p1.x, p2.y - p1.y };

        if (autoTracker->state == TRACKER_SELECTING) {
            DrawRectangleLinesEx(screenR, 2, YELLOW);
        }
        else if (autoTracker->state == TRACKER_INITIALIZING) {
            DrawRectangleLinesEx(screenR, 2, BLUE);
            DrawRectangleRec(screenR, (Color){0, 121, 241, 100});
            const char* msg = L(T_INITIALIZING);
            int tw = MeasureText(msg, 20);
            DrawRectangle(screenR.x + screenR.width/2 - tw/2 - 10, screenR.y - 35, tw + 20, 30, BLACK);
            DrawText(msg, (int)(screenR.x + screenR.width/2 - tw/2), (int)screenR.y - 30, 20, WHITE);
        }
        else if (autoTracker->state == TRACKER_READY) {
            DrawRectangleLinesEx(screenR, 3, GREEN);
            const char* msg = L(T_READY_SPACE);
            int fontSize = 20;
            int tw = MeasureText(msg, fontSize);
            Vector2 textPos = { screenR.x + screenR.width/2 - tw/2, screenR.y - 40 };
            if (screenR.height > 100) textPos.y = screenR.y + screenR.height/2 - 10;
            Rectangle bgRect = { textPos.x - 10, textPos.y - 5, (float)tw + 20, 30.0f };
            DrawRectangleRounded(bgRect, 0.5f, 10, (Color){0,0,0,200});
            DrawText(msg, (int)textPos.x, (int)textPos.y, fontSize, WHITE);
        }
        else if (autoTracker->state == TRACKER_TRACKING) {
            DrawRectangleLinesEx(screenR, 2, GREEN);
            Vector2 centerScreen = VideoToScreen(autoTracker->centerPos, destRec, (float)v->width, (float)v->height);
            DrawCircleV(centerScreen, 3.0f, RED);
            DrawText(L(T_TRACKING_MSG), (int)screenR.x, (int)screenR.y - 20, 10, GREEN);
        }
        else if (autoTracker->state == TRACKER_LOST) {
            DrawRectangleLinesEx(screenR, 2, RED);
            const char* msg = L(T_LOST_RETRY);
            int tw = MeasureText(msg, 20);
            DrawRectangle(screenR.x, screenR.y - 30, screenR.width, 30, (Color){255,0,0,100});
            DrawText(msg, (int)screenR.x + (int)screenR.width/2 - tw/2, (int)screenR.y - 25, 20, WHITE);
        }
    }

    if (ui->showAxes && (ts->calib.hasOriginSet || ts->calib.isSettingOrigin)) {
        Vector2 originVideo = (ts->calib.isSettingOrigin && mouseInVideo) ? ScreenToVideo(mouse, destRec, (float)v->width, (float)v->height) : ts->calib.origin;
        Vector2 originScreen = VideoToScreen(originVideo, destRec, (float)v->width, (float)v->height);
        int dirX = (ts->calib.config == AXIS_X_RIGHT_Y_UP || ts->calib.config == AXIS_X_RIGHT_Y_DOWN) ? 1 : -1;
        int dirY = (ts->calib.config == AXIS_X_RIGHT_Y_DOWN || ts->calib.config == AXIS_X_LEFT_Y_DOWN) ? 1 : -1;
        Color finalColor = ts->calib.isSettingOrigin ? YELLOW : WHITE;
        float axisLen = 120.0f; float tailLen = 20.0f;
        Vector2 startX = { originScreen.x - (tailLen * dirX), originScreen.y };
        Vector2 endX   = { originScreen.x + (axisLen * dirX), originScreen.y };
        DrawOutlinedLine(startX, endX, 2.0f, finalColor);
        DrawModernArrow(endX, (Vector2){(float)dirX, 0}, finalColor);
        DrawTextApp(ui, "x", (int)(endX.x + (dirX>0?15:-25)), (int)endX.y - 7, 14, finalColor);
        Vector2 startY = { originScreen.x, originScreen.y - (tailLen * dirY) };
        Vector2 endY   = { originScreen.x, originScreen.y + (axisLen * dirY) };
        DrawOutlinedLine(startY, endY, 2.0f, finalColor);
        DrawModernArrow(endY, (Vector2){0, (float)dirY}, finalColor);
        DrawTextApp(ui, "y", (int)originScreen.x + 8, (int)(endY.y + (dirY>0?15:-20)), 14, finalColor);
        DrawCircleV(originScreen, 4.0f, BLACK); DrawCircleV(originScreen, 2.5f, finalColor);
    }

    if (ts->calib.scaleStep > 0 || ts->calib.pxPerMeter > 0) {
        Vector2 pA = VideoToScreen(ts->calib.scalePointA, destRec, (float)v->width, (float)v->height);
        Vector2 pB = VideoToScreen(ts->calib.scalePointB, destRec, (float)v->width, (float)v->height);
        Color scaleColor = COLOR_ACCENT;
        DrawOutlinedLine(pA, pB, 2.0f, scaleColor);
        DrawCircleV(pA, 4.0f, scaleColor);
        DrawCircleV(pB, 4.0f, scaleColor);
        DrawTextApp(ui, "A", (int)pA.x - 15, (int)pA.y - 15, 14, scaleColor);
        DrawTextApp(ui, "B", (int)pB.x + 5, (int)pB.y - 15, 14, scaleColor);
        if (ts->calib.realDistance > 0) {
            Vector2 mid = { (pA.x + pB.x)/2, (pA.y + pB.y)/2 };
            const char* distTxt = TextFormat("%.2f m", ts->calib.realDistance);
            DrawRectangle((int)mid.x - 20, (int)mid.y - 10, 40, 20, (Color){0,0,0,150});
            DrawTextApp(ui, distTxt, (int)mid.x - 15, (int)mid.y - 7, 10, WHITE);
        }
    }

    if (ui->showPoints) {
        for (int i = 0; i < ts->count; i++) {
            Vector2 screenPos = VideoToScreen(ts->points[i].pixelPos, destRec, (float)v->width, (float)v->height);
            bool isCurrent = (fabs(ts->points[i].time - v->currentTime) < 0.001);
            if (isCurrent) {
                DrawCircleV(screenPos, 5.0f, RED); DrawCircleLines((int)screenPos.x, (int)screenPos.y, 8.0f, RED);
            } else {
                DrawLine((int)screenPos.x - 4, (int)screenPos.y, (int)screenPos.x + 4, (int)screenPos.y, YELLOW);
                DrawLine((int)screenPos.x, (int)screenPos.y - 4, (int)screenPos.x, (int)screenPos.y + 4, YELLOW);
            }
        }
    }

    if (ts->calib.isSettingOrigin && mouseInVideo) DrawCircleLines((int)mouse.x, (int)mouse.y, 10, YELLOW);
    if (ts->calib.isSettingScale && mouseInVideo) DrawTextApp(ui, ts->calib.scaleStep==0?L(T_POINT_A) : L(T_POINT_B), (int)mouse.x+15, (int)mouse.y, 12, COLOR_ACCENT);
}