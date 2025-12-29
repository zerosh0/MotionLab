#include "ui_graph.h"
#include "raylib.h"
#include "rlgl.h" 
#include <stdio.h>
#include <math.h>
#include <float.h> 
#include "theme.h"
#include "ui_panels.h"
#include "lang.h"

Vector2 GetPos(TrackingSystem *ts, int i) {
    if (i < 0) i = 0; if (i >= ts->count) i = ts->count - 1;
    return PixelToPhysical(ts, ts->points[i].pixelPos);
}

float CalculateVelocity(TrackingSystem *ts, int index, bool isX) {
    if (index <= ts->startFrame || index >= ts->count - 1) return 0.0f;
    
    Vector2 pPrev = GetPos(ts, index - 1); 
    Vector2 pNext = GetPos(ts, index + 1);
    float dt = ts->points[index + 1].time - ts->points[index - 1].time; 
    
    if (fabs(dt) < 1e-6) return 0.0f;
    return (isX ? (pNext.x - pPrev.x) : (pNext.y - pPrev.y)) / dt;
}

float CalculateAccel(TrackingSystem *ts, int index, bool isX) {

    if (index <= ts->startFrame || index >= ts->count - 1) return 0.0f;

    float vPrev = CalculateVelocity(ts, index - 1, isX);
    float vNext = CalculateVelocity(ts, index + 1, isX);
    float dt = ts->points[index + 1].time - ts->points[index - 1].time;

    if (fabs(dt) < 1e-6) return 0.0f;
    return (vNext - vPrev) / dt;
}

typedef enum { REG_LINEAR, REG_QUADRATIC } RegressionType;
typedef struct { RegressionType type; double a, b, c, rSquared; } RegressionResult;

float GetModelY(RegressionResult reg, float x) {
    if (reg.type == REG_LINEAR) return (float)(reg.a * x + reg.b);
    else return (float)(reg.a * x * x + reg.b * x + reg.c);
}

double CalculateRSquared(float* x, float* y, int count, RegressionResult reg) {
    double meanY = 0; 
    for(int i=0; i<count; i++) meanY += y[i]; 
    meanY /= count;
    
    double ssTot = 0, ssRes = 0;
    for(int i=0; i<count; i++) {
        double f = GetModelY(reg, x[i]);
        ssTot += (y[i] - meanY)*(y[i] - meanY); 
        ssRes += (y[i] - f)*(y[i] - f);
    }

    if (ssTot < 1e-9) return 1.0; 
    return 1.0 - (ssRes / ssTot);
}

RegressionResult CalcLinearReg(float* x, float* y, int count) {
    double sX=0, sY=0, sXY=0, sXX=0;
    for (int i=0; i<count; i++) { sX+=x[i]; sY+=y[i]; sXY+=x[i]*y[i]; sXX+=x[i]*x[i]; }
    
    double denom = count*sXX - sX*sX;
    
    if (fabs(denom) < 1e-9) return (RegressionResult){ REG_LINEAR, 0, 0, 0, 0 };

    double slope = (count*sXY - sX*sY) / denom;
    double intercept = (sY - slope*sX) / count;
    
    RegressionResult res = { REG_LINEAR, slope, intercept, 0, 0 };
    res.rSquared = CalculateRSquared(x, y, count, res);
    return res;
}

