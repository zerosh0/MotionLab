#include "lang.h"
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <windows.h>
#endif

Language currentLang = LANG_EN; 

void InitLanguage(void) {
#ifdef _WIN32
    LANGID langId = GetUserDefaultUILanguage();

    if (PRIMARYLANGID(langId) == LANG_FRENCH) {
        currentLang = LANG_FR;
    } else {
        currentLang = LANG_EN;
    }
#else
    const char* envLang = getenv("LANG");
    if (!envLang) envLang = getenv("LANGUAGE");

    if (envLang && strncmp(envLang, "fr", 2) == 0) {
        currentLang = LANG_FR;
    } else {
        currentLang = LANG_EN;
    }
#endif
}

static const char* dictionary[STR_COUNT][LANG_COUNT] = {
    // Menus
    [T_FILE]          = {"Fichier", "File"},
    [T_EXPORT]        = {"Exporter", "Export"},
    [T_GRAPHS]        = {"Graphiques", "Graphs"},
    [T_HELP]          = {"Aide", "Help"},
    [T_OPEN_VIDEO]    = {"Ouvrir une vidéo...", "Open Video..."},
    [T_OPEN_PROJET]   = {"Ouvrir un projet", "Open Project"},
    [T_SAVE_PROJET]   = {"Enregistrer projet", "Save Project"},
    [T_QUIT]          = {"Quitter", "Quit"},
    [T_TO_REGRESSI]   = {"Vers Regressi (.txt)", "To Regressi (.txt)"},
    [T_TO_EXCEL]      = {"Vers Excel (.csv)", "To Excel (.csv)"},
    [T_COPY_CLIPBOARD]= {"Copier presse-papier", "Copy to Clipboard"},
    [T_VIEW_CURVES]   = {"Visualiser les courbes", "View Curves"},
    [T_USER_GUIDE]    = {"Guide utilisateur", "User Guide"},
    [T_LANGUAGE]      = {"Langue : FR", "Language: EN"},
    [T_MSG_LOADED]    = {"Projet chargé", "Project loaded"},

    // Filtres SFD
    [T_FILTER_CSV]    = {"Fichier CSV\0*.csv\0", "CSV File\0*.csv\0"},
    [T_FILTER_TXT]    = {"Fichier Regressi\0*.txt\0", "Regressi File\0*.txt\0"},
    [T_FILTER_LAB]    = {"Projet MotionLab\0*.lab\0", "MotionLab Project\0*.lab\0"},

    // En-têtes
    [T_HEADER_TIME]   = {"Temps", "Time"},
    [T_HEADER_X]      = {"Abscisse", "X-Axis"},
    [T_HEADER_Y]      = {"Ordonnée", "Y-Axis"},
    [T_DEFAULT_PROJECT_NAME] = {"mon_projet", "my_project"},
    [T_UNTITLED]      = {"Projet Sans Titre", "Untitled Project"},

    // Canevas
    [T_MAGNIFIER]     = {"Loupe", "Magnifier"},
    [T_OUT_OF_ZONE]   = {"Hors zone", "Out of area"},
    [T_DRAG_VIDEO]    = {"Glissez une vidéo ici", "Drag a video here"},
    [T_INITIALIZING]  = {"INITIALISATION...", "INITIALIZING..."},
    [T_READY_SPACE]   = {"PRÊT : ESPACE", "READY: SPACE"},
    [T_TRACKING_MSG]  = {"Tracking...", "Tracking..."},
    [T_LOST_RETRY]    = {"PERDU (Réessayez)", "LOST (Try again)"},
    [T_POINT_A]       = {"Point A", "Point A"},
    [T_POINT_B]       = {"Point B", "Point B"},

    // Statuts
    [T_NO_FILE]           = {"Aucun fichier", "No file loaded"},
    [T_STATUS_IDLE_TRACK] = {"Cliquez-glissez pour entourer l'objet", "Click-drag to select object"},
    [T_STATUS_SELECTING]  = {"Relâchez pour valider", "Release to validate"},
    [T_STATUS_INITIALIZING]= {"Initialisation du tracker...", "Initializing tracker..."},
    [T_STATUS_READY]      = {"Prêt : Espace pour suivre", "Ready: Space to track"},
    [T_STATUS_TRACKING]   = {"Suivi en cours...", "Tracking..."},
    [T_STATUS_LOST]       = {"Cible perdue", "Target lost"},

    // Graphiques
    [T_GRAPH_WINDOW_TITLE] = {"Visualisation des courbes", "Curves Visualization"},
    [T_SHOW_FIT]           = {"Show Fit", "Show Fit"},
    [T_HIDE_FIT]           = {"Hide Fit", "Hide Fit"},
    [T_SHOW_FILL]          = {"Show Fill", "Show Fill"},
    [T_HIDE_FILL]          = {"Hide Fill", "Hide Fill"},
    [T_FIT_NA]             = {"Fit: N/A", "Fit: N/A"},

    // Onglets & Panneaux
    [T_TAB_MEASURES]    = {"Mesures", "Measures"},
    [T_TAB_CALIB]       = {"Étalonnage", "Calibration"},
    [T_TAB_INFO]        = {"Détails", "Details"},
    [T_AXIS_ORIENTATION]= {"ORIENTATION DES AXES", "AXIS ORIENTATION"},
    [T_ORIGIN_POS]      = {"POSITION ORIGINE", "ORIGIN POSITION"},
    [T_CLICK_ON_VIDEO]  = {"Cliquez sur la vidéo...", "Click on the video..."},
    [T_PLACE_ORIGIN]    = {"Placer l'origine", "Place origin"},
    [T_SCALE]           = {"ÉCHELLE", "SCALE"},
    [T_DEFINE_SCALE_AB] = {"Définir (Cliquez A puis B)", "Define (Click A then B)"},
    [T_DEFINE_SCALE]    = {"Définir l'échelle", "Define scale"},
    [T_REAL_DIST]       = {"Distance réelle :", "Real distance:"},
    [T_PX_PER_METER]    = {"1 m = %.1f pixels", "1 m = %.1f pixels"},

    // Détails Vidéo
    [T_FILE_SECTION]    = {"FICHIER", "FILE"},
    [T_VIDEO_SECTION]   = {"VIDÉO", "VIDEO"},
    [T_ENCODING_SECTION]= {"ENCODAGE", "ENCODING"},
    [T_RES]             = {"Résolution", "Resolution"},
    [T_FPS]             = {"Cadence", "Framerate"},
    [T_DURATION]        = {"Durée", "Duration"},
    [T_TOTAL_FRAMES]    = {"Total Images", "Total Frames"},
    [T_CODEC]           = {"Codec", "Codec"},
    [T_PIXELS]          = {"Format Pixels", "Pixel Format"},

    // Guide Utilisateur
    [T_HELP_TITLE]      = {"Guide Utilisateur", "User Guide"},
    [T_HELP_S1_TITLE]   = {"1. IMPORTATION", "1. IMPORTING"},
    [T_HELP_S1_B1]      = {"- Glissez une vidéo ou faites Ctrl+O.", "- Drag a video or press Ctrl+O."},
    [T_HELP_S1_B2]      = {"- Placez-vous au début du mouvement.", "- Move to the start of motion."},
    [T_HELP_S2_TITLE]   = {"2. ÉTALONNAGE", "2. CALIBRATION"},
    [T_HELP_S2_B1]      = {"- Origine : Cliquez 'Placer l'origine'.", "- Origin: Click 'Place origin'."},
    [T_HELP_S2_B2]      = {"- Échelle : Cliquez 'Définir' (Points A et B).", "- Scale: Click 'Define' (Points A & B)."},
    [T_HELP_S2_B3]      = {"- Distance : Entrez la valeur réelle.", "- Distance: Enter the real value."},
    [T_HELP_S3_TITLE]   = {"3. SUIVI DU MOUVEMENT", "3. MOTION TRACKING"},
    [T_HELP_S3_B1]      = {"- Manuel : Outil 'Point'.", "- Manual: 'Point' tool."},
    [T_HELP_S3_B2]      = {"- Auto : Outil 'Track' (Encadrez l'objet).", "- Auto: 'Track' tool (surround object)."},
    [T_HELP_S3_B3]      = {"-> ESPACE pour démarrer/arrêter.", "-> SPACE to start/stop."},
    [T_HELP_S4_TITLE]   = {"4. ANALYSE", "4. ANALYSIS"},
    [T_HELP_S4_B1]      = {"- Touche F2 : Voir les graphiques", "- F2 Key: View graphs"},
    [T_HELP_S4_B2]      = {"- Menu Exporter : CSV, Regressi, etc.", "- Export Menu: CSV, Regressi, etc."},

    // Réglages Communs
    [T_AUTO_ADVANCE]      = {"Point suivant auto", "Auto advance"},
    [T_SHOW_POINTS_TOGGLE]= {"Afficher les points", "Show points"},
    [T_SHOW_AXES_TOGGLE]  = {"Afficher les axes", "Show axes"},
    [T_FIRST_FRAME]       = {"Première image", "First frame"},
};

const char* L(TextID id) {
    if (id >= STR_COUNT) return "";
    return dictionary[id][currentLang];
}