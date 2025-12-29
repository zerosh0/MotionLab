#include "raylib.h"
#include "ui_panels.h"
#include "theme.h"
#include "ui_core.h"
#include "tracking.h"
#include "video_engine.h"
#include <stdio.h>
#include <math.h>
#include "ui_input.h"
#include "ui_table.h"
#include "auto_tracker.h"
#include "lang.h"

bool DrawTabButton(const char* label, bool active, Rectangle bounds, struct UIState *ui) {
    if (active) DrawRectangleRoundedCustom(bounds,0.3f,10, COLOR_ACCENT,true,true,false,false);
    else if (CheckCollisionPointRec(GetMousePosition(), bounds)) DrawRectangleRoundedCustom(bounds,0.3f,10, (Color){45, 45, 45, 255},true,true,false,false);
    DrawTextApp(ui,label, (int)bounds.x + 10, (int)bounds.y + 10, 16, active ? WHITE : LIGHTGRAY);
    return (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), bounds));
}

bool GuiButton(struct UIState *ui, Rectangle bounds, const char* text) {
    Vector2 mousePoint = GetMousePosition();
    bool isHover = CheckCollisionPointRec(mousePoint, bounds);
    bool isPressed = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    bool clicked = isHover && isPressed;
    Color colorBg = (Color){60, 60, 60, 255}; 
    if (isHover) colorBg = (Color){80, 80, 80, 255};
    if (clicked) colorBg = COLOR_ACCENT;
    DrawRectangleRounded(bounds, 0.4f, 4, colorBg);
    float fontSize = 14;
    Vector2 textSize = MeasureTextEx(ui->appFont, text, fontSize, 1.0f);
    Vector2 textPos = {
        bounds.x + (bounds.width - textSize.x) / 2.0f,
        bounds.y + (bounds.height - textSize.y) / 2.0f
    };
    
    DrawTextEx(ui->appFont, text, textPos, fontSize, 1.0f, WHITE);

    return clicked;
}


void DrawInfoRow(struct UIState *ui, const char* label, const char* value, int x, int y, int width) {
    if (label) DrawTextApp(ui, label, x, y, 14, GRAY);
    const char* safeValue = (value && value[0] != '\0') ? value : "-";
    Vector2 size = MeasureTextEx(ui->appFont, safeValue, 14, 1.0f);
    DrawTextApp(ui, safeValue, x + width - (int)size.x, y, 14, WHITE);
    DrawLine(x, y + 20, x + width, y + 20, (Color){60, 60, 60, 255});
}