RegressionResult CalcQuadraticReg(float* x, float* y, int count) {
    double sX=0, sX2=0, sX3=0, sX4=0, sY=0, sXY=0, sX2Y=0;
    for (int i=0; i<count; i++) {
        double xi=x[i], yi=y[i], xi2=xi*xi;
        sX+=xi; sX2+=xi2; sX3+=xi2*xi; sX4+=xi2*xi2; sY+=yi; sXY+=xi*yi; sX2Y+=xi2*yi;
    }
    double n=(double)count;
    double D = n*(sX2*sX4 - sX3*sX3) - sX*(sX*sX4 - sX3*sX2) + sX2*(sX*sX3 - sX2*sX2);
    
    if (fabs(D)<1e-9) return (RegressionResult){ REG_QUADRATIC, 0,0,0,0 };
    
    double Da = n*(sX2*sX2Y - sXY*sX3) - sX*(sX*sX2Y - sXY*sX2) + sY*(sX*sX3 - sX2*sX2);
    double Db = n*(sXY*sX4 - sX3*sX2Y) - sY*(sX*sX4 - sX3*sX2) + sX2*(sX*sX2Y - sXY*sX2);
    double Dc = sY*(sX2*sX4 - sX3*sX3) - sX*(sXY*sX4 - sX3*sX2Y) + sX2*(sXY*sX3 - sX2*sX2Y);
    
    RegressionResult res = { REG_QUADRATIC, Da/D, Db/D, Dc/D, 0 };
    res.rSquared = CalculateRSquared(x, y, count, res);
    return res;
}


RegressionResult CalcBestFit(float* x, float* y, int count, GraphMode mode) {
    RegressionResult lin = CalcLinearReg(x, y, count);
    
    if (lin.rSquared < 0.15) return lin; 

    if (lin.rSquared > 0.98) return lin;

    RegressionResult quad = CalcQuadraticReg(x, y, count);
    
    float threshold = 0.02f;

    if (mode == GRAPH_VX_T || mode == GRAPH_VY_T) {
        threshold = 0.08f;
    } 
    else if (mode == GRAPH_AX_T || mode == GRAPH_AY_T) {
        threshold = 0.20f;
    }

    if (quad.rSquared > lin.rSquared + threshold) return quad; 
    return lin;
}


void InitGraphSystem(GraphState *state) {
    int sw = GetScreenWidth(); int sh = GetScreenHeight();
    state->bounds = (Rectangle){ (float)sw/2 - 400, (float)sh/2 - 280, 800, 560 };
    state->mode = GRAPH_Y_X; 
    state->isDragging = false;
    state->requestExport = false;
    state->showRegression = true;
    state->showFill = true; 
}

