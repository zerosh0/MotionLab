#ifndef UI_CORE_H
#define UI_CORE_H

#include "raylib.h"

struct AutoTracker;
struct VideoEngine;
struct TrackingSystem;
struct GraphState;

typedef enum { TOOL_SELECT, TOOL_POINT, TOOL_LOOP, TOOL_TRACK } ToolMode;
typedef enum { TAB_MEASURES, TAB_CALIB, TAB_INFO } PanelTab;

typedef struct {
    bool isVisible;
    Rectangle bounds;
    float zoomLevel;
    bool isDragging;
    Vector2 dragOffset;
} MagnifierState;

typedef struct UIState {
    ToolMode currentTool;
    PanelTab activeTab;
    bool autoAdvance;
    bool showPoints;
    bool showAxes;
    MagnifierState magnifier;   
    bool isWindowDragging;
    Vector2 dragStartMouseOffset;
    bool isResizing;
    int resizeEdge; // 0=None, 1=Left, 2=Right, 4=Top, 8=Bottom (Bitmask)
    Rectangle windowBoundsStart;
    Vector2 mouseStartGlobal;
    bool isMaximized;
    Rectangle oldWindowPos;
    Font appFont;
    Texture2D iconClose;
    Texture2D iconMax;
    Texture2D iconMin;
    Texture2D toolIcons[4];
    Texture2D iconPlay;
    Texture2D iconPause;
    Texture2D iconNext;
    Texture2D iconPrev;
    Texture2D axisIcons[4];
    bool isEditingDist;
    char distInputBuf[64];
    int distCursorIndex;
    int distSelectStart;
    float distScrollOffset;
    float tableScrollOffset;
    int tableHoveredRow;
    bool isDraggingTableScroll;
    Texture2D iconCheckOn;
    Texture2D iconCheckOff;
    bool isMenuOpen;
    bool showGraphWindow;
    bool showHelp;
} UIState;



void InitUI(UIState *state);
void UnloadUI(UIState *state);
bool DrawTitleBar(UIState *state, const char* fullPath, struct VideoEngine *video, struct TrackingSystem *ts, struct AutoTracker *tracker);
void DrawRectangleRoundedCustom(Rectangle rec, float roundness, int segments, Color color,
                                bool topLeft, bool topRight, bool bottomLeft, bool bottomRight);
void DrawTextApp(UIState *ui, const char *text, int x, int y, int fontSize, Color color);
void DrawSidePanels(struct UIState *ui, struct TrackingSystem *ts, struct VideoEngine *v, struct AutoTracker *tracker, const char* filename);
const char* GetFileNameFromPath(const char* path);
void HandleWindowResize(UIState *state);

#endif