void DrawRightPanel(struct UIState *ui, struct TrackingSystem *ts, struct VideoEngine *v, struct AutoTracker *tracker, const char* filename) {
    Rectangle panelBounds = { (float)GetScreenWidth() - PANEL_WIDTH, TITLEBAR_HEIGHT, PANEL_WIDTH, (float)GetScreenHeight() - TITLEBAR_HEIGHT };
    if (ui->isMaximized) {
        DrawRectangleRec(panelBounds, COLOR_PANEL);
    } else {
        DrawRectangleRoundedCustom(panelBounds, 0.07f, 10, COLOR_PANEL, true, false, false, true);
    }

    float tabWidth = PANEL_WIDTH / 3.0f;
    if (DrawTabButton(L(T_TAB_MEASURES), ui->activeTab == TAB_MEASURES, (Rectangle){ panelBounds.x, panelBounds.y, tabWidth, 40 }, ui)) ui->activeTab = TAB_MEASURES;
    if (DrawTabButton(L(T_TAB_CALIB), ui->activeTab == TAB_CALIB, (Rectangle){ panelBounds.x + tabWidth, panelBounds.y, tabWidth, 40 }, ui)) ui->activeTab = TAB_CALIB;
    if (DrawTabButton(L(T_TAB_INFO), ui->activeTab == TAB_INFO, (Rectangle){ panelBounds.x + tabWidth * 2, panelBounds.y, tabWidth, 40 }, ui)) ui->activeTab = TAB_INFO;

    float commonHeight = 160.0f;
    Rectangle contentArea = { 
        panelBounds.x + 10, 
        panelBounds.y + 60, 
        PANEL_WIDTH - 20, 
        panelBounds.height - 70 - commonHeight
    };

    Rectangle commonArea = {
        panelBounds.x,
        panelBounds.y + panelBounds.height - commonHeight,
        PANEL_WIDTH,
        commonHeight
    };
    if (ui->activeTab == TAB_MEASURES) {
        DrawMeasurementTable(ui, ts, v, contentArea);
    } 
    else if (ui->activeTab == TAB_CALIB) {
        int y = (int)contentArea.y;
        int w = (int)contentArea.width;

        DrawTextApp(ui, L(T_AXIS_ORIENTATION), (int)contentArea.x, y, 12, COLOR_ACCENT);
        y += 25;

        int btnSize = 50;
        int gap = 15;
        int startX = (int)contentArea.x + (w - (btnSize*2 + gap)) / 2;

        for (int i = 0; i < 4; i++) {
            int row = i / 2;
            int col = i % 2;
            int bx = startX + col * (btnSize + gap);
            int by = y + row * (btnSize + gap);
            
            Rectangle btnRect = { (float)bx, (float)by, (float)btnSize, (float)btnSize };
            bool isSelected = (ts->calib.config == (AxisConfiguration)i);
            bool hover = CheckCollisionPointRec(GetMousePosition(), btnRect);
            
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hover) {
                ts->calib.config = (AxisConfiguration)i;
            }

            Color bg = isSelected ? (Color){60, 60, 60, 255} : (Color){40, 40, 40, 255};
            if(hover && !isSelected) bg = (Color){50, 50, 50, 255};
            
            DrawRectangleRounded(btnRect, 0.2f, 4, bg);
            if(isSelected) DrawRectangleRoundedLines(btnRect, 0.2f, 4, COLOR_ACCENT);
            if (ui->axisIcons[i].id > 0) {
                float targetSize = btnSize * 0.6f; 
                float maxDim = (float)fmax(ui->axisIcons[i].width, ui->axisIcons[i].height);
                float iconScale = targetSize / maxDim;
                float iw = ui->axisIcons[i].width * iconScale;
                float ih = ui->axisIcons[i].height * iconScale;
                Vector2 pos = { 
                    bx + (btnSize - iw) / 2.0f, 
                    by + (btnSize - ih) / 2.0f 
                };
                
                Color tint = isSelected ? COLOR_ACCENT : (hover ? WHITE : GRAY);
                DrawTextureEx(ui->axisIcons[i], pos, 0.0f, iconScale, tint);
            }
        }
        y += (btnSize * 2 + gap + 25);

        DrawTextApp(ui, L(T_ORIGIN_POS), (int)contentArea.x, y, 12, COLOR_ACCENT);
        y += 25;
        const char* btnLabel = ts->calib.isSettingOrigin ? L(T_CLICK_ON_VIDEO) : L(T_PLACE_ORIGIN);
        if (GuiButton(ui, (Rectangle){ (float)contentArea.x, (float)y, (float)w, 35 }, btnLabel)) {
             ts->calib.isSettingOrigin = !ts->calib.isSettingOrigin;
             ts->calib.isSettingScale = false;
        }
        y += 45;
        DrawTextApp(ui, TextFormat("X: %.0f px   Y: %.0f px", ts->calib.origin.x, ts->calib.origin.y), (int)contentArea.x, y, 14, GRAY);
        y += 30;

        DrawLine((int)contentArea.x, y, (int)contentArea.x + w, y, (Color){60, 60, 60, 255});
        y += 20;
        DrawTextApp(ui, L(T_SCALE), (int)contentArea.x, y, 12, COLOR_ACCENT);
        y += 25;
        const char* scaleLabel = ts->calib.isSettingScale ? L(T_DEFINE_SCALE_AB) : L(T_DEFINE_SCALE);
        if (GuiButton(ui, (Rectangle){ (float)contentArea.x, (float)y, (float)w, 35 }, scaleLabel)) {
            ts->calib.isSettingScale = !ts->calib.isSettingScale;
            ts->calib.isSettingOrigin = false;
            if (ts->calib.isSettingScale) ts->calib.scaleStep = 0;
        }
        y += 50;
        DrawTextApp(ui, L(T_REAL_DIST), (int)contentArea.x, y + 8, 14, GRAY);
        float inputW = 100;
        GuiFloatInput(ui, (Rectangle){ (float)(contentArea.x + w - inputW), (float)y, inputW, 30 }, &ts->calib.realDistance, "m");
        y += 45;
        if (ts->calib.scaleStep > 0 || ts->calib.pxPerMeter > 0) {
            float dx = ts->calib.scalePointB.x - ts->calib.scalePointA.x;
            float dy = ts->calib.scalePointB.y - ts->calib.scalePointA.y;
            float pxDist = sqrtf(dx*dx + dy*dy);
            
            if (pxDist > 0 && ts->calib.realDistance > 0) {
                ts->calib.pxPerMeter = pxDist / ts->calib.realDistance;
            }
        }
        DrawRectangleRounded((Rectangle){(float)contentArea.x, (float)y, (float)w, 40}, 0.2f, 4, (Color){35,35,35,255});
        const char* resText = TextFormat(L(T_PX_PER_METER), ts->calib.pxPerMeter);
        Vector2 resSz = MeasureTextEx(ui->appFont, resText, 14, 1.0f);
        DrawTextApp(ui, resText, (int)(contentArea.x + (w-resSz.x)/2), y + 13, 14, COLOR_ACCENT);
    }
    else if (ui->activeTab == TAB_INFO) {
        if (!v->isLoaded) {
            DrawTextApp(ui, L(T_NO_FILE), (int)contentArea.x, (int)contentArea.y + 20, 16, GRAY);
            return;
        }

        int y = (int)contentArea.y;
        int spacing = 35;
        int w = (int)contentArea.width;

        DrawTextApp(ui, L(T_FILE_SECTION), (int)contentArea.x, y, 12, COLOR_ACCENT);
        y += 25;

        const char* rawName = GetFileNameFromPath(filename);
        static char displayName[44];
        if (rawName == NULL) rawName = L(T_NO_FILE);
        if (strlen(rawName) > 30) {
            const char* sub = TextSubtext(rawName, 0, 27);
            TextCopy(displayName, TextFormat("%s...", sub));
        } else {
            TextCopy(displayName, rawName);
        }

        DrawInfoRow(ui, (currentLang == LANG_FR ? "Nom" : "Name"), displayName, (int)contentArea.x, y, w);
        y += spacing;
        y += UI_PADDING_SM;
        DrawTextApp(ui, L(T_VIDEO_SECTION), (int)contentArea.x, y, 12, COLOR_ACCENT);
        y += UI_PADDING_MD;
        DrawInfoRow(ui, L(T_RES), TextFormat("%d x %d", v->width, v->height), (int)contentArea.x, y, w);
        y += spacing;
        DrawInfoRow(ui, L(T_FPS), TextFormat("%.2f fps", v->fps), (int)contentArea.x, y, w);
        y += spacing;
        DrawInfoRow(ui, L(T_DURATION), TextFormat("%.2f s", v->durationSec), (int)contentArea.x, y, w);
        y += spacing;
        DrawInfoRow(ui, L(T_TOTAL_FRAMES), TextFormat("%lld imgs", v->frameCount), (int)contentArea.x, y, w);
        y += spacing;
        y += UI_PADDING_SM;
        DrawTextApp(ui, L(T_ENCODING_SECTION), (int)contentArea.x, y, 12, COLOR_ACCENT);
        y += UI_PADDING_MD;
        DrawInfoRow(ui,L(T_CODEC), v->codecName, (int)contentArea.x, y, w);
        y += spacing;
        DrawInfoRow(ui,L(T_PIXELS), v->pixelFormat, (int)contentArea.x, y, w);
    }
    DrawLine(commonArea.x, commonArea.y, commonArea.x + commonArea.width, commonArea.y, (Color){60,60,60,255});
    DrawCommonSettingsWithTS(ui, ts, tracker, commonArea);
}


