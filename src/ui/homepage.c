#include <ui/homepage.h>
#include <windows.h>
#include <commctrl.h>
#include <globals.h>
#include <resource.h>
#include <utils/utils.h> 
#include <stdio.h>
#include <stdlib.h>
#include <richedit.h>
// Initialize the global structure
HOMEPAGE_CONTROLS g_HomeUI = {0};



// Helper to set font
static BOOL CALLBACK ApplyHomeFont(HWND hwnd, LPARAM font) {
    SendMessage(hwnd, WM_SETFONT, (WPARAM)font, TRUE);
    return TRUE;
}

// Helper to add column
static void AddListColumn(HWND hList, int index, const wchar_t* text, int width) {
    LVCOLUMNW lvc = {0};
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvc.fmt = LVCFMT_LEFT;
    lvc.cx = width;
    lvc.pszText = (wchar_t*)text;
    lvc.iSubItem = index;
    ListView_InsertColumn(hList, index, &lvc);
}

HWND CreateHomePage(HWND parent) {
    g_HomeUI.hPage = parent;
    HINSTANCE hInst = GetModuleHandle(NULL);
    wchar_t* s = NULL;

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);

    // --- 1. Top Buttons ---
    s = LoadResString(hInst, IDS_HOME_BTN_UPLOAD);
    g_HomeUI.hBtnUpload = CreateWindowW(L"BUTTON", s ? s : L"Upload Combo", 
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT, 
        0, 0, 0, 0, parent, (HMENU)IDC_BTN_UPLOAD, hInst, NULL);
    if(s) free(s);

    s = LoadResString(hInst, IDS_HOME_BTN_START);
    g_HomeUI.hBtnStart = CreateWindowW(L"BUTTON", s ? s : L"Start Checking", 
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT, 
        0, 0, 0, 0, parent, (HMENU)IDC_BTN_START, hInst, NULL);
    if(s) free(s);

    // --- 2. Middle Status Labels ---
    s = LoadResString(hInst, IDS_HOME_LBL_COMBOS);
    g_HomeUI.hLblCombos = CreateWindowW(L"STATIC", s ? s : L"Combos Loaded: 0", 
        WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE | SS_LEFT, 
        0, 0, 0, 0, parent, (HMENU)IDC_LBL_COMBOS, hInst, NULL);
    if(s) free(s);

    s = LoadResString(hInst, IDS_HOME_LBL_VALID);
    g_HomeUI.hLblValid = CreateWindowW(L"STATIC", s ? s : L"Valid SMTPs: 0", 
        WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE | SS_LEFT, 
        0, 0, 0, 0, parent, (HMENU)IDC_LBL_VALID, hInst, NULL);
    if(s) free(s);

    s = LoadResString(hInst, IDS_HOME_LBL_FAILED);
    g_HomeUI.hLblFailed = CreateWindowW(L"STATIC", s ? s : L"Failed: 0", 
        WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE | SS_LEFT, 
        0, 0, 0, 0, parent, (HMENU)IDC_LBL_FAILED, hInst, NULL);
    if(s) free(s);

    s = LoadResString(hInst, IDS_HOME_LBL_DELIVERY);
    g_HomeUI.hLblDelivery = CreateWindowW(L"STATIC", s ? s : L"Delivery Failed: 0", 
        WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE | SS_LEFT, 
        0, 0, 0, 0, parent, (HMENU)IDC_LBL_DELIVERY, hInst, NULL);
    if(s) free(s);

    // --- 3. Main List View ---
    g_HomeUI.hListMain = CreateWindowW(WC_LISTVIEWW, L"", 
        WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL, 
        0, 0, 0, 0, parent, (HMENU)IDC_LISTVIEW_MAIN, hInst, NULL);

    ListView_SetExtendedListViewStyle(g_HomeUI.hListMain, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    // Add Columns
    s = LoadResString(hInst, IDS_HOME_COL_HOST);
    AddListColumn(g_HomeUI.hListMain, 0, s ? s : L"SMTP Host", 180);
    if(s) free(s);

    s = LoadResString(hInst, IDS_HOME_COL_EMAIL);
    AddListColumn(g_HomeUI.hListMain, 1, s ? s : L"Email", 220);
    if(s) free(s);

    s = LoadResString(hInst, IDS_HOME_COL_PORT);
    AddListColumn(g_HomeUI.hListMain, 2, s ? s : L"Port", 60);
    if(s) free(s);

    s = LoadResString(hInst, IDS_HOME_COL_PASS);
    AddListColumn(g_HomeUI.hListMain, 3, s ? s : L"Password", 150);
    if(s) free(s);

    s = LoadResString(hInst, IDS_HOME_COL_STATUS);
    AddListColumn(g_HomeUI.hListMain, 4, s ? s : L"Status", 100);
    if(s) free(s);

    s = LoadResString(hInst, IDS_HOME_COL_DELIVERY);
    AddListColumn(g_HomeUI.hListMain, 5, s ? s : L"Delivery", 100);
    if(s) free(s);

    // --- 4. Bottom Console ---
    s = LoadResString(hInst, IDS_HOME_CONSOLE_INIT);
// --- 4. Bottom Console (Rich Edit) ---
    // Load the RichEdit library (Required before creating the control)
    LoadLibraryW(L"Msftedit.dll"); 

    s = LoadResString(hInst, IDS_HOME_CONSOLE_INIT);
    g_HomeUI.hConsole = CreateWindowExW(0, MSFTEDIT_CLASS, 
        s ? s : L" \r\n SMTP Cracker Dashboard Ready!\r\n Upload a combo file to get started...", 
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_TABSTOP, 
        0, 0, 0, 0, parent, (HMENU)IDC_CONSOLE_LOG, hInst, NULL);
    
    // Set background color to black (Optional, for hacker look)
    SendMessage(g_HomeUI.hConsole, EM_SETBKGNDCOLOR, 0, (LPARAM)RGB(30, 30, 30));
    
    // Set default text color to white
    CHARFORMAT2W cf = {0};
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR;
    cf.crTextColor = RGB(200, 200, 200);
    SendMessage(g_HomeUI.hConsole, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
    if(s) free(s);

    EnumChildWindows(parent, (WNDENUMPROC)ApplyHomeFont, (LPARAM)g_hFontDefault);

    return parent;
}

BOOL Home_PageProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    return FALSE;
}

