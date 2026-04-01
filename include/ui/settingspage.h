#ifndef SETTINGSPAGE_H
#define SETTINGSPAGE_H

#include <windows.h>

// Structure to hold all Settings Page control handles
typedef struct {
    HWND hPage;
    HWND hGroupGeneral;
    HWND hLabelEmail;
    HWND hEditEmail;
    HWND hLabelThread;
    HWND hSliderThread;
    HWND hLabelThreadPreview;
    HWND hLabelRetry;
    HWND hSliderRetry;
    HWND hLabelRetryPreview;
    HWND hLabelSaveFile;
    HWND hEditSaveFile;
    
    HWND hGroupTelegram;
    HWND hEditTelToken;
    HWND hEditTelID;
    
    HWND hGroupAppearance;
    HWND hLabelThemeMode;
    HWND hComboThemeMode;
    HWND hLabelColorScheme;
    HWND hComboColorScheme;
    
    HWND hBtnTestTelegram;
    HWND hBtnSaveSettings;
} SETTINGS_CONTROLS;

// Make the structure global
extern SETTINGS_CONTROLS g_SettingsUI;

HWND CreateSettingsPage(HWND parent);
BOOL Settings_PageProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void ResizeSettingsPage(int width, int height);
void Settings_PopulateColorSchemes(BOOL isDark);
#endif