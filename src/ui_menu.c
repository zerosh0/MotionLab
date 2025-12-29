#include "ui_menu.h"
#include "ui_action.h"
#include "theme.h"
#include <stdio.h>
#include "file_utils.h"
#include "lang.h"

static int activeMenu = MENU_NONE;

extern char currentFilePath[512];

#ifdef _WIN32
    #ifndef SW_SHOWNORMAL
        #define SW_SHOWNORMAL 1
    #endif
    unsigned long __stdcall ShellExecuteA(void* hwnd, const char* lpOperation, const char* lpFile, 
                                         const char* lpParameters, const char* lpDirectory, int nShowCmd);
#endif

void App_OpenURL(const char* url) {
    #ifdef _WIN32
        ShellExecuteA(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
    #else
        OpenURL(url);
    #endif
}

static void Action_OpenVideo_Internal(VideoEngine *v, TrackingSystem *ts) {
    char* path = sfd_open_file("Videos\0*.mp4;*.avi;*.mkv\0");
    if (path) {
        Video_Unload(v);
        if (Video_Load(v, path)) {
            strncpy(currentFilePath, path, 511);
            ts->count = 0;
            Tracking_Init(ts); 
        }
    }
}


bool DrawMenuItem(UIState *ui, Rectangle bounds, const char* label, const char* shortcut) {
    Vector2 mouse = GetMousePosition();
    bool hover = CheckCollisionPointRec(mouse, bounds);
    if (hover) {
        DrawRectangleRounded(bounds, 0.3f, 6, (Color){60, 60, 60, 255});
    }

    DrawTextApp(ui, label, (int)bounds.x + 12, (int)bounds.y + 7, 14, hover ? COLOR_CYAN : WHITE);
    if (shortcut) {
        int sw = MeasureText(shortcut, 10);
        DrawTextApp(ui, shortcut, (int)(bounds.x + bounds.width - sw - 12), (int)bounds.y + 9, 12, GRAY);
    }

    return (hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON));
}

void DrawMenuBar(UIState *ui, VideoEngine *v, TrackingSystem *ts, AutoTracker *at) {
    ui->isMenuOpen = (activeMenu != MENU_NONE);
    const char* menuLabels[] = { L(T_FILE), L(T_EXPORT), L(T_GRAPHS), L(T_HELP) };
    float startX = 135;
    float btnWidth = 90;
    float btnHeight = 28;
    float menuY = (45 - btnHeight) / 2;
    
    Vector2 mouse = GetMousePosition();
    bool mouseOverAnyMenu = false;

    for (int i = 0; i < MENU_COUNT; i++) {
        Rectangle btnRect = { startX + (i * btnWidth), menuY, btnWidth, btnHeight };
        bool overButton = CheckCollisionPointRec(mouse, btnRect);

        if (overButton) {
            activeMenu = i;
        }

        bool isActive = (activeMenu == i);
        Color btnTextColor = isActive ? COLOR_CYAN : (overButton ? WHITE : LIGHTGRAY);
        
        if (isActive || overButton) {
            DrawRectangleRounded(btnRect, 0.3f, 6, (Color){50, 50, 50, 255});
        }
        
        int tw = MeasureText(menuLabels[i], 14);
        DrawTextApp(ui, menuLabels[i], (int)(btnRect.x + (btnWidth - tw)/2), (int)btnRect.y + 7, 14, btnTextColor);

        if (isActive) {
            float itemHeight = 32;
            int itemCount = 0;
            if (i == MENU_FILE) itemCount = 4;
            else if (i == MENU_EXPORT) itemCount = 3;
            else if (i == MENU_GRAPHS) itemCount = 1;
            else if (i == MENU_HELP) itemCount = 3;

            Rectangle dropRect = { btnRect.x, 45, 210, (float)itemCount * itemHeight + 10 };
            Rectangle safeZone = { dropRect.x - 10, 0, dropRect.width + 20, dropRect.y + dropRect.height + 10 };
            
            if (CheckCollisionPointRec(mouse, safeZone)) {
                mouseOverAnyMenu = true;
                
                DrawRectangleRounded(dropRect, 0.1f, 8, (Color){30, 30, 30, 255});
                DrawRectangleRoundedLines(dropRect, 0.1f, 8, (Color){70, 70, 70, 255});

                float itemY = dropRect.y + 5;
                float itemWidth = dropRect.width - 10;
                float itemX = dropRect.x + 5;

                if (i == MENU_FILE) {
                    if (DrawMenuItem(ui, (Rectangle){itemX, itemY, itemWidth, itemHeight}, L(T_OPEN_VIDEO), "Ctrl+O")) {
                    Action_OpenVideo_Internal(v, ts);
                    activeMenu = MENU_NONE;
                    }
                    itemY += itemHeight;
                    if (DrawMenuItem(ui, (Rectangle){itemX, itemY, itemWidth, itemHeight}, L(T_OPEN_PROJET), NULL)) {
                        if (Action_LoadProject(ts, v, currentFilePath)) {
                            Video_Unload(v);
                            if (Video_Load(v, currentFilePath)) {
                                printf("%s : %s\n", L(T_MSG_LOADED), currentFilePath);
                            }
                        }
                        activeMenu = MENU_NONE;
                    }
                    itemY += itemHeight;
                    if (DrawMenuItem(ui, (Rectangle){itemX, itemY, itemWidth, itemHeight}, L(T_SAVE_PROJET), "Ctrl+S")) {
                        Action_SaveProject(ts,currentFilePath);
                        
                        activeMenu = MENU_NONE;
                    }
                    itemY += itemHeight;
                    if (DrawMenuItem(ui, (Rectangle){itemX, itemY, itemWidth, itemHeight}, L(T_QUIT), "Alt+F4")) {
                        activeMenu = MENU_NONE;
                        Video_Unload(v);
                        AutoTracker_Free(at);
                        CloseWindow();
                        exit(0);
                    }
                }
                else if (i == MENU_EXPORT) {
                    if (DrawMenuItem(ui, (Rectangle){itemX, itemY, itemWidth, itemHeight}, L(T_TO_REGRESSI), NULL)) {
                        Action_ExportRegressi(ts, currentFilePath);
                        activeMenu = MENU_NONE;
                    }
                    itemY += itemHeight;
                    if (DrawMenuItem(ui, (Rectangle){itemX, itemY, itemWidth, itemHeight}, L(T_TO_EXCEL), NULL)) {
                        Action_ExportCSV(ts, currentFilePath);
                        activeMenu = MENU_NONE;
                    }
                    itemY += itemHeight;
                    if (DrawMenuItem(ui, (Rectangle){itemX, itemY, itemWidth, itemHeight}, L(T_COPY_CLIPBOARD), NULL)) {
                        Action_CopyClipboard(ts);
                        activeMenu = MENU_NONE;
                    }
                }
                else if (i == MENU_GRAPHS) {
                    if (DrawMenuItem(ui, (Rectangle){itemX, itemY, itemWidth, itemHeight}, L(T_VIEW_CURVES), "F2")) { 
                        ui->showGraphWindow = !ui->showGraphWindow;
                        activeMenu = MENU_NONE; 
                    }

                }
                else if (i == MENU_HELP) {
                    if (DrawMenuItem(ui, (Rectangle){itemX, itemY, itemWidth, itemHeight}, L(T_USER_GUIDE), "F1")) { activeMenu = MENU_NONE;ui->showHelp = !ui->showHelp; }
                    itemY += itemHeight;
                    if (DrawMenuItem(ui, (Rectangle){itemX, itemY, itemWidth, itemHeight}, L(T_LANGUAGE), NULL)) {
                        currentLang = (currentLang == LANG_FR) ? LANG_EN : LANG_FR;
                    }
                    itemY += itemHeight;
                   if (DrawMenuItem(ui, (Rectangle){itemX, itemY, itemWidth, itemHeight}, "GitHub", NULL)) { 
                        App_OpenURL("https://github.com/zerosh0/MotionLab"); 
                        activeMenu = MENU_NONE; 
                    }
                }
            }
        }
    }

    if (!mouseOverAnyMenu) {
        if (mouse.y > 45 || mouse.y < 0 || mouse.x < startX || mouse.x > (startX + MENU_COUNT * btnWidth)) {
             activeMenu = MENU_NONE;
        }
    }
}

