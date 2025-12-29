#include "raylib.h"
#include "ui_core.h"
#include "ui_panels.h"
#include "theme.h"
#include "video_engine.h"
#include <string.h>
#include <math.h>
#include "auto_tracker.h"
#include "resources.h"
#include "ui_menu.h"
#include "lang.h"

#ifdef _WIN32
    typedef struct {
        long x;
        long y;
    } WIN_POINT;
    
    __declspec(dllimport) int __stdcall GetCursorPos(WIN_POINT* lpPoint);
#endif

Vector2 GetMousePositionGlobal(void) {
    #ifdef _WIN32
        WIN_POINT p;
        GetCursorPos(&p);
        return (Vector2){ (float)p.x, (float)p.y };
    #else
        Vector2 win = GetWindowPosition();
        Vector2 mouse = GetMousePosition();
        return (Vector2){ win.x + mouse.x, win.y + mouse.y };
    #endif
}

const char* GetFileNameFromPath(const char* path) {
    if (path == NULL || strlen(path) == 0) return L(T_NO_FILE);
    const char* filename = strrchr(path, '\\');
    if (!filename) filename = strrchr(path, '/');
    return filename ? filename + 1 : path;
}

static Texture2D LoadTextureFromRes(const unsigned char *data, unsigned int size) {
    Image img = LoadImageFromMemory(".png", data, size);
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
    return tex;
}

void InitUIState(UIState *state){
    state->currentTool = TOOL_SELECT;
    state->activeTab = TAB_MEASURES;
    state->autoAdvance = true;
    state->showPoints = true;
    state->showAxes = true;
    state->magnifier.isVisible = false;
    state->magnifier.zoomLevel = 2.0f;
    state->magnifier.bounds = (Rectangle){ 100, 100, 200, 200 };
    state->magnifier.isDragging = false;
    state->magnifier.dragOffset = (Vector2){ 0, 0 };
    state->showGraphWindow = false;
    state->showHelp = false;
    state->isWindowDragging = false;
    state->isResizing = false;  
    state->resizeEdge = 0;
    state->isMaximized = false;
    state->isMenuOpen = false;
    state->isEditingDist = false;
    memset(state->distInputBuf, 0, 64);
    state->distCursorIndex = 0;
    state->distSelectStart = -1;
    state->distScrollOffset = 0.0f;
    state->tableScrollOffset = 0.0f;
    state->isDraggingTableScroll = false;
}

void LoadUIResources(UIState *state){
    state->appFont = LoadFontFromMemory(".ttf", fonts_Inter_24pt_Medium_ttf_data, fonts_Inter_24pt_Medium_ttf_size, 32, NULL, 250);
    SetTextureFilter(state->appFont.texture, TEXTURE_FILTER_BILINEAR);
    state->iconClose = LoadTextureFromRes(icons_close_png_data, icons_close_png_size);
    state->iconMax   = LoadTextureFromRes(icons_maximize_png_data, icons_maximize_png_size);
    state->iconMin   = LoadTextureFromRes(icons_minimize_png_data, icons_minimize_png_size);
    state->toolIcons[0] = LoadTextureFromRes(icons_cursor_png_data, icons_cursor_png_size);
    state->toolIcons[1] = LoadTextureFromRes(icons_target_png_data, icons_target_png_size);
    state->toolIcons[2] = LoadTextureFromRes(icons_loop_png_data, icons_loop_png_size);
    state->toolIcons[3] = LoadTextureFromRes(icons_glasses_png_data, icons_glasses_png_size);
    state->iconPlay  = LoadTextureFromRes(icons_play_png_data, icons_play_png_size);
    state->iconPause = LoadTextureFromRes(icons_pause_png_data, icons_pause_png_size);
    state->iconNext  = LoadTextureFromRes(icons_next_frame_png_data, icons_next_frame_png_size);
    state->iconPrev  = LoadTextureFromRes(icons_prev_frame_png_data, icons_prev_frame_png_size);
    state->axisIcons[0] = LoadTextureFromRes(axes_axis0_png_data, axes_axis0_png_size);
    state->axisIcons[1] = LoadTextureFromRes(axes_axis1_png_data, axes_axis1_png_size);
    state->axisIcons[2] = LoadTextureFromRes(axes_axis2_png_data, axes_axis2_png_size);
    state->axisIcons[3] = LoadTextureFromRes(axes_axis3_png_data, axes_axis3_png_size);
    state->iconCheckOn  = LoadTextureFromRes(icons_check_on_png_data, icons_check_on_png_size);
    state->iconCheckOff = LoadTextureFromRes(icons_check_off_png_data, icons_check_off_png_size);

}

