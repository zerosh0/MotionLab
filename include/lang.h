#ifndef LANG_H
#define LANG_H

typedef enum { LANG_FR = 0, LANG_EN, LANG_COUNT } Language;

typedef enum {
    // Menus & Barre de titre
    T_FILE, T_EXPORT, T_GRAPHS, T_HELP,
    T_OPEN_VIDEO, T_OPEN_PROJET, T_SAVE_PROJET, T_QUIT,
    T_TO_REGRESSI, T_TO_EXCEL, T_COPY_CLIPBOARD,
    T_VIEW_CURVES, T_USER_GUIDE, T_LANGUAGE,
    T_MSG_LOADED,
    
    // Filtres de fichiers (SFD)
    T_FILTER_CSV, T_FILTER_TXT, T_FILTER_LAB,
    
    // En-têtes Export & Tableaux
    T_HEADER_TIME, T_HEADER_X, T_HEADER_Y,
    T_DEFAULT_PROJECT_NAME, T_UNTITLED,
    
    // Interface Canevas (Loupe, Drag)
    T_MAGNIFIER, T_OUT_OF_ZONE, T_DRAG_VIDEO,
    
    // États du Tracker (Overlay Vidéo)
    T_INITIALIZING, T_READY_SPACE, T_TRACKING_MSG, T_LOST_RETRY,
    T_POINT_A, T_POINT_B,
    
    // Statuts (Barre de titre / Info)
    T_NO_FILE,
    T_STATUS_IDLE_TRACK,
    T_STATUS_SELECTING,
    T_STATUS_INITIALIZING,
    T_STATUS_READY,
    T_STATUS_TRACKING,
    T_STATUS_LOST,
    
    // Fenêtre Graphiques
    T_GRAPH_WINDOW_TITLE,
    T_SHOW_FIT, T_HIDE_FIT,
    T_SHOW_FILL, T_HIDE_FILL,
    T_FIT_NA,
    
    // Onglets du panneau droit
    T_TAB_MEASURES, T_TAB_CALIB, T_TAB_INFO,
    
    // Paramètres Étalonnage
    T_AXIS_ORIENTATION, T_ORIGIN_POS, T_CLICK_ON_VIDEO, T_PLACE_ORIGIN,
    T_SCALE, T_DEFINE_SCALE_AB, T_DEFINE_SCALE, T_REAL_DIST, T_PX_PER_METER,
    
    // Détails Vidéo (Tab Info)
    T_FILE_SECTION, T_VIDEO_SECTION, T_ENCODING_SECTION,
    T_RES, T_FPS, T_DURATION, T_TOTAL_FRAMES, T_CODEC, T_PIXELS,
    
    // Guide Utilisateur (Aide F1)
    T_HELP_TITLE, 
    T_HELP_S1_TITLE, T_HELP_S1_B1, T_HELP_S1_B2,
    T_HELP_S2_TITLE, T_HELP_S2_B1, T_HELP_S2_B2, T_HELP_S2_B3,
    T_HELP_S3_TITLE, T_HELP_S3_B1, T_HELP_S3_B2, T_HELP_S3_B3,
    T_HELP_S4_TITLE, T_HELP_S4_B1, T_HELP_S4_B2,

    // Réglages Communs
    T_AUTO_ADVANCE,
    T_SHOW_POINTS_TOGGLE,
    T_SHOW_AXES_TOGGLE,
    T_FIRST_FRAME,
    
    STR_COUNT
} TextID;


extern Language currentLang;

void InitLanguage(void);
const char* L(TextID id);

#endif