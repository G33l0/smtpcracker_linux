#ifndef HOMEPAGE_H
#define HOMEPAGE_H
#include <windows.h>

// Structure to hold all Home Page control handles
typedef struct {
    HWND hPage;          // The main container (parent)
    
    // Top Buttons
    HWND hBtnUpload;
    HWND hBtnStart;
    
    // Status Labels
    HWND hLblCombos;
    HWND hLblValid;
    HWND hLblFailed;
    HWND hLblDelivery;
    
    // Main Content
    HWND hListMain;
    HWND hConsole;
} HOMEPAGE_CONTROLS;

// Make the structure global
extern HOMEPAGE_CONTROLS g_HomeUI;

HWND CreateHomePage(HWND parent);
BOOL Home_PageProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void ResizeHomePage(int width, int height);

#endif