#include <ui/settingspage.h>
#include <resource.h>
#include <globals.h>
#include <theme/theme.h>
#include <commctrl.h>
#include <DarkMode.h>
#include <stdio.h>
#include <utils/utils.h>
#include <wchar.h>
#include <strsafe.h>

// Initialize global struct
SETTINGS_CONTROLS g_SettingsUI = {0};



void Settings_PopulateColorSchemes(BOOL isDark) {
    if (!g_SettingsUI.hComboColorScheme) return;
    WCHAR themeName[128];
    SendMessageW(g_SettingsUI.hComboColorScheme, CB_RESETCONTENT, 0, 0);
    if (isDark) {
        for (int i = 0; i < g_numDarkThemes; ++i) {
            LoadStringW(main_hInstance, g_DarkThemes[i].nameID, themeName, _countof(themeName));
            SendMessageW(g_SettingsUI.hComboColorScheme, CB_ADDSTRING, 0, (LPARAM)themeName);
        }
    } else {
        for (int i = 0; i < g_numLightThemes; ++i) {
            LoadStringW(main_hInstance, g_LightThemes[i].nameID, themeName, _countof(themeName));
            SendMessageW(g_SettingsUI.hComboColorScheme, CB_ADDSTRING, 0, (LPARAM)themeName);
        }
    }
}
BOOL CALLBACK ApplySettingsFont(HWND hwnd, LPARAM font) {
    SendMessage(hwnd, WM_SETFONT, (WPARAM)font, TRUE);
    return TRUE;
}

