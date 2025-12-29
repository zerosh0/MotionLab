
#include "ui_table.h"
#include "ui_core.h"
#include "tracking.h"
#include "video_engine.h"
#include "theme.h"
#include "ui_input.h"
#include "auto_tracker.h"
#include <stdio.h>
#include <math.h>
#include "lang.h"

bool GuiButton(struct UIState *ui, Rectangle bounds, const char* text);

void DrawMeasurementTable(struct UIState *ui, struct TrackingSystem *ts, struct VideoEngine *v, Rectangle bounds) {
    DrawRectangleRec(bounds, (Color){30, 30, 30, 255});
    
    int headerH = 30;
    int rowH = 25;
    int colW = bounds.width / 3;

    DrawRectangle(bounds.x, bounds.y, bounds.width, headerH, (Color){45, 45, 45, 255});
    
    Font font = ui->appFont;
    Color hColor = LIGHTGRAY;
    DrawTextEx(font, "t (s)", (Vector2){bounds.x + 10, bounds.y + 8}, 14, 1, hColor);
    DrawTextEx(font, "x (m)", (Vector2){bounds.x + colW + 10, bounds.y + 8}, 14, 1, hColor);
    DrawTextEx(font, "y (m)", (Vector2){bounds.x + colW*2 + 10, bounds.y + 8}, 14, 1, hColor);

    Rectangle contentRect = { bounds.x, bounds.y + headerH, bounds.width, bounds.height - headerH };
    
    int totalRows = v->frameCount;
    float contentHeight = totalRows * rowH;
    float viewHeight = contentRect.height;
    float maxScroll = contentHeight - viewHeight;
    if (maxScroll < 0) maxScroll = 0;
    
    float scrollBarWidth = 14.0f; 
    float scrollPadding = 4.0f;

    Rectangle scrollHitbox = { 
        bounds.x + bounds.width - scrollBarWidth, 
        contentRect.y, 
        scrollBarWidth, 
        contentRect.height 
    };

    if (CheckCollisionPointRec(GetMousePosition(), contentRect)) {
        ui->tableScrollOffset -= GetMouseWheelMove() * 20.0f;
    }

    Vector2 mouse = GetMousePosition();
    
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, scrollHitbox)) {
        ui->isDraggingTableScroll = true;
    }
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        ui->isDraggingTableScroll = false;
    }

    if (ui->isDraggingTableScroll && maxScroll > 0) {
        float mouseRelY = mouse.y - scrollHitbox.y;
        float ratio = mouseRelY / scrollHitbox.height;
        ui->tableScrollOffset = ratio * maxScroll;
    }

    int currentFrameIdx = (int)(v->currentTime * v->fps + 0.5f);
    if (!ui->isDraggingTableScroll) {
        static int lastFrameIdx = -1;
        if (currentFrameIdx != lastFrameIdx) {
            float rowY = currentFrameIdx * rowH;
            if (rowY < ui->tableScrollOffset || rowY > ui->tableScrollOffset + viewHeight - rowH) {
                ui->tableScrollOffset = rowY - viewHeight / 2;
            }
            lastFrameIdx = currentFrameIdx;
        }
    }

    if (ui->tableScrollOffset < 0) ui->tableScrollOffset = 0;
    if (ui->tableScrollOffset > maxScroll) ui->tableScrollOffset = maxScroll;

    BeginScissorMode((int)contentRect.x, (int)contentRect.y, (int)contentRect.width - (int)scrollBarWidth, (int)contentRect.height);
    
    int startRow = (int)(ui->tableScrollOffset / rowH);
    int endRow = startRow + (int)(viewHeight / rowH) + 2;
    if (endRow > totalRows) endRow = totalRows;

    for (int i = startRow; i < endRow; i++) {
        float y = contentRect.y + (i * rowH) - ui->tableScrollOffset;
        Rectangle rowRect = { bounds.x, y, bounds.width - scrollBarWidth, rowH };

        Color rowColor = (i % 2 == 0) ? (Color){35, 35, 35, 255} : (Color){30, 30, 30, 255};
        bool isDisabled = (i < ts->startFrame);
        
        if (isDisabled) rowColor = (Color){20, 20, 20, 255}; 
        if (i == currentFrameIdx) rowColor = (Color){0, 100, 180, 255}; 
        
        DrawRectangleRec(rowRect, rowColor);

        double t = i * (1.0 / v->fps);
        char tStr[32], xStr[32], yStr[32];
        sprintf(tStr, "%.3f", t);
        sprintf(xStr, "-"); sprintf(yStr, "-");

        int foundPt = -1;
        for(int k=0; k<ts->count; k++) {
            if (fabs(ts->points[k].time - t) < 0.001) { foundPt = k; break; }
        }

        if (foundPt != -1) {
            Vector2 rawPx = ts->points[foundPt].pixelPos;
            if (ts->calib.hasOriginSet && ts->calib.pxPerMeter > 0) {
                float pxPerM = ts->calib.pxPerMeter;
                float dxPx = rawPx.x - ts->calib.origin.x;
                float dyPx = rawPx.y - ts->calib.origin.y;
                int dirX = (ts->calib.config == AXIS_X_RIGHT_Y_UP || ts->calib.config == AXIS_X_RIGHT_Y_DOWN) ? 1 : -1;
                int dirY = (ts->calib.config == AXIS_X_RIGHT_Y_DOWN || ts->calib.config == AXIS_X_LEFT_Y_DOWN) ? 1 : -1;
                
                float valX = (dxPx * dirX) / pxPerM;
                float valY = (dyPx * dirY) / pxPerM;

                sprintf(xStr, "%.3f", valX);
                sprintf(yStr, "%.3f", valY);
            } else {
                sprintf(xStr, "%.0f", rawPx.x);
                sprintf(yStr, "%.0f", rawPx.y);
            }
        }

        Color txtColor = isDisabled ? (Color){100, 100, 100, 255} : WHITE;
        DrawTextEx(font, tStr, (Vector2){bounds.x + 10, y+6}, 14, 1, txtColor);
        DrawTextEx(font, xStr, (Vector2){bounds.x + colW + 10, y+6}, 14, 1, txtColor);
        DrawTextEx(font, yStr, (Vector2){bounds.x + colW*2 + 10, y+6}, 14, 1, txtColor);

        DrawLine(bounds.x + colW, y, bounds.x + colW, y+rowH, (Color){60,60,60,255});
        DrawLine(bounds.x + colW*2, y, bounds.x + colW*2, y+rowH, (Color){60,60,60,255});
        
        if (CheckCollisionPointRec(GetMousePosition(), rowRect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !ui->isDraggingTableScroll) {
            Video_Seek(v, t);
            v->isPlaying = false;
        }
    }
    EndScissorMode();

    if (maxScroll > 0) {
        Rectangle trackVisuel = { 
            scrollHitbox.x + 2, 
            scrollHitbox.y + scrollPadding, 
            scrollHitbox.width - 4, 
            scrollHitbox.height - (scrollPadding * 2)
        };

        DrawRectangleRec(scrollHitbox, (Color){30, 30, 30, 255}); 
        DrawRectangleRounded(trackVisuel, 1.0f, 4, (Color){20, 20, 20, 255});

        float viewRatio = viewHeight / contentHeight;
        float handleHeight = trackVisuel.height * viewRatio;
        if (handleHeight < 30) handleHeight = 30;
        
        float scrollPercent = ui->tableScrollOffset / maxScroll;
        float handleY = trackVisuel.y + (scrollPercent * (trackVisuel.height - handleHeight));
        
        Rectangle handleRect = { 
            trackVisuel.x, 
            handleY, 
            trackVisuel.width, 
            handleHeight
        };

        Color handleColor = ui->isDraggingTableScroll ? (Color){180, 180, 180, 255} : (Color){100, 100, 100, 255};
        DrawRectangleRounded(handleRect, 1.0f, 4, handleColor);
    } else {
        DrawRectangleRec(scrollHitbox, (Color){30, 30, 30, 255});
        DrawLine(scrollHitbox.x + scrollHitbox.width - 1, scrollHitbox.y, 
                 scrollHitbox.x + scrollHitbox.width - 1, scrollHitbox.y + scrollHitbox.height, 
                 (Color){60, 60, 60, 255});
    }
    DrawRectangleLinesEx(bounds, 1, (Color){60, 60, 60, 255});
}
void DrawCheckbox(struct UIState *ui, const char* label, bool *value, int x, int y) {
    Rectangle hitBox = { x, y, 200, 24 };
    if (CheckCollisionPointRec(GetMousePosition(), hitBox) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        *value = !(*value);
    }

    Rectangle destRect = { x, y + 2, 20, 20 };
    Texture2D icon = *value ? ui->iconCheckOn : ui->iconCheckOff;
    
    if (icon.id > 0) {
        Rectangle srcRect = { 0, 0, (float)icon.width, (float)icon.height };
        DrawTexturePro(icon, srcRect, destRect, (Vector2){0,0}, 0.0f, WHITE);
    } else {
        DrawRectangleLines(x, y + 4, 16, 16, GRAY);
        if (*value) DrawRectangle(x + 4, y + 8, 8, 8, COLOR_ACCENT);
    }
    
    DrawTextEx(ui->appFont, label, (Vector2){x + 28, y + 4}, 14, 1.0f, WHITE);
}