void DrawGraphContent(Rectangle bodyRect, GraphState *state, TrackingSystem *ts, UIState *ui) {
    Color graphBgColor = (Color){25, 25, 25, 255}; 

    rlDisableColorBlend();
    DrawRectangleRec(bodyRect, graphBgColor);
    rlEnableColorBlend();

    if (ts->count < 5) return;

    float minX = FLT_MAX, maxX = -FLT_MAX;
    float minY = FLT_MAX, maxY = -FLT_MAX;
    #define MAX_DRAW_POINTS 4096
    float valX[MAX_DRAW_POINTS]; float valY[MAX_DRAW_POINTS];
    float regX[MAX_DRAW_POINTS]; float regY[MAX_DRAW_POINTS];

    int effectiveStart = ts->startFrame; 
    int count = (ts->count < MAX_DRAW_POINTS) ? ts->count : MAX_DRAW_POINTS;

    int startI = effectiveStart;
    int endI = count;

    if (state->mode == GRAPH_VX_T || state->mode == GRAPH_VY_T) {
            startI = effectiveStart + 1; 
            endI = count - 1;
        } else if (state->mode == GRAPH_AX_T || state->mode == GRAPH_AY_T) {
            startI = effectiveStart + 2; 
            endI = count - 2;
        }

    if (startI >= endI) return;

    int validCount = 0;
    for (int i = startI; i < endI; i++) {
        Vector2 phys = PixelToPhysical(ts, ts->points[i].pixelPos);
        float t = ts->points[i].time;
        switch (state->mode) {
            case GRAPH_Y_X: valX[i]=phys.x; valY[i]=phys.y; break;
            case GRAPH_X_T: valX[i]=t; valY[i]=phys.x; break;
            case GRAPH_Y_T: valX[i]=t; valY[i]=phys.y; break;
            case GRAPH_VX_T: valX[i]=t; valY[i]=CalculateVelocity(ts, i, true); break;
            case GRAPH_VY_T: valX[i]=t; valY[i]=CalculateVelocity(ts, i, false); break;
            case GRAPH_AX_T: valX[i]=t; valY[i]=CalculateAccel(ts, i, true); break;
            case GRAPH_AY_T: valX[i]=t; valY[i]=CalculateAccel(ts, i, false); break;
            default: valX[i]=0; valY[i]=0;
        }
        regX[validCount] = valX[i]; 
        regY[validCount] = valY[i]; 
        validCount++;

        if (valX[i] < minX) minX = valX[i]; if (valX[i] > maxX) maxX = valX[i];
        if (valY[i] < minY) minY = valY[i]; if (valY[i] > maxY) maxY = valY[i];
    }

    float rangeX = maxX - minX; if(fabs(rangeX) < 1e-4) rangeX = 1.0f;
    float rangeY = maxY - minY; if(fabs(rangeY) < 1e-4) rangeY = 1.0f;
    
    if (IsKeyPressed(KEY_D)) {
        int totalCount = ts->count;
            printf("\n--- DEBUG DATA DANS DRAW_GRAPH_CONTENT (Count: %d) ---\n", totalCount);
            printf("Idx | Time  | PosY  | VelY  | AccY  |\n");
            for(int i=0; i<totalCount; i++) {
                float t = ts->points[i].time;
                Vector2 p = PixelToPhysical(ts, ts->points[i].pixelPos);
                float vy = CalculateVelocity(ts, i, false);
                float ay = CalculateAccel(ts, i, false);
                char used = (i >= startI && i < endI) ? '*' : ' ';
                printf("%02d%c | %.3f | %.3f | %.3f | %.3f |\n", i, used, t, p.y, vy, ay);
            }
            printf("------------------------------------------\n");
        }


    minX -= rangeX * 0.05f; maxX += rangeX * 0.05f;
    minY -= rangeY * 0.1f;  maxY += rangeY * 0.1f;

    RegressionResult reg;
    if (state->showRegression && validCount > 1) reg = CalcBestFit(regX, regY, validCount, state->mode);

    Vector2 mouse = GetMousePosition();
    int hoverIdx = -1;
    if (CheckCollisionPointRec(mouse, bodyRect)) {
        float closestDistX = FLT_MAX;
        for (int i = startI; i < endI; i++) {
            float nx = (valX[i] - minX) / (maxX - minX);
            float screenX = bodyRect.x + nx * bodyRect.width;
            float dist = fabs(mouse.x - screenX);
            if (dist < closestDistX) { closestDistX = dist; hoverIdx = i; }
        }
        if (closestDistX > 50.0f) hoverIdx = -1;
    }

    rlSetBlendMode(RL_BLEND_ALPHA);
    rlBegin(RL_LINES); rlColor4ub(60, 60, 60, 255);
    int gridDivs = 6;
    for (int i = 0; i <= gridDivs; i++) {
        float r = (float)i / gridDivs;
        float gx = bodyRect.x + r * bodyRect.width;
        float gy = bodyRect.y + bodyRect.height - (r * bodyRect.height);
        rlVertex2f(gx, bodyRect.y); rlVertex2f(gx, bodyRect.y + bodyRect.height);
        rlVertex2f(bodyRect.x, gy); rlVertex2f(bodyRect.x + bodyRect.width, gy);
    }
    rlEnd();

    for (int i = 0; i <= gridDivs; i += 2) {
        float r = (float)i / gridDivs;
        char lblX[32], lblY[32];
        sprintf(lblX, "%.2f", minX + r * (maxX - minX));
        sprintf(lblY, "%.2f", minY + r * (maxY - minY));
        DrawTextEx(ui->appFont, lblX, (Vector2){bodyRect.x + r * bodyRect.width + 2, bodyRect.y + bodyRect.height - 15}, 10, 1.0f, GRAY);
        DrawTextEx(ui->appFont, lblY, (Vector2){bodyRect.x + 5, bodyRect.y + bodyRect.height - (r * bodyRect.height) - 12}, 10, 1.0f, GRAY);
    }

    Color mainColor = COLOR_ACCENT; 
    if (state->mode == GRAPH_Y_T || state->mode == GRAPH_VY_T) mainColor = (Color){255, 80, 150, 255};
    if (state->mode == GRAPH_X_T || state->mode == GRAPH_VX_T) mainColor = (Color){0, 220, 100, 255};

    BeginScissorMode((int)bodyRect.x, (int)bodyRect.y, (int)bodyRect.width, (int)bodyRect.height);

    if (state->showRegression && validCount > 1) {
        Color fadeTop = mainColor; fadeTop.a = 150; 
        Color fadeBot = mainColor; fadeBot.a = 0;
        int steps = 200; float stepX = (maxX - minX) / steps; Vector2 prevP = {0};

        if (state->showFill) {
            rlDrawRenderBatchActive(); rlColorMask(true, true, true, false); 
            rlBegin(RL_TRIANGLES);
            for (int i = 0; i < steps; i++) {
                float xCur = minX + i * stepX; float xNxt = minX + (i + 1) * stepX;
                float yCur = GetModelY(reg, xCur); float yNxt = GetModelY(reg, xNxt);
                float nx1 = (xCur - minX)/(maxX - minX); float ny1 = (yCur - minY)/(maxY - minY);
                float nx2 = (xNxt - minX)/(maxX - minX); float ny2 = (yNxt - minY)/(maxY - minY);
                float sx1 = bodyRect.x + nx1 * bodyRect.width;
                float sy1 = bodyRect.y + bodyRect.height - (ny1 * bodyRect.height);
                float sx2 = bodyRect.x + nx2 * bodyRect.width;
                float sy2 = bodyRect.y + bodyRect.height - (ny2 * bodyRect.height);
                float base = bodyRect.y + bodyRect.height;

                rlColor4ub(fadeTop.r, fadeTop.g, fadeTop.b, fadeTop.a); rlVertex2f(sx1, sy1);
                rlColor4ub(fadeBot.r, fadeBot.g, fadeBot.b, fadeBot.a); rlVertex2f(sx1, base);
                rlColor4ub(fadeTop.r, fadeTop.g, fadeTop.b, fadeTop.a); rlVertex2f(sx2, sy2);
                rlColor4ub(fadeBot.r, fadeBot.g, fadeBot.b, fadeBot.a); rlVertex2f(sx1, base);
                rlColor4ub(fadeBot.r, fadeBot.g, fadeBot.b, fadeBot.a); rlVertex2f(sx2, base);
                rlColor4ub(fadeTop.r, fadeTop.g, fadeTop.b, fadeTop.a); rlVertex2f(sx2, sy2);
            }
            rlEnd();
            rlDrawRenderBatchActive(); rlColorMask(true, true, true, true); 
        }

        for (int i = 0; i <= steps; i++) {
            float xVal = minX + i * stepX; float yVal = GetModelY(reg, xVal);
            float nx = (xVal - minX) / (maxX - minX); float ny = (yVal - minY) / (maxY - minY);
            Vector2 p = { bodyRect.x + nx * bodyRect.width, bodyRect.y + bodyRect.height - (ny * bodyRect.height) };
            if (i > 0) DrawLineEx(prevP, p, 2.5f, mainColor);
            prevP = p;
        }
    }

    for (int i = startI; i < endI; i++) {
        float nx = (valX[i] - minX) / (maxX - minX);
        float ny = (valY[i] - minY) / (maxY - minY);
        Vector2 p = { bodyRect.x + nx * bodyRect.width, bodyRect.y + bodyRect.height - (ny * bodyRect.height) };
        
        if (i == hoverIdx) {
            DrawCircleV(p, 6.0f, WHITE); 
            DrawCircleLines((int)p.x, (int)p.y, 10.0f, mainColor);
        } else {
            DrawCircleV(p, 3.0f, (Color){200, 200, 200, 150}); 
        }
    }
    EndScissorMode(); 


    if (state->showRegression && validCount > 1) {
        static char eqBuffer[128]; 
        if (isnan(reg.a) || isnan(reg.b) || isnan(reg.rSquared)) {
             sprintf(eqBuffer, L(T_FIT_NA));
        } else if (reg.type == REG_LINEAR) {
            char signB = (reg.b >= 0) ? '+' : '-';
            sprintf(eqBuffer, "Fit: %.3f x %c %.3f (R²: %.2f)", reg.a, signB, fabsf(reg.b), reg.rSquared);
        } else {
            char signB = (reg.b >= 0) ? '+' : '-';
            char signC = (reg.c >= 0) ? '+' : '-';
            sprintf(eqBuffer, "Fit: %.3f x² %c %.3f x %c %.3f (R²: %.2f)", reg.a, signB, fabsf(reg.b), signC, fabsf(reg.c), reg.rSquared);
        }

        int fontSize = 20; 
        Vector2 txtSz = MeasureTextEx(ui->appFont, eqBuffer, fontSize, 1.0f);
        rlDrawRenderBatchActive(); rlColorMask(true, true, true, false); 
        DrawRectangle(bodyRect.x + bodyRect.width - txtSz.x - 10, bodyRect.y + 5, txtSz.x + 10, txtSz.y + 6, graphBgColor);
        rlDrawRenderBatchActive(); rlColorMask(true, true, true, true);
        DrawTextEx(ui->appFont, eqBuffer, (Vector2){bodyRect.x + bodyRect.width - txtSz.x - 5, bodyRect.y + 7}, fontSize, 1.0f, mainColor);
    }

    if (hoverIdx != -1) {
        float hValX = valX[hoverIdx]; float hValY = valY[hoverIdx];
        float nx = (hValX - minX) / (maxX - minX); float ny = (hValY - minY) / (maxY - minY);
        Vector2 target = { bodyRect.x + nx * bodyRect.width, bodyRect.y + bodyRect.height - (ny * bodyRect.height) };

        DrawLine(target.x, bodyRect.y, target.x, bodyRect.y + bodyRect.height, Fade(WHITE, 0.2f));
        DrawLine(bodyRect.x, target.y, bodyRect.x + bodyRect.width, target.y, Fade(WHITE, 0.2f));

        const char* txtX = TextFormat("t: %.3f", hValX); 
        if (state->mode == GRAPH_Y_X) txtX = TextFormat("x: %.3f", hValX);
        const char* txtY = TextFormat("v: %.3f", hValY);
        if (state->mode == GRAPH_Y_X || state->mode == GRAPH_Y_T) txtY = TextFormat("y: %.3f", hValY);
        if (state->mode == GRAPH_AX_T || state->mode == GRAPH_AY_T) txtY = TextFormat("a: %.3f", hValY);

        const char* fullTxt = TextFormat("%s\n%s", txtX, txtY);
        float tipFontSize = 20.0f; float spacing = 2.0f;
        Vector2 txtSz = MeasureTextEx(ui->appFont, fullTxt, tipFontSize, spacing);
        float pad = 10.0f;
        Rectangle badgeRect = { target.x + 15, target.y - 15, txtSz.x + (pad*2), txtSz.y + (pad*2) };

        if (badgeRect.x + badgeRect.width > bodyRect.x + bodyRect.width) badgeRect.x = target.x - badgeRect.width - 15;
        if (badgeRect.y + badgeRect.height > bodyRect.y + bodyRect.height) badgeRect.y = target.y - badgeRect.height - 15;
        if (badgeRect.y < bodyRect.y) badgeRect.y = bodyRect.y + 5;

        rlDrawRenderBatchActive(); rlColorMask(true, true, true, false);
        DrawRectangleRec(badgeRect, graphBgColor);
        rlDrawRenderBatchActive(); rlColorMask(true, true, true, true);

        DrawRectangleLinesEx(badgeRect, 1.5f, mainColor);
        DrawTextEx(ui->appFont, fullTxt, (Vector2){badgeRect.x + pad, badgeRect.y + pad}, tipFontSize, spacing, WHITE);
    }
}