HWND CreateSettingsPage(HWND hParent) {
    g_SettingsUI.hPage = hParent;
    HINSTANCE hInst = main_hInstance;
    
    wchar_t *s;

    // --- 1. General Settings Group ---
    s = LoadResString(hInst, IDS_SETTINGS_GROUP_GENERAL);
    g_SettingsUI.hGroupGeneral = CreateWindowW(L"BUTTON", s, WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0,0,0,0, hParent, NULL, hInst, NULL);
    memset(s,0,wcslen(s));

    s = LoadResString(hInst, IDS_SETTINGS_LABEL_EMAIL);
    g_SettingsUI.hLabelEmail = CreateWindowW(L"STATIC", s, WS_CHILD | WS_VISIBLE, 0,0,0,0, hParent, NULL, hInst, NULL);
    g_SettingsUI.hEditEmail = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 0,0,0,0, hParent, (HMENU)IDC_EDIT_TEST_EMAIL, hInst, NULL);
    
    s = LoadResString(hInst, IDS_SETTINGS_LABEL_TEST_EMAIL);
    SendMessageW(g_SettingsUI.hEditEmail, EM_SETCUEBANNER, FALSE, (LPARAM)s); 

    s = LoadResString(hInst, IDS_SETTINGS_LABEL_THREAD);
    g_SettingsUI.hLabelThread = CreateWindowW(L"BUTTON", s, WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0,0,0,0, hParent, NULL, hInst, NULL);
    g_SettingsUI.hSliderThread = CreateWindowW(TRACKBAR_CLASSW, L"", WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS, 0,0,0,0, hParent, (HMENU)IDC_SLIDER_THREADS, hInst, NULL);
    SendMessage(g_SettingsUI.hSliderThread, TBM_SETRANGE, TRUE, MAKELONG(1, 20)); 
    
    s = LoadResString(hInst, IDS_SETTINGS_THREAD_PREVIEW_FMT);
    size_t len = wcslen(s);
    if (len >= 2) {
        s[len - 2] = '\0';
    }
    g_SettingsUI.hLabelThreadPreview = CreateWindowW(L"STATIC", s, WS_CHILD | WS_VISIBLE, 0,0,0,0, hParent, (HMENU)IDC_LBL_THREAD_PREVIEW, hInst, NULL);


    s = LoadResString(hInst, IDS_SETTINGS_LABEL_RETRY);
    g_SettingsUI.hLabelRetry = CreateWindowW(L"BUTTON", s, WS_CHILD | WS_VISIBLE |BS_GROUPBOX, 0,0,0,0, hParent, NULL, hInst, NULL);
    g_SettingsUI.hSliderRetry = CreateWindowW(TRACKBAR_CLASSW, L"", WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS, 0,0,0,0, hParent, (HMENU)IDC_SLIDER_RETRIES, hInst, NULL);
    SendMessage(g_SettingsUI.hSliderRetry, TBM_SETRANGE, TRUE, MAKELONG(1, 5)); 
    
    s = LoadResString(hInst, IDS_SETTINGS_RETRY_PREVIEW_FMT);
    len = wcslen(s);
    if (len >= 2) {
        s[len - 2] = '\0';
    }
    g_SettingsUI.hLabelRetryPreview = CreateWindowW(L"STATIC", s, WS_CHILD | WS_VISIBLE, 0,0,0,0, hParent, (HMENU)IDC_LBL_RETRY_PREVIEW, hInst, NULL);

    s = LoadResString(hInst, IDS_SETTINGS_LABEL_SAVEFILE);
    g_SettingsUI.hLabelSaveFile = CreateWindowW(L"BUTTON", s, WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0,0,0,0, hParent, NULL, hInst, NULL);
    g_SettingsUI.hEditSaveFile = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"results.txt", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 0,0,0,0, hParent, (HMENU)IDC_EDIT_SAVE_FILE, hInst, NULL);

    // --- 2. Telegram Integration Group ---
    s = LoadResString(hInst, IDS_SETTINGS_GROUP_TELEGRAM);
    g_SettingsUI.hGroupTelegram = CreateWindowW(L"BUTTON", s, WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0,0,0,0, hParent, NULL, hInst, NULL);
    g_SettingsUI.hEditTelToken = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 0,0,0,0, hParent, (HMENU)IDC_EDIT_TELEGRAM_TOKEN, hInst, NULL);
    s = LoadResString(hInst, IDS_SETTINGS_TELEGRAM_TOKEN_CUE);
    SendMessageW(g_SettingsUI.hEditTelToken, EM_SETCUEBANNER, FALSE, (LPARAM)s); 
    

    g_SettingsUI.hEditTelID = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 0,0,0,0, hParent, (HMENU)IDC_EDIT_TELEGRAM_ID, hInst, NULL);
    s = LoadResString(hInst, IDS_SETTINGS_TELEGRAM_CHAT_CUE);
    SendMessageW(g_SettingsUI.hEditTelID, EM_SETCUEBANNER, FALSE, (LPARAM)s); 

    // --- 3. Appearance Group ---
    s = LoadResString(hInst, IDS_SETTINGS_GROUP_APPEARANCE);
    g_SettingsUI.hGroupAppearance = CreateWindowW(L"BUTTON", s, WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0,0,0,0, hParent, NULL, hInst, NULL);
    s = LoadResString(hInst, IDS_SETTINGS_THEME_MODE);
    g_SettingsUI.hLabelThemeMode = CreateWindowW(L"STATIC", s, WS_CHILD | WS_VISIBLE, 0,0,0,0, hParent, NULL, hInst, NULL);
    g_SettingsUI.hComboThemeMode = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 0,0,0,0, hParent, (HMENU)IDC_COMBO_THEME_MODE, hInst, NULL);
    
    s = LoadResString(hInst, IDS_THEME_MODE_DARK);
    SendMessageW(g_SettingsUI.hComboThemeMode, CB_ADDSTRING, 0, (LPARAM)s);
    s = LoadResString(hInst, IDS_THEME_MODE_LIGHT); 
    SendMessageW(g_SettingsUI.hComboThemeMode, CB_ADDSTRING, 0, (LPARAM)s);
    s = LoadResString(hInst, IDS_THEME_MODE_SYSTEM); 
    SendMessageW(g_SettingsUI.hComboThemeMode, CB_ADDSTRING, 0, (LPARAM)s);

    s = LoadResString(hInst, IDS_SETTINGS_COLOR_SCHEME); 
    g_SettingsUI.hLabelColorScheme = CreateWindowW(L"STATIC", s, WS_CHILD | WS_VISIBLE, 0,0,0,0, hParent, NULL, hInst, NULL);
    g_SettingsUI.hComboColorScheme = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 0,0,0,0, hParent, (HMENU)IDC_COMBO_COLOR_SCHEME, hInst, NULL);
    
    
    // --- 4. Action Buttons ---
    s = LoadResString(hInst, IDS_SETTINGS_BTN_TEST_TELEGRAM); 
    g_SettingsUI.hBtnTestTelegram = CreateWindowW(L"BUTTON", s, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0,0,0,0, hParent, (HMENU)IDC_BTN_TEST_TELEGRAM, hInst, NULL); 
    s = LoadResString(hInst, IDS_SETTINGS_BTN_SAVE); 
    g_SettingsUI.hBtnSaveSettings = CreateWindowW(L"BUTTON", s, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0,0,0,0, hParent, (HMENU)IDC_BTN_SAVE_SETTINGS, hInst, NULL); 
    
    BOOL isDark = (g_settings.themeMode == 0) || 
                  (g_settings.themeMode == 2 && DarkMode_isDarkModeReg());



    Settings_PopulateColorSchemes(isDark);
    SendMessage(g_SettingsUI.hComboThemeMode, CB_SETCURSEL, g_settings.themeMode, 0);
    SendMessage(g_SettingsUI.hComboColorScheme, CB_SETCURSEL, g_settings.themeIndex, 0);
    EnumChildWindows(hParent, (WNDENUMPROC)ApplySettingsFont, (LPARAM)g_hFontDefault);
    
    SetWindowTextW(g_SettingsUI.hEditEmail, g_settings.testEmail);
    SetWindowTextW(g_SettingsUI.hEditSaveFile, g_settings.saveFileName);

    // 2. Populate Sliders
    SendMessage(g_SettingsUI.hSliderThread, TBM_SETPOS, TRUE, g_settings.numThreads);
    SendMessage(g_SettingsUI.hSliderRetry, TBM_SETPOS, TRUE, g_settings.maxRetries);

    // 3. Update Slider Preview Labels
    WCHAR buf[128];
    // Re-use the format strings from resources or hardcode if preferred
    StringCchPrintfW(buf, _countof(buf), L"Number of threads: %d", g_settings.numThreads);
    SetWindowTextW(g_SettingsUI.hLabelThreadPreview, buf);

    StringCchPrintfW(buf, _countof(buf), L"Max retries: %d", g_settings.maxRetries);
    SetWindowTextW(g_SettingsUI.hLabelRetryPreview, buf);

    // 4. Populate Telegram Controls
    SetWindowTextW(g_SettingsUI.hEditTelToken, g_settings.telegramToken);
    SetWindowTextW(g_SettingsUI.hEditTelID, g_settings.telegramID);
    
    return hParent;
}