void ResizeHomePage(int width, int height) {
    if (!g_HomeUI.hPage) return;

    // --- Layout Configuration ---
    int padding = 10;
    int topBarHeight = 50;
    int statsBarHeight = 30;
    int consoleHeight = 150;

    // --- 1. Top Buttons Layout ---
    int btnWidth = 160;
    int btnHeight = 35;
    int btnY = (topBarHeight - btnHeight) / 2;
    int totalBtnWidth = (btnWidth * 2) + 20; 
    int startBtnX = (width - totalBtnWidth) / 2;

    SetWindowPos(g_HomeUI.hBtnUpload, NULL, startBtnX, btnY + padding, btnWidth, btnHeight, SWP_NOZORDER);
    SetWindowPos(g_HomeUI.hBtnStart, NULL, startBtnX + btnWidth + 20, btnY + padding, btnWidth, btnHeight, SWP_NOZORDER);

    // --- 2. Bottom Console Layout ---
    int consoleY = height - consoleHeight - padding;
    SetWindowPos(g_HomeUI.hConsole, NULL, 
        padding, consoleY, 
        width - (2 * padding), consoleHeight, 
        SWP_NOZORDER);

    // --- 3. Middle Status Bar Layout ---
    int statsY = consoleY - statsBarHeight - 5; 
    int lblWidth = (width - (2 * padding)) / 4; 
    int curLblX = padding;

    SetWindowPos(g_HomeUI.hLblCombos, NULL, curLblX, statsY, lblWidth, statsBarHeight, SWP_NOZORDER);
    curLblX += lblWidth;
    
    SetWindowPos(g_HomeUI.hLblValid, NULL, curLblX, statsY, lblWidth, statsBarHeight, SWP_NOZORDER);
    curLblX += lblWidth;

    SetWindowPos(g_HomeUI.hLblFailed, NULL, curLblX, statsY, lblWidth, statsBarHeight, SWP_NOZORDER);
    curLblX += lblWidth;

    SetWindowPos(g_HomeUI.hLblDelivery, NULL, curLblX, statsY, lblWidth, statsBarHeight, SWP_NOZORDER);

    // --- 4. Main List View Layout ---
    int listY = topBarHeight + (padding * 2);
    int listHeight = statsY - listY - 5; 

    if (listHeight < 100) listHeight = 100;

    SetWindowPos(g_HomeUI.hListMain, NULL, 
        padding, listY, 
        width - (2 * padding), listHeight, 
        SWP_NOZORDER);
}