void DrawGraphWindow(UIState *ui, GraphState *state, TrackingSystem *ts) {
    if (!ui->showGraphWindow) return;

    DrawRectangleRec(state->bounds, (Color){35, 35, 35, 255});
    DrawRectangleLinesEx(state->bounds, 1, (Color){60, 60, 60, 255});

    Rectangle header = { state->bounds.x, state->bounds.y, state->bounds.width, 35 };
    Rectangle closeBtn = { state->bounds.x + state->bounds.width - 35, state->bounds.y, 35, 35 };
    Vector2 mouse = GetMousePosition();
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, header) && !CheckCollisionPointRec(mouse, closeBtn)) {
        state->isDragging = true; state->dragOffset = (Vector2){ mouse.x - state->bounds.x, mouse.y - state->bounds.y };
    }
    if (state->isDragging) {
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) { state->bounds.x = mouse.x - state->dragOffset.x; state->bounds.y = mouse.y - state->dragOffset.y; } 
        else state->isDragging = false;
    }
    DrawRectangleRec(header, (Color){45, 45, 45, 255});
    DrawTextEx(ui->appFont, L(T_GRAPH_WINDOW_TITLE), (Vector2){state->bounds.x + 12, state->bounds.y + 8}, 16, 1.0f, LIGHTGRAY);
    if (GuiButton(ui, closeBtn, "X")) ui->showGraphWindow = false;

    const char* labels[] = { "y(x)", "x(t)", "y(t)", "Vx(t)", "Vy(t)", "Ax(t)", "Ay(t)" };
    float btnW = 70; int fontSize = 15;
    float startX = state->bounds.x + 15;
    float startY = state->bounds.y + 45;

    for (int i = 0; i < 7; i++) {
        Rectangle btnRect = { startX + i*(btnW+5), startY, btnW, 30 };
        bool isActive = (state->mode == (GraphMode)i);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, btnRect)) state->mode = (GraphMode)i;
        Color bg = isActive ? COLOR_ACCENT : (Color){50,50,50,255};
        DrawRectangleRounded(btnRect, 0.3f, 4, bg);
        Vector2 txtSz = MeasureTextEx(ui->appFont, labels[i], fontSize, 1.0f);
        DrawTextEx(ui->appFont, labels[i], (Vector2){btnRect.x + (btnW-txtSz.x)/2, btnRect.y + (30-txtSz.y)/2}, fontSize, 1.0f, WHITE);
    }

    Rectangle toggleRegBtn = { state->bounds.x + state->bounds.width - 190, startY, 80, 28 };
    if (GuiButton(ui, toggleRegBtn, state->showRegression ? L(T_HIDE_FIT) : L(T_SHOW_FIT))) state->showRegression = !state->showRegression;

    Rectangle fillBtn = { state->bounds.x + state->bounds.width - 100, startY, 90, 28 };
    if (GuiButton(ui, fillBtn, state->showFill ? L(T_HIDE_FILL) : L(T_SHOW_FILL))) {
        state->showFill = !state->showFill;
    }

    Rectangle graphBody = { state->bounds.x + 50, state->bounds.y + 90, state->bounds.width - 70, state->bounds.height - 120 };
    DrawGraphContent(graphBody, state, ts, ui);
    DrawRectangleLinesEx(graphBody, 1, (Color){80, 80, 80, 255});
}