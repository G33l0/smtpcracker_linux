#ifndef GLOBALS_H
#define GLOBALS_H
#include <windows.h>

// Application settings structure
typedef struct {
    // Appearance
    int  themeMode;    // 0=Dark, 1=Light, 2=System [cite: 36]
    int  themeIndex;   // Index within the theme array [cite: 37]
    
    // General
    wchar_t testEmail[256];
    int     numThreads;
    int     maxRetries;
    wchar_t saveFileName[MAX_PATH];
    
    // Telegram
    wchar_t telegramToken[256];
    wchar_t telegramID[128];
} AppSettings;



extern HWND main_hwnd;
extern HINSTANCE main_hInstance;
extern AppSettings g_settings;
extern HFONT g_hFontDefault;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

#endif