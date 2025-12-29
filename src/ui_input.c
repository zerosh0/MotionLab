#include "ui_input.h"
#include "ui_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "theme.h"


static void DeleteTextRange(char* buffer, int start, int end) {
    int len = (int)strlen(buffer);
    if (start < 0) start = 0;
    if (end > len) end = len;
    if (start >= end) return;
    memmove(buffer + start, buffer + end, len - end + 1);
}

static void InsertText(char* buffer, int maxLen, int* cursor, const char* textToInsert) {
    int curLen = (int)strlen(buffer);
    int insLen = (int)strlen(textToInsert);
    if (curLen + insLen >= maxLen) return;
    memmove(buffer + *cursor + insLen, buffer + *cursor, curLen - *cursor + 1);
    memcpy(buffer + *cursor, textToInsert, insLen);
    *cursor += insLen;
}


static int GetCharIndexUnderMouse(Font font, const char* text, float mouseRelX) {
    int len = (int)strlen(text);
    int bestIndex = 0;
    float minDiff = 10000.0f;
    
    for(int i=0; i<=len; i++) {
        char tmp[64];
        strncpy(tmp, text, i);
        tmp[i] = '\0';
        Vector2 sz = MeasureTextEx(font, tmp, 14.0f, 1.0f);
        
        float diff = fabsf(sz.x - mouseRelX);
        if (diff < minDiff) {
            minDiff = diff;
            bestIndex = i;
        }
    }
    return bestIndex;
}