void DrawSidebar(struct UIState *ui) {
    Rectangle barBounds = { 0, TITLEBAR_HEIGHT + 20, SIDEBAR_WIDTH, (float)GetScreenHeight() - TITLEBAR_HEIGHT - 20 };
    if (ui->isMaximized) {
        DrawRectangleRec(barBounds, (Color){25, 25, 25, 255});
    } else {
        DrawRectangleRoundedCustom(barBounds, 0.3f, 10, (Color){25, 25, 25, 255}, false, true, true, false);
    }

    float startY = TITLEBAR_HEIGHT + 40;
    float btnHeight = 70;
    float btnWidth = SIDEBAR_WIDTH;
    
    const char* toolLabels[] = { "Select", "Point", "Loop", "Track" };

    for (int i = 0; i < 4; i++) {
        Rectangle btnBounds = { 0, startY + (i * btnHeight), btnWidth, btnHeight };
        bool hover = CheckCollisionPointRec(GetMousePosition(), btnBounds);
        bool active = false;

        if (i == TOOL_LOOP) {
            active = ui->magnifier.isVisible;
        } 
        else {
            active = (ui->currentTool == i);
        }

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hover) {
            if (i == TOOL_LOOP) {
                ui->magnifier.isVisible = !ui->magnifier.isVisible;
            }
            else {
                ui->currentTool = (ToolMode)i;
            }
        }


        Color tint = active ? COLOR_ACCENT : (hover ? WHITE : GRAY);

        if (active) {
            DrawRectangleRec(btnBounds, BLACK);
            DrawRectangle(0, (int)btnBounds.y + 10, 2, (int)btnBounds.height - 20, COLOR_ACCENT);
        }

        Texture2D icon = ui->toolIcons[i];
        float iconSize = 28.0f;
        
        float iconX = btnBounds.x + (btnBounds.width - iconSize) / 2.0f;
        float iconY = btnBounds.y + 10;

        Rectangle src = { 0, 0, (float)icon.width, (float)icon.height };
        Rectangle dest = { iconX, iconY, iconSize, iconSize };
        
        DrawTexturePro(icon, src, dest, (Vector2){0,0}, 0.0f, tint);

        const char* label = toolLabels[i];
        int fontSize = 14;
        int textWidth = MeasureText(label, fontSize);
        
        int textX = (int)(btnBounds.x + (btnBounds.width - textWidth) / 2.0f);
        int textY = (int)(iconY + iconSize + 5);

        DrawTextApp(ui, label, textX, textY, fontSize, tint);
    }
}