BOOL Settings_PageProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_HSCROLL) {
        HWND hSlider = (HWND)lParam;
        int pos = (int)SendMessage(hSlider, TBM_GETPOS, 0, 0);
        WCHAR buf[64];

        if (hSlider == g_SettingsUI.hSliderThread) {
            StringCchPrintfW(buf, 64, L"Number of threads: %d", pos);
            SetWindowTextW(g_SettingsUI.hLabelThreadPreview, buf);
        } else if (hSlider == g_SettingsUI.hSliderRetry) {
            StringCchPrintfW(buf, 64, L"Max retries: %d", pos);
            SetWindowTextW(g_SettingsUI.hLabelRetryPreview, buf);
        }
        return TRUE;
    }
    return FALSE;
}

void ResizeSettingsPage(int width, int height) {
    if (!g_SettingsUI.hPage) return;
    int margin = 20, col1X = margin + 20, curY = 15;
    int groupW = width - (margin * 2), editW = groupW - 60;

    // Position General Group
    SetWindowPos(g_SettingsUI.hGroupGeneral, NULL, margin, curY, groupW, 250, SWP_NOZORDER);
    SetWindowPos(g_SettingsUI.hLabelEmail, NULL, col1X, curY + 25, 200, 20, SWP_NOZORDER);
    SetWindowPos(g_SettingsUI.hEditEmail, NULL, col1X, curY + 45, editW, 25, SWP_NOZORDER);
    
    SetWindowPos(g_SettingsUI.hLabelThread, NULL, col1X - 10, curY + 90, 200 + 20, 90, SWP_NOZORDER);
    SetWindowPos(g_SettingsUI.hLabelRetry, NULL, col1X + 240 - 10, curY + 90, 200 + 20, 90, SWP_NOZORDER);
    SetWindowPos(g_SettingsUI.hSliderThread, NULL, col1X, curY + 110, 200, 30, SWP_NOZORDER);
    SetWindowPos(g_SettingsUI.hLabelThreadPreview, NULL, col1X, curY + 145, 200, 20, SWP_NOZORDER);
    SetWindowPos(g_SettingsUI.hSliderRetry, NULL, col1X + 240, curY + 110, 200, 30, SWP_NOZORDER);
    SetWindowPos(g_SettingsUI.hLabelRetryPreview, NULL, col1X + 240, curY + 145, 200, 20, SWP_NOZORDER);
    
    SetWindowPos(g_SettingsUI.hLabelSaveFile, NULL, col1X - 10, curY + 186, 170, 56, SWP_NOZORDER);
    SetWindowPos(g_SettingsUI.hEditSaveFile, NULL, col1X, curY + 206, 150, 25, SWP_NOZORDER);



    
    curY += 270;

    // Position Telegram Group
    SetWindowPos(g_SettingsUI.hGroupTelegram, NULL, margin, curY, groupW, 120, SWP_NOZORDER);
    SetWindowPos(g_SettingsUI.hEditTelToken, NULL, col1X, curY + 35, editW, 25, SWP_NOZORDER);
    SetWindowPos(g_SettingsUI.hEditTelID, NULL, col1X, curY + 75, editW, 25, SWP_NOZORDER);

    curY += 140;

    // Position Appearance Group
    SetWindowPos(g_SettingsUI.hGroupAppearance, NULL, margin, curY, groupW, 110, SWP_NOZORDER);
    SetWindowPos(g_SettingsUI.hLabelThemeMode, NULL, col1X, curY + 32, 100, 20, SWP_NOZORDER);
    SetWindowPos(g_SettingsUI.hComboThemeMode, NULL, col1X + 110, curY + 30, 250, 25, SWP_NOZORDER);
    SetWindowPos(g_SettingsUI.hLabelColorScheme, NULL, col1X, curY + 67, 100, 20, SWP_NOZORDER);
    SetWindowPos(g_SettingsUI.hComboColorScheme, NULL, col1X + 110, curY + 65, 250, 25, SWP_NOZORDER);

    // Action Buttons
    SetWindowPos(g_SettingsUI.hBtnTestTelegram, NULL, margin, height - 60, 150, 35, SWP_NOZORDER);
    SetWindowPos(g_SettingsUI.hBtnSaveSettings, NULL, width - 170, height - 60, 150, 35, SWP_NOZORDER);
}