void HandleShortcuts(UIState *ui, VideoEngine *v, TrackingSystem *ts, AutoTracker *autoTracker) {

    bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);

    if (IsFileDropped()) {
        FilePathList droppedFiles = LoadDroppedFiles();
        if (droppedFiles.count > 0) {
            Video_Unload(v);
            if (Video_Load(v, droppedFiles.paths[0])) {
                strncpy(currentFilePath, droppedFiles.paths[0], 511);
                ts->count = 0;
                Tracking_Init(ts);
            }
        }
        UnloadDroppedFiles(droppedFiles);
    }


    if (autoTracker->pendingInit) {
        AutoTracker_ConfirmSelection(autoTracker, v);
        autoTracker->pendingInit = false;
    }
    if (autoTracker->state == TRACKER_READY && IsKeyPressed(KEY_SPACE)) {
        AutoTracker_StartTracking(autoTracker);
    }
    else if (autoTracker->state == TRACKER_TRACKING && IsKeyPressed(KEY_SPACE)) {
        AutoTracker_Stop(autoTracker);
        v->isPlaying = false;
    }
    if (autoTracker->state == TRACKER_TRACKING && autoTracker->needsToAdvance) {
        Video_NextFrame(v);
        v->isPlaying = false; 
        autoTracker->needsToAdvance = false;
    }


    if (ctrl && IsKeyPressed(KEY_O)) {
        Action_OpenVideo_Internal(v, ts);
    }

    if (ctrl && IsKeyPressed(KEY_S)) {
        if (ts->count > 0 || v->isLoaded) {
            Action_SaveProject(ts, currentFilePath);
        }
    }
    if (IsKeyPressed(KEY_F1)) {
        ui->showHelp = !ui->showHelp;
        if (ui->showHelp) ui->showGraphWindow = false;
    }
    if (IsKeyPressed(KEY_F2)) {
        ui->showGraphWindow = !ui->showGraphWindow;
        if (ui->showGraphWindow) ui->showHelp = false;
    }
    
    if (!v->isPlaying && v->isLoaded) {
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) {
            Video_NextFrame(v);
        }
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) {
            Video_PrevFrame(v);
        }
    }
}