void DrawBottomBar(struct UIState *ui, struct VideoEngine *v, Rectangle videoArea) {
    if (!v->isLoaded) return;

    float barHeight = 80.0f;
    Rectangle barRect = { 
        videoArea.x, 
        videoArea.y + videoArea.height + 10, 
        videoArea.width, 
        barHeight 
    };

    DrawRectangleRounded(barRect, 0.1f, 10, COLOR_PANEL);

    float iconSize = 24.0f;
    float spacing = 20.0f;
    float totalControlsWidth = (iconSize * 3) + (spacing * 2);
    
    float startX = barRect.x + (barRect.width - totalControlsWidth) / 2.0f;
    float centerY = barRect.y + 25.0f;

    Rectangle prevRect = { startX, centerY - iconSize/2, iconSize, iconSize };
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), prevRect)) {
        Video_PrevFrame(v);
        v->isPlaying = false;
    }
    DrawTexturePro(ui->iconPrev, (Rectangle){0,0,ui->iconPrev.width, ui->iconPrev.height}, prevRect, (Vector2){0,0}, 0.0f, WHITE);

    Rectangle playRect = { startX + iconSize + spacing, centerY - iconSize/2, iconSize, iconSize };
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), playRect)) {
        Video_TogglePlay(v);
    }
    Texture2D currentIcon = v->isPlaying ? ui->iconPause : ui->iconPlay;
    DrawTexturePro(currentIcon, (Rectangle){0,0,currentIcon.width, currentIcon.height}, playRect, (Vector2){0,0}, 0.0f, WHITE);

    Rectangle nextRect = { startX + (iconSize + spacing) * 2, centerY - iconSize/2, iconSize, iconSize };
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), nextRect)) {
        Video_NextFrame(v);
        v->isPlaying = false;
    }
    DrawTexturePro(ui->iconNext, (Rectangle){0,0,ui->iconNext.width, ui->iconNext.height}, nextRect, (Vector2){0,0}, 0.0f, WHITE);


    float sliderMargin = 10.0f;
    float sliderY = barRect.y + 55.0f;
    float sliderW = barRect.width - (sliderMargin * 2);
    float sliderH = 6.0f;

    DrawRectangleRounded((Rectangle){barRect.x + sliderMargin, sliderY, sliderW, sliderH}, 1.0f, 4, (Color){60,60,60,255});

    static bool isDraggingTimeline = false;
    Rectangle hitBoxSlider = { barRect.x + sliderMargin, sliderY - 15, sliderW, 35 };

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), hitBoxSlider)) {
        isDraggingTimeline = true;
        v->isPlaying = false;
    }
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        isDraggingTimeline = false;
    }

    if (isDraggingTimeline) {
        float mouseX = GetMousePosition().x;
        float ratio = 0.0f;
        if(sliderW > 0) ratio = (mouseX - (barRect.x + sliderMargin)) / sliderW;
        
        if (ratio < 0.0f) ratio = 0.0f;
        if (ratio > 1.0f) ratio = 1.0f;
        
        Video_Seek(v, ratio * v->durationSec);
    }

    double frameDuration = (v->fps > 0) ? (1.0 / v->fps) : 0.0;
    double maxSeekableTime = v->durationSec - frameDuration;
    if (maxSeekableTime <= 0) maxSeekableTime = v->durationSec;

    float progress = 0.0f;
    if (maxSeekableTime > 0) {
        progress = (float)(v->currentTime / maxSeekableTime);
    }
    if (progress > 1.0f) progress = 1.0f;
    if (progress < 0.0f) progress = 0.0f;

    Rectangle progRect = { barRect.x + sliderMargin, sliderY, sliderW * progress, sliderH };
    DrawRectangleRounded(progRect, 1.0f, 4, COLOR_ACCENT);
    DrawCircle((int)(progRect.x + progRect.width), (int)(sliderY + sliderH/2), 7, WHITE);

    double displayTime = v->currentTime;
    
    if (v->currentTime >= maxSeekableTime - 0.01) {
        displayTime = v->durationSec;
    }

    const char* timeText;
    if (v->durationSec < 60.0) {
        timeText = TextFormat("%.3f / %.3f s", displayTime, v->durationSec);
    } else {
        timeText = TextFormat("%02d:%02d / %02d:%02d", 
            (int)displayTime / 60, (int)displayTime % 60,
            (int)v->durationSec / 60, (int)v->durationSec % 60);
    }
    
    int textW = MeasureText(timeText, 12);
    DrawTextApp(ui, timeText, (int)(barRect.x + barRect.width - textW - 15), (int)(centerY - 6), 12, LIGHTGRAY);
}



void DrawHelpWindow(struct UIState *ui) {
    if (!ui->showHelp) return;

    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    float w = 850;
    float h = 650;
    Rectangle bounds = { (sw - w)/2, (sh - h)/2, w, h };

    DrawRectangleRec(bounds, (Color){35, 35, 35, 255}); 
    DrawRectangleLinesEx(bounds, 2, (Color){80, 80, 80, 255});

    Rectangle header = { bounds.x, bounds.y, w, 50 };
    DrawRectangleRec(header, (Color){50, 50, 50, 255});
    DrawTextEx(ui->appFont, L(T_HELP_TITLE), (Vector2){bounds.x + 25, bounds.y + 12}, 20, 1.0f, WHITE);

    Rectangle closeBtn = { bounds.x + w - 50, bounds.y, 50, 50 };
    if (GuiButton(ui, closeBtn, "X")) ui->showHelp = false;

    float textX = bounds.x + 40;
    float textY = bounds.y + 80;
    int spacing = 32;
    int sectionGap = 45;
    Color colTitle = COLOR_ACCENT;
    Color colText = LIGHTGRAY;

    DrawTextEx(ui->appFont, L(T_HELP_S1_TITLE), (Vector2){textX, textY}, 20, 1.0f, colTitle); 
    textY += UI_ROW_HEIGHT;
    DrawTextEx(ui->appFont, L(T_HELP_S1_B1), (Vector2){textX, textY}, 18, 1.0f, colText); 
    textY += spacing;
    DrawTextEx(ui->appFont, L(T_HELP_S1_B2), (Vector2){textX, textY}, 18, 1.0f, colText); 
    textY += sectionGap;

    DrawTextEx(ui->appFont, L(T_HELP_S2_TITLE), (Vector2){textX, textY}, 20, 1.0f, colTitle); 
    textY += UI_ROW_HEIGHT;
    DrawTextEx(ui->appFont, L(T_HELP_S2_B1), (Vector2){textX, textY}, 18, 1.0f, colText); 
    textY += spacing;
    DrawTextEx(ui->appFont, L(T_HELP_S2_B2), (Vector2){textX, textY}, 18, 1.0f, colText); 
    textY += spacing;
    DrawTextEx(ui->appFont, L(T_HELP_S2_B3), (Vector2){textX, textY}, 18, 1.0f, colText); 
    textY += sectionGap;

    DrawTextEx(ui->appFont, L(T_HELP_S3_TITLE), (Vector2){textX, textY}, 20, 1.0f, colTitle); 
    textY += UI_ROW_HEIGHT;
    DrawTextEx(ui->appFont, L(T_HELP_S3_B1), (Vector2){textX, textY}, 18, 1.0f, colText); 
    textY += spacing;
    DrawTextEx(ui->appFont, L(T_HELP_S3_B2), (Vector2){textX, textY}, 18, 1.0f, colText); 
    textY += spacing;
    DrawTextEx(ui->appFont, L(T_HELP_S3_B3), (Vector2){textX, textY}, 18, 1.0f, colText); 
    textY += sectionGap;

    DrawTextEx(ui->appFont, L(T_HELP_S4_TITLE), (Vector2){textX, textY}, 20, 1.0f, colTitle); 
    textY += UI_ROW_HEIGHT;
    DrawTextEx(ui->appFont, L(T_HELP_S4_B1), (Vector2){textX, textY}, 18, 1.0f, colText); 
    textY += spacing;
    DrawTextEx(ui->appFont, L(T_HELP_S4_B2), (Vector2){textX, textY}, 18, 1.0f, colText); 
}