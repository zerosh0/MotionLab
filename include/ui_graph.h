#ifndef UI_GRAPH_H
#define UI_GRAPH_H

#include "raylib.h"
#include "tracking.h"
#include "ui_core.h"


typedef enum {
    GRAPH_Y_X,
    GRAPH_X_T,
    GRAPH_Y_T,
    GRAPH_VX_T,
    GRAPH_VY_T,
    GRAPH_AX_T,
    GRAPH_AY_T
} GraphMode;


typedef struct GraphState {
    Rectangle bounds;
    GraphMode mode;
    bool isDragging;
    Vector2 dragOffset;
    bool requestExport;
    bool showRegression;
    bool showFill;
} GraphState;

void InitGraphSystem(GraphState *state);
void DrawGraphWindow(UIState *ui, GraphState *state, TrackingSystem *ts);

#endif