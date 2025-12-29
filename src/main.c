#include "raylib.h"
#include "video_engine.h"
#include "ui_core.h"
#include "ui_canvas.h"
#include "ui_panels.h"
#include "theme.h"
#include "tracking.h"
#include <string.h>
#include "auto_tracker.h"
#include "ui_graph.h"
#include "resources.h"
#include "ui_menu.h"
#include "lang.h"

char currentFilePath[512] = { 0 };

int main(void) {
    SetConfigFlags(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TRANSPARENT | FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(1280, 800, "MotionLab");
    SetTargetFPS(60);
    InitLanguage();

    Image icon = LoadImageFromMemory(".png", logo_png_data, logo_png_size);
    if (icon.data != NULL) {
        ImageFormat(&icon, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8); 
        SetWindowIcon(icon); 
        UnloadImage(icon); 
    }


    VideoEngine video = { 0 };
    UIState ui = { 0 };
    TrackingSystem ts = { 0 };
    AutoTracker autoTracker;
    GraphState graphState;

    ts.scale = 100.0f;
    InitUI(&ui);
    AutoTracker_Init(&autoTracker);
    InitGraphSystem(&graphState);

    while (!WindowShouldClose()) {
        Rectangle videoArea = { 
            SIDEBAR_WIDTH + 20, 
            TITLEBAR_HEIGHT + 20, 
            (float)GetScreenWidth() - SIDEBAR_WIDTH - PANEL_WIDTH - 40, 
            (float)GetScreenHeight() - TITLEBAR_HEIGHT - TIMELINE_HEIGHT - 40 
        };

        HandleWindowResize(&ui);
        if (video.isLoaded) Video_Update(&video);
        UpdateVideoCanvas(&video, &ui, &ts, &autoTracker, videoArea);
        HandleShortcuts(&ui, &video, &ts,&autoTracker);
        AutoTracker_Update(&autoTracker, &video, &ts);
        
        BeginDrawing();
        ClearBackground(BLANK);
        if (ui.isMaximized) {
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), COLOR_BG);
        } else {
                DrawRectangleRounded((Rectangle){0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()}, WINDOW_BORDER_RADUIS, 10, COLOR_BG);
        }
            

        DrawVideoCanvas(&video, &ui, &ts, &autoTracker, videoArea);
        DrawSidePanels(&ui, &ts, &video, &autoTracker, currentFilePath);
        DrawBottomBar(&ui, &video, videoArea);
        DrawMagnifierWindow(&video, &ui, videoArea);    
        DrawGraphWindow(&ui, &graphState, &ts);
        DrawHelpWindow(&ui);
        if (DrawTitleBar(&ui, currentFilePath, &video, &ts, &autoTracker)) break;

        EndDrawing();
    }

    Video_Unload(&video);
    UnloadUI(&ui);
    AutoTracker_Free(&autoTracker);
    CloseWindow();
    return 0;
}