void InitUI(UIState *state) {
    InitUIState(state);
    LoadUIResources(state);
}


void UnloadUI(UIState *state) {
    UnloadFont(state->appFont);
    UnloadTexture(state->iconClose);
    UnloadTexture(state->iconMax);
    UnloadTexture(state->iconMin);
    UnloadTexture(state->iconPlay);
    UnloadTexture(state->iconPause);
    UnloadTexture(state->iconNext);
    UnloadTexture(state->iconPrev);
    UnloadTexture(state->iconCheckOff);
    UnloadTexture(state->iconCheckOn);
    for(int i=0; i<4; i++) UnloadTexture(state->toolIcons[i]);
    for(int i=0; i<4; i++) UnloadTexture(state->axisIcons[i]);
}

void DrawTextApp(UIState *ui, const char *text, int x, int y, int fontSize, Color color) {
    DrawTextEx(ui->appFont, text, (Vector2){(float)x, (float)y}, (float)fontSize, 1.0f, color);
}

void DrawRectangleRoundedCustom(Rectangle rec, float roundness, int segments, Color color,
                                bool topLeft, bool topRight, bool bottomLeft, bool bottomRight) {
    
    DrawRectangleRounded(rec, roundness, segments, color);
    float shortSide = (rec.width < rec.height) ? rec.width : rec.height;
    float radius = shortSide * roundness;
    if (!topLeft) {
        DrawRectangleRec((Rectangle){ rec.x, rec.y, radius, radius }, color);
    }
    if (!topRight) {
        DrawRectangleRec((Rectangle){ rec.x + rec.width - radius, rec.y, radius, radius }, color);
    }
    if (!bottomLeft) {
        DrawRectangleRec((Rectangle){ rec.x, rec.y + rec.height - radius, radius, radius }, color);
    }

    if (!bottomRight) {
        DrawRectangleRec((Rectangle){ rec.x + rec.width - radius, rec.y + rec.height - radius, radius, radius }, color);
    }
}


void HandleWindowResize(UIState *state) {
    if (IsWindowMaximized()) return;

    Vector2 mouseLocal = GetMousePosition();
    int w = GetScreenWidth();
    int h = GetScreenHeight();
    int margin = 8; 

    if (!state->isResizing) {
        bool right  = (mouseLocal.x > w - margin);
        bool bottom = (mouseLocal.y > h - margin);
        
        // --- PORTABILITÉ WAYLAND ---
        bool left = false;
        bool top  = false;
#ifdef _WIN32
        left = (mouseLocal.x < margin);
        top  = (mouseLocal.y < margin);
#endif

        if ((left && top) || (right && bottom)) SetMouseCursor(MOUSE_CURSOR_RESIZE_NWSE);
        else if ((left && bottom) || (right && top)) SetMouseCursor(MOUSE_CURSOR_RESIZE_NESW);
        else if (left || right) SetMouseCursor(MOUSE_CURSOR_RESIZE_EW);
        else if (top || bottom) SetMouseCursor(MOUSE_CURSOR_RESIZE_NS);
        else SetMouseCursor(MOUSE_CURSOR_DEFAULT);

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            state->resizeEdge = 0;
            if (left)   state->resizeEdge |= 1; 
            if (right)  state->resizeEdge |= 2; 
            if (top)    state->resizeEdge |= 4; 
            if (bottom) state->resizeEdge |= 8; 

            if (state->resizeEdge != 0) {
                state->isResizing = true;
                state->mouseStartGlobal = GetMousePositionGlobal();
                Vector2 winPos = GetWindowPosition();
                state->windowBoundsStart = (Rectangle){ winPos.x, winPos.y, (float)w, (float)h };
            }
        }
    }

    if (state->isResizing) {
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            state->isResizing = false;
            state->resizeEdge = 0;
            SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        } else {
            Vector2 currentGlobal = GetMousePositionGlobal();
            float dx = currentGlobal.x - state->mouseStartGlobal.x;
            float dy = currentGlobal.y - state->mouseStartGlobal.y;

            Rectangle newRec = state->windowBoundsStart;

            if (state->resizeEdge & 2) newRec.width += dx;
            if (state->resizeEdge & 8) newRec.height += dy;

#ifdef _WIN32
            if (state->resizeEdge & 1) {
                newRec.x += dx;
                newRec.width -= dx;
            }
            if (state->resizeEdge & 4) {
                newRec.y += dy;
                newRec.height -= dy;
            }
#endif

            if (newRec.width < 800) {
#ifdef _WIN32
                if (state->resizeEdge & 1) newRec.x = state->windowBoundsStart.x + state->windowBoundsStart.width - 800;
#endif
                newRec.width = 800;
            }
            if (newRec.height < 600) {
#ifdef _WIN32
                if (state->resizeEdge & 4) newRec.y = state->windowBoundsStart.y + state->windowBoundsStart.height - 600;
#endif
                newRec.height = 600;
            }

            SetWindowSize((int)newRec.width, (int)newRec.height);
            
#ifdef _WIN32
            SetWindowPosition((int)newRec.x, (int)newRec.y);
#endif
        }
    }
}