void DrawCommonSettingsWithTS(struct UIState *ui, struct TrackingSystem *ts, struct AutoTracker *tracker, Rectangle bounds) {
    int x = (int)bounds.x + 10;
    int y = (int)bounds.y + 10;
    int spacing = 30;

    DrawCheckbox(ui, L(T_AUTO_ADVANCE), &ui->autoAdvance, x, y);
    y += spacing;
    DrawCheckbox(ui, L(T_SHOW_POINTS_TOGGLE), &ui->showPoints, x, y);
    y += spacing;
    DrawCheckbox(ui, L(T_SHOW_AXES_TOGGLE), &ui->showAxes, x, y);
    y += spacing;
    y += 10;

    DrawTextEx(ui->appFont, L(T_FIRST_FRAME), (Vector2){(float)x, (float)y + 6}, 14, 1.0f, LIGHTGRAY);
    
    int textWidth = MeasureText(L(T_FIRST_FRAME), 14);
    int startX = x + textWidth + 15; 

    if (GuiButton(ui, (Rectangle){(float)startX, (float)y, 25, 25}, "<")) {
        if (ts->startFrame > 0) ts->startFrame--;
    }
    
    const char* valStr = TextFormat("%02d", ts->startFrame);
    DrawTextEx(ui->appFont, valStr, (Vector2){(float)startX + 32, (float)y + 4}, 14, 1.0f, WHITE);
    
    if (GuiButton(ui, (Rectangle){(float)startX + 60, (float)y, 25, 25}, ">")) {
        ts->startFrame++;
    }
}