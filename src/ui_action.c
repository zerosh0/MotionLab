#include "tracking.h"
#include "video_engine.h"
#include "file_utils.h"
#include "raylib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "lang.h"

void CleanAndFormatFr(char* str) {
    if (strchr(str, '.') != NULL) {
        char* p = str + strlen(str) - 1;
        while (p > str && *p == '0') { *p = '\0'; p--; }
        if (p > str && *p == '.') *p = '\0';
    }
    
    if (currentLang == LANG_FR) {
        char* walk = str;
        while (*walk) { if (*walk == '.') *walk = ','; walk++; }
    }
}

const char* App_GetFileNameWithoutExt(const char* path) {
    if (path == NULL || path[0] == '\0' || strstr(path, "Projet Sans Titre") != NULL) 
        return "mon_projet";
    
    const char* base = GetFileName(path); 
    static char buffer[256];
    TextCopy(buffer, base);
    
    char* dot = strrchr(buffer, '.');
    if (dot) *dot = '\0'; 
    
    return buffer;
}

void Action_ExportCSV(struct TrackingSystem *ts, const char* videoName) {
    if (ts->count == 0) return;

    char defaultName[256];
    sprintf(defaultName, "export_%s.csv", App_GetFileNameWithoutExt(videoName));

    char* path = sfd_save_file(L(T_FILTER_CSV), "csv", defaultName);
    if (!path) return;

    FILE *f = fopen(path, "w");
    if (f) {
        fprintf(f, "%s (s);%s (m);%s (m)\n", L(T_HEADER_TIME), L(T_HEADER_X), L(T_HEADER_Y));
        for (int i = 0; i < ts->count; i++) {
            Vector2 phys = PixelToPhysical(ts, ts->points[i].pixelPos);
            char tBuf[64], xBuf[32], yBuf[32];
            sprintf(tBuf, "%.4lf", ts->points[i].time);
            sprintf(xBuf, "%.4f", phys.x);
            sprintf(yBuf, "%.4f", phys.y);

            CleanAndFormatFr(tBuf); CleanAndFormatFr(xBuf); CleanAndFormatFr(yBuf);
            fprintf(f, "%s;%s;%s\n", tBuf, xBuf, yBuf);
        }
        fclose(f);
    }
}

void Action_ExportRegressi(struct TrackingSystem *ts, const char* videoName) {
    if (ts->count == 0) return;
    
    char defaultName[256];
    sprintf(defaultName, "regressi_%s.txt", App_GetFileNameWithoutExt(videoName));
    
    char* path = sfd_save_file("Fichier Regressi\0*.txt\0", "txt", defaultName);
    if (!path) return;

    FILE *f = fopen(path, "w");
    if (f) {
        fprintf(f, "MotionLab Export\n");
        fprintf(f, "Video : %s\n", videoName);
        fprintf(f, "Donnees experimentales\n");
        fprintf(f, "t\tx\ty\n");
        fprintf(f, "s\tm\tm\n");
        fprintf(f, "Temps\tAbscisse\tOrdonnee\n");

        for (int i = 0; i < ts->count; i++) {
            Vector2 phys = PixelToPhysical(ts, ts->points[i].pixelPos);
            fprintf(f, "%.4lf\t%.4f\t%.4f\n", 
                    ts->points[i].time, 
                    phys.x, 
                    phys.y);
        }
        fclose(f);
    }
}

void Action_CopyClipboard(struct TrackingSystem *ts) {
    if (ts->count == 0) return;
    size_t bufferSize = ts->count * 100 + 128; 
    char* buffer = (char*)malloc(bufferSize);
    if (!buffer) return;

    strcpy(buffer, "t(s)\tx(m)\ty(m)\n");
    for (int i = 0; i < ts->count; i++) {
        Vector2 phys = PixelToPhysical(ts, ts->points[i].pixelPos);
        char line[128], tBuf[64], xBuf[32], yBuf[32];
        
        sprintf(tBuf, "%.4lf", ts->points[i].time);
        sprintf(xBuf, "%.4f", phys.x);
        sprintf(yBuf, "%.4f", phys.y);
        
        CleanAndFormatFr(tBuf); CleanAndFormatFr(xBuf); CleanAndFormatFr(yBuf);
        sprintf(line, "%s\t%s\t%s\n", tBuf, xBuf, yBuf);
        strcat(buffer, line);
    }
    SetClipboardText(buffer);
    free(buffer);
}