bool GuiFloatInput(struct UIState *ui, Rectangle bounds, float *value, const char* suffix) {
    Vector2 mouse = GetMousePosition();
    bool hover = CheckCollisionPointRec(mouse, bounds);
    
    Font font = ui->appFont;
    float fontSize = 14.0f;
    float textPadding = 5.0f;


    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (hover) {
            if (!ui->isEditingDist) {
                ui->isEditingDist = true;
                sprintf(ui->distInputBuf, "%.3f", *value);
                ui->distCursorIndex = (int)strlen(ui->distInputBuf);
                ui->distSelectStart = 0;
            } else {
                float clickX = mouse.x - (bounds.x + textPadding) + ui->distScrollOffset;
                int idx = GetCharIndexUnderMouse(font, ui->distInputBuf, clickX);
                
                ui->distCursorIndex = idx;

                if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
                    if (ui->distSelectStart == -1) ui->distSelectStart = ui->distCursorIndex;
                } else {
                    ui->distSelectStart = idx;
                }
            }
        } else {
            if (ui->isEditingDist) {
                ui->isEditingDist = false;
                ui->distSelectStart = -1;
                if (strlen(ui->distInputBuf) > 0) *value = (float)atof(ui->distInputBuf);
            }
        }
    }

    if (ui->isEditingDist && IsMouseButtonDown(MOUSE_LEFT_BUTTON) && hover) {
        if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            float clickX = mouse.x - (bounds.x + textPadding) + ui->distScrollOffset;
            int idx = GetCharIndexUnderMouse(font, ui->distInputBuf, clickX);

            ui->distCursorIndex = idx;
        }
    }

    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && ui->isEditingDist) {
        if (ui->distSelectStart == ui->distCursorIndex) {
            ui->distSelectStart = -1;
        }
    }


    if (ui->isEditingDist) {
        bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
        bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
        int len = (int)strlen(ui->distInputBuf);

        if (IsKeyPressed(KEY_ENTER)) {
            ui->isEditingDist = false;
            ui->distSelectStart = -1;
            if (len > 0) *value = (float)atof(ui->distInputBuf);
            return true;
        }

        if (ctrl && IsKeyPressed(KEY_A)) {
            ui->distSelectStart = 0;
            ui->distCursorIndex = len;
        }

        if (ctrl && IsKeyPressed(KEY_C) && ui->distSelectStart != -1) {
            int start = (ui->distSelectStart < ui->distCursorIndex) ? ui->distSelectStart : ui->distCursorIndex;
            int end = (ui->distSelectStart > ui->distCursorIndex) ? ui->distSelectStart : ui->distCursorIndex;
            char clipBuf[64];
            int clipLen = end - start;
            if (clipLen > 0 && clipLen < 63) {
                strncpy(clipBuf, ui->distInputBuf + start, clipLen);
                clipBuf[clipLen] = '\0';
                SetClipboardText(clipBuf);
            }
        }

        if (ctrl && IsKeyPressed(KEY_V)) {
            const char* clipText = GetClipboardText();
            if (clipText != NULL) {
                if (ui->distSelectStart != -1) {
                    int start = (ui->distSelectStart < ui->distCursorIndex) ? ui->distSelectStart : ui->distCursorIndex;
                    int end = (ui->distSelectStart > ui->distCursorIndex) ? ui->distSelectStart : ui->distCursorIndex;
                    DeleteTextRange(ui->distInputBuf, start, end);
                    ui->distCursorIndex = start;
                    ui->distSelectStart = -1;
                }
                char cleanText[64];
                int cleanIdx = 0;
                for(int i=0; clipText[i] != '\0' && cleanIdx < 60; i++) {
                    if (isdigit(clipText[i]) || clipText[i] == '.' || clipText[i] == ',') {
                        cleanText[cleanIdx++] = (clipText[i] == ',') ? '.' : clipText[i];
                    }
                }
                cleanText[cleanIdx] = '\0';
                InsertText(ui->distInputBuf, 63, &ui->distCursorIndex, cleanText);
            }
        }

        if (IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) {
            if (shift && ui->distSelectStart == -1) ui->distSelectStart = ui->distCursorIndex;
            else if (!shift && ui->distSelectStart != -1) ui->distSelectStart = -1;
            ui->distCursorIndex--;
            if (ui->distCursorIndex < 0) ui->distCursorIndex = 0;
        }
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) {
            if (shift && ui->distSelectStart == -1) ui->distSelectStart = ui->distCursorIndex;
            else if (!shift && ui->distSelectStart != -1) ui->distSelectStart = -1;
            ui->distCursorIndex++;
            if (ui->distCursorIndex > len) ui->distCursorIndex = len;
        }

        if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
            if (ui->distSelectStart != -1) {
                int start = (ui->distSelectStart < ui->distCursorIndex) ? ui->distSelectStart : ui->distCursorIndex;
                int end = (ui->distSelectStart > ui->distCursorIndex) ? ui->distSelectStart : ui->distCursorIndex;
                DeleteTextRange(ui->distInputBuf, start, end);
                ui->distCursorIndex = start;
                ui->distSelectStart = -1;
            } else if (ui->distCursorIndex > 0) {
                DeleteTextRange(ui->distInputBuf, ui->distCursorIndex - 1, ui->distCursorIndex);
                ui->distCursorIndex--;
            }
        }

        int key = GetCharPressed();
        while (key > 0) {
            if ((key >= 48 && key <= 57) || key == 46 || key == 44) {
                if (key == 44) key = 46;
                if (ui->distSelectStart != -1) {
                    int start = (ui->distSelectStart < ui->distCursorIndex) ? ui->distSelectStart : ui->distCursorIndex;
                    int end = (ui->distSelectStart > ui->distCursorIndex) ? ui->distSelectStart : ui->distCursorIndex;
                    DeleteTextRange(ui->distInputBuf, start, end);
                    ui->distCursorIndex = start;
                    ui->distSelectStart = -1;
                }
                char str[2] = { (char)key, '\0' };
                InsertText(ui->distInputBuf, 63, &ui->distCursorIndex, str);
            }
            key = GetCharPressed();
        }
    }

    char textBeforeCursor[64];
    strncpy(textBeforeCursor, ui->distInputBuf, ui->distCursorIndex);
    textBeforeCursor[ui->distCursorIndex] = '\0';
    float cursorPixelX = MeasureTextEx(font, textBeforeCursor, fontSize, 1.0f).x;
    
    float visibleWidth = bounds.width - (textPadding * 2);
    if (cursorPixelX - ui->distScrollOffset > visibleWidth) ui->distScrollOffset = cursorPixelX - visibleWidth;
    if (cursorPixelX - ui->distScrollOffset < 0) ui->distScrollOffset = cursorPixelX;
    
    float totalWidth = MeasureTextEx(font, ui->distInputBuf, fontSize, 1.0f).x;
    if (totalWidth < visibleWidth) ui->distScrollOffset = 0;

    Color bgColor = ui->isEditingDist ? (Color){30, 30, 30, 255} : (Color){50, 50, 50, 255};
    Color borderColor = ui->isEditingDist ? COLOR_ACCENT : (Color){80, 80, 80, 255};
    
    DrawRectangleRounded(bounds, 0.3f, 4, bgColor);
    DrawRectangleRoundedLines(bounds, 0.3f, 4, borderColor);

    BeginScissorMode((int)bounds.x + 2, (int)bounds.y + 2, (int)bounds.width - 4, (int)bounds.height - 4);
        float drawX = bounds.x + textPadding - ui->distScrollOffset;
        float drawY = bounds.y + (bounds.height - fontSize)/2;

        if (ui->isEditingDist && ui->distSelectStart != -1) {
            int start = (ui->distSelectStart < ui->distCursorIndex) ? ui->distSelectStart : ui->distCursorIndex;
            int end = (ui->distSelectStart > ui->distCursorIndex) ? ui->distSelectStart : ui->distCursorIndex;
            char t1[64], t2[64];
            strncpy(t1, ui->distInputBuf, start); t1[start] = '\0';
            strncpy(t2, ui->distInputBuf, end); t2[end] = '\0';
            float x1 = MeasureTextEx(font, t1, fontSize, 1.0f).x;
            float x2 = MeasureTextEx(font, t2, fontSize, 1.0f).x;
            DrawRectangle((int)(drawX + x1), (int)bounds.y + 4, (int)(x2 - x1), (int)bounds.height - 8, (Color){0, 120, 215, 100});
        }


        bool needMorePrecision = (fabsf(*value * 100.0f - roundf(*value * 100.0f)) > 0.001f);

        const char* fmt = needMorePrecision ? "%.3f %s" : "%.2f %s";

        const char* displayStr = ui->isEditingDist ? ui->distInputBuf : TextFormat(fmt, *value, suffix ? suffix : "");

        DrawTextEx(font, displayStr, (Vector2){drawX, drawY}, fontSize, 1.0f, WHITE);

        if (ui->isEditingDist && ((int)(GetTime() * 2) % 2) == 0) {
            DrawLine((int)(drawX + cursorPixelX), (int)bounds.y + 5, (int)(drawX + cursorPixelX), (int)(bounds.y + bounds.height - 5), COLOR_ACCENT);
        }
    EndScissorMode();

    return ui->isEditingDist;
}