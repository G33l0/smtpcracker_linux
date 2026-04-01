#ifndef THEME_H
#define THEME_H

#include <windows.h>
#include <DarkMode.h>

typedef struct {
    UINT nameID;
    DarkMode_Colors colors;
    BOOL isLight; // Flag to identify light themes
} Theme;

// This helper macro is fine to keep in the header
#define DEFINE_THEME(nameID, light, bg, ctrlBg, hotBg, dlgBg, errBg, txt, darkTxt, disTxt, linkTxt, edge, hotEdge, disEdge) \
    { nameID, { bg, ctrlBg, hotBg, dlgBg, errBg, txt, darkTxt, disTxt, linkTxt, edge, hotEdge, disEdge }, light }
// --- EXTERNAL DECLARATIONS ---
// Declare the variables here without initializing them.
// The actual data will be in theme.c
extern Theme g_DarkThemes[];
extern Theme g_LightThemes[];
extern const int g_numDarkThemes;
extern const int g_numLightThemes;
extern int g_currentThemeIndex;
extern BOOL g_isCurrentThemeDark;

// --- Public API ---
void ApplyTheme(HWND hwnd, int themeIndex, BOOL isDark);

#endif // THEME_H