void Action_SaveProject(struct TrackingSystem *ts, const char* videoPath) {
    char defaultName[256];
    sprintf(defaultName, "%s.lab", App_GetFileNameWithoutExt(videoPath));
    
    char* path = sfd_save_file(L(T_FILTER_LAB), "lab", defaultName);
    if (!path) return;

    FILE *f = fopen(path, "w");
    if (!f) return;

    fprintf(f, "MOTIONLAB_V1\n");
    fprintf(f, "VIDEO|%s\n", videoPath);
    
    // Format : pxPerMeter | originX | originY | config | Ax | Ay | Bx | By | realDist | startFrame
    fprintf(f, "CALIB|%f|%f|%f|%d|%f|%f|%f|%f|%f|%d\n", 
        ts->calib.pxPerMeter, 
        ts->calib.origin.x, 
        ts->calib.origin.y, 
        ts->calib.config,
        ts->calib.scalePointA.x,
        ts->calib.scalePointA.y,
        ts->calib.scalePointB.x,
        ts->calib.scalePointB.y,
        ts->calib.realDistance,
        ts->startFrame
    );

    fprintf(f, "POINTS|%d\n", ts->count);
    
    for (int i = 0; i < ts->count; i++) {
        fprintf(f, "P|%lf|%f|%f\n", ts->points[i].time, ts->points[i].pixelPos.x, ts->points[i].pixelPos.y);
    }
    fclose(f);
}

bool Action_LoadProject(struct TrackingSystem *ts, struct VideoEngine *v, char* outVideoPath) {
    char* path = sfd_open_file("Projet MotionLab\0*.lab\0");
    if (!path) return false;

    FILE *f = fopen(path, "r");
    if (!f) return false;

    char line[MAX_POINTS];
    if (!fgets(line, sizeof(line), f) || strncmp(line, "MOTIONLAB", 9) != 0) {
        fclose(f);
        return false;
    }

    ts->count = 0;
    ts->startFrame = 0; 

    while (fgets(line, sizeof(line), f)) {
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') line[len-1] = '\0';

        if (strncmp(line, "VIDEO|", 6) == 0) {
            strcpy(outVideoPath, line + 6);
        }
        else if (strncmp(line, "CALIB|", 6) == 0) {
            int readCount = sscanf(line + 6, "%f|%f|%f|%d|%f|%f|%f|%f|%f|%d", 
                &ts->calib.pxPerMeter, 
                &ts->calib.origin.x, 
                &ts->calib.origin.y, 
                &ts->calib.config,
                &ts->calib.scalePointA.x,
                &ts->calib.scalePointA.y,
                &ts->calib.scalePointB.x,
                &ts->calib.scalePointB.y,
                &ts->calib.realDistance,
                &ts->startFrame
            );

            ts->calib.hasOriginSet = true;
            
            if (readCount < 10) {
                if (readCount >= 9) {
                    ts->startFrame = 0; 
                }
                else if (readCount >= 4) {
                    ts->calib.scalePointA = (Vector2){0,0};
                    ts->calib.scalePointB = (Vector2){100,0};
                    ts->calib.realDistance = 1.0f;
                    ts->startFrame = 0;
                }
            }
        }
        else if (strncmp(line, "P|", 2) == 0) {
            if (ts->count < MAX_POINTS) { 
                sscanf(line + 2, "%lf|%f|%f", &ts->points[ts->count].time, &ts->points[ts->count].pixelPos.x, &ts->points[ts->count].pixelPos.y);
                ts->count++;
            }
        }
    }
    fclose(f);
    return true;
}