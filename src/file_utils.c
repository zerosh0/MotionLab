#include "file_utils.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#if defined(_WIN32)
    #include <windows.h>
    #include <commdlg.h>
#else
    #include "tinyfiledialogs.h"
#endif

static const char** convert_filter_to_tinyfd(const char* winFilter, int* outCount) {
    static const char* patterns[32];
    static char buffer[1024];
    int count = 0;

    if (!winFilter || !*winFilter) {
        *outCount = 0;
        return NULL;
    }

    const char* p = winFilter;
    p += strlen(p) + 1; 

    if (*p) {
        strncpy(buffer, p, 1023);
        char* token = strtok(buffer, ";");
        while (token != NULL && count < 31) {
            patterns[count++] = token;
            token = strtok(NULL, ";");
        }
    }

    *outCount = count;
    return patterns;
}

char* sfd_open_file(const char* filter) {
#if defined(_WIN32)
    static char filename[MAX_PATH];
    OPENFILENAMEA ofn;
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = filename;
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn)) return filename;
    return NULL;
#else
    int count = 0;
    const char** patterns = convert_filter_to_tinyfd(filter, &count);
    
    const char* result = tinyfd_openFileDialog(
        "Ouvrir un fichier", "", count, patterns, "Fichiers supportés", 0
    );
    
    static char pathBuffer[1024];
    if (result) {
        TextCopy(pathBuffer, result);
        return pathBuffer;
    }
    return NULL;
#endif
}

char* sfd_save_file(const char* filter, const char* defaultExt, const char* initialName) {
#if defined(_WIN32)
    static char filename[MAX_PATH];
    if (initialName) snprintf(filename, MAX_PATH, "%s", initialName);
    else filename[0] = '\0';

    OPENFILENAMEA ofn;
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = filter;
    ofn.lpstrDefExt = defaultExt;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

    if (GetSaveFileNameA(&ofn)) return filename;
    return NULL;
#else
    int count = 0;
    const char** patterns = convert_filter_to_tinyfd(filter, &count);
    char defaultPath[1024] = "";
    if (initialName) snprintf(defaultPath, 1024, "./%s", initialName);

    const char* result = tinyfd_saveFileDialog(
        "Enregistrer", defaultPath, count, patterns, NULL
    );

    static char pathBuffer[1024];
    if (result) {
        TextCopy(pathBuffer, result);
        return pathBuffer;
    }
    return NULL;
#endif
}