bool DrawTitleBar(UIState *state, const char* fullPath, struct VideoEngine *video, struct TrackingSystem *ts, struct AutoTracker *tracker) {
    int screenW = GetScreenWidth();
    Rectangle titleBarRec = { 0, 0, (float)screenW, TITLEBAR_HEIGHT };
    
    if (state->isMaximized) {
        DrawRectangleRec(titleBarRec, COLOR_PANEL);
    } else {
        Rectangle roundedHeader = titleBarRec;
        DrawRectangleRoundedCustom(roundedHeader, 0.2f, 10, COLOR_PANEL, true, true, false, false);
    }


    int logoY = (int)(UI_TITLEBAR_HEIGHT - 20) / 2; 
    DrawTextApp(state, "Motion", 15, logoY, 20, COLOR_TEXT);
    DrawTextApp(state, "Lab", 75, logoY, 20, COLOR_CYAN);

    const char* rawName = GetFileNameFromPath(fullPath);
    static char displayName[44];
    if (rawName == NULL) rawName = L(T_NO_FILE);
    if (strlen(rawName) > 38) {
        const char* sub = TextSubtext(rawName, 0, 35);
        TextCopy(displayName, TextFormat("%s...", sub));
    } else {
        TextCopy(displayName, rawName);
    }

    Vector2 titleSize = MeasureTextEx(state->appFont, displayName, 16, 1.0f);
    DrawTextApp(state, displayName, screenW/2 - (int)titleSize.x/2, (int)(UI_TITLEBAR_HEIGHT - 16)/2, 16, LIGHTGRAY);
    const char* statusText = NULL; 
    Color statusColor = GRAY;
    
    if (tracker != NULL) {
    if (state->currentTool == TOOL_TRACK && tracker->state == TRACKER_IDLE) {
            statusText = L(T_STATUS_IDLE_TRACK);
            statusColor = COLOR_CYAN; 
        }
        else if (tracker->state == TRACKER_SELECTING) {
            statusText = L(T_STATUS_SELECTING);
            statusColor = (Color){ 255, 238, 88, 255 }; 
        }
        else if (tracker->state == TRACKER_INITIALIZING) {
            statusText = L(T_STATUS_INITIALIZING);
            statusColor = (Color){ 41, 182, 246, 255 }; 
        }
        else if (tracker->state == TRACKER_READY) {
            statusText = L(T_STATUS_READY);
            statusColor = (Color){ 102, 187, 106, 255 }; 
        }
        else if (tracker->state == TRACKER_TRACKING) {
            statusText = L(T_STATUS_TRACKING);
            statusColor = (Color){ 102, 187, 106, 255 }; 
        }
        else if (tracker->state == TRACKER_LOST) {
            statusText = L(T_STATUS_LOST);
            statusColor = (Color){ 239, 83, 80, 255 }; 
        }
    }
    

    if (statusText != NULL) {
        Vector2 statusSize = MeasureTextEx(state->appFont, statusText, UI_STATUS_FONT_SIZE, 1.0f);
        int textY = (int)(UI_TITLEBAR_HEIGHT + ((TITLEBAR_HEIGHT - UI_TITLEBAR_HEIGHT) - UI_STATUS_FONT_SIZE) / 2); 
        DrawTextApp(state, statusText, screenW/2 - (int)statusSize.x/2, textY, UI_STATUS_FONT_SIZE, statusColor);
    }


   
    Rectangle closeRect = { screenW - UI_BTN_SIZE, 0, UI_BTN_SIZE, UI_BTN_SIZE };
    Rectangle maxRect =   { screenW - UI_BTN_SIZE*2, 0, UI_BTN_SIZE, UI_BTN_SIZE };
    Rectangle minRect =   { screenW - UI_BTN_SIZE*3, 0, UI_BTN_SIZE, UI_BTN_SIZE };
    
    Vector2 mouse = GetMousePosition();
    bool hoverClose = CheckCollisionPointRec(mouse, closeRect);
    bool hoverMax   = CheckCollisionPointRec(mouse, maxRect);
    bool hoverMin   = CheckCollisionPointRec(mouse, minRect);

    static int clickedButtonId = 0; 

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (hoverClose) clickedButtonId = 1;
        else if (hoverMax) clickedButtonId = 2;
        else if (hoverMin) clickedButtonId = 3;
        else if (CheckCollisionPointRec(mouse, titleBarRec) && !state->isResizing) {
            state->isWindowDragging = true;
            state->mouseStartGlobal = GetMousePositionGlobal();
            Vector2 winPos = GetWindowPosition();
            state->windowBoundsStart.x = winPos.x;
            state->windowBoundsStart.y = winPos.y;
        }
    }

    bool shouldClose = false;

    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        if (clickedButtonId == 1 && hoverClose) shouldClose = true;
        else if (clickedButtonId == 2 && hoverMax) {
             if (!state->isMaximized) {
                state->oldWindowPos = (Rectangle){ (float)GetWindowPosition().x, (float)GetWindowPosition().y, (float)screenW, (float)GetScreenHeight() };
                int monitor = GetCurrentMonitor();
                SetWindowSize(GetMonitorWidth(monitor), GetMonitorHeight(monitor));
                SetWindowPosition(0, 0);
                state->isMaximized = true;
            } else {
                SetWindowSize((int)state->oldWindowPos.width, (int)state->oldWindowPos.height);
                SetWindowPosition((int)state->oldWindowPos.x, (int)state->oldWindowPos.y);
                state->isMaximized = false;
            }
        }
        else if (clickedButtonId == 3 && hoverMin) MinimizeWindow();

        clickedButtonId = 0;
        state->isWindowDragging = false;
    }

    float iconSize = 30.0f;
    float offset = (UI_BTN_SIZE - iconSize) / 2.0f;
    Color colClose = hoverClose ? (clickedButtonId == 1 ? RED : (Color){231, 76, 60, 255}) : BLANK;
    DrawRectangleRounded(closeRect,0.2f,10, colClose);
    Rectangle srcClose = { 0, 0, (float)state->iconClose.width, (float)state->iconClose.height };
    Rectangle destClose = { closeRect.x + offset, closeRect.y + (UI_BTN_SIZE - iconSize)/2, iconSize, iconSize };
    DrawTexturePro(state->iconClose, srcClose, destClose, (Vector2){0,0}, 0.0f, WHITE);

    Color colMax = hoverMax ? (Color){60,60,60,255} : BLANK;
    DrawRectangleRounded(maxRect,0.2f,10, colMax);
    Rectangle srcMax = { 0, 0, (float)state->iconMax.width, (float)state->iconMax.height };
    Rectangle destMax = { maxRect.x + offset, maxRect.y + (UI_BTN_SIZE - iconSize)/2, iconSize, iconSize };
    DrawTexturePro(state->iconMax, srcMax, destMax, (Vector2){0,0}, 0.0f, WHITE);

    Color colMin = hoverMin ? (Color){60,60,60,255} : BLANK;
    DrawRectangleRounded(minRect,0.2f,10, colMin);
    Rectangle srcMin = { 0, 0, (float)state->iconMin.width, (float)state->iconMin.height };
    Rectangle destMin = { minRect.x + offset, minRect.y + (UI_BTN_SIZE - iconSize)/2, iconSize, iconSize };
    DrawTexturePro(state->iconMin, srcMin, destMin, (Vector2){0,0}, 0.0f, WHITE);

    DrawRectangle(0, 40, screenW, 2, (Color){57, 56, 64, 255});

    if (state->isWindowDragging) {
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            Vector2 currentGlobal = GetMousePositionGlobal();
            float dx = currentGlobal.x - state->mouseStartGlobal.x;
            float dy = currentGlobal.y - state->mouseStartGlobal.y;
            
            SetWindowPosition((int)(state->windowBoundsStart.x + dx), 
                              (int)(state->windowBoundsStart.y + dy));
        } else {
            state->isWindowDragging = false;
        }
    }
    DrawMenuBar(state, video, ts, tracker);
    return shouldClose;
}


void DrawSidePanels(struct UIState *ui, struct TrackingSystem *ts, struct VideoEngine *v, struct AutoTracker *tracker, const char* filename) {
    DrawSidebar(ui);
    DrawRightPanel(ui, ts, v, tracker, filename);
}