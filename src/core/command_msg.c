#include <windows.h>
#include <commctrl.h>
#include <globals.h>
#include <sidebar/sidebar.h>
#include <ui/sidebar_main.h>
#include <resource.h>
#include <ui/settingspage.h>
#include <theme/theme.h>
#include <dwmapi.h>
#include <core/command_msg.h>
#include <core/settings.h>
#include <ui/settingspage.h>
#include <core/checker.h>

void HANDLE_WMCOMMAND(WPARAM wParam, LPARAM lParam) {
    WORD controlId = LOWORD(wParam);
    WORD notificationCode = HIWORD(wParam);
    HWND hControl = (HWND)lParam;

    // --- Handle Settings Page Commands ---
    if (notificationCode == CBN_SELCHANGE) {
        int sel = SendMessage(hControl, CB_GETCURSEL, 0, 0);
        if (sel == CB_ERR) return;

        switch (controlId) {
            case IDC_COMBO_THEME_MODE: {
                g_settings.themeMode = sel;
                BOOL isDark = (sel == 0) || (sel == 2 && DarkMode_isDarkModeReg());
                Settings_PopulateColorSchemes(isDark);
                g_settings.themeIndex = 0;
                SendMessage(g_SettingsUI.hComboColorScheme, CB_SETCURSEL, 0, 0);
                ApplyTheme(main_hwnd, g_settings.themeIndex, isDark);
                SaveSettings();
                break;
            }
            case IDC_COMBO_COLOR_SCHEME: {
                g_settings.themeIndex = sel;
                BOOL isDark = (g_settings.themeMode == 0) || (g_settings.themeMode == 2 && DarkMode_isDarkModeReg());
                ApplyTheme(main_hwnd, g_settings.themeIndex, isDark);
                SaveSettings();
                break;
            }
        }
    }
    switch(controlId) {
        case IDC_BTN_SAVE_SETTINGS: {
                // 1. Get Text from Edits
            GetWindowTextW(g_SettingsUI.hEditEmail, g_settings.testEmail, 256);
            GetWindowTextW(g_SettingsUI.hEditSaveFile, g_settings.saveFileName, MAX_PATH);
            GetWindowTextW(g_SettingsUI.hEditTelToken, g_settings.telegramToken, 256);
            GetWindowTextW(g_SettingsUI.hEditTelID, g_settings.telegramID, 128);

            // 2. Get Slider Positions
            g_settings.numThreads = (int)SendMessage(g_SettingsUI.hSliderThread, TBM_GETPOS, 0, 0);
            g_settings.maxRetries = (int)SendMessage(g_SettingsUI.hSliderRetry, TBM_GETPOS, 0, 0);

            // 3. Commit to Registry
            if (SaveSettings() == 0) {
                MessageBoxW(main_hwnd, L"Settings saved successfully!", L"Success", MB_OK | MB_ICONINFORMATION);
            }
            break;
        }
        case IDC_BTN_UPLOAD: {
            HandleUploadCombo(main_hwnd);
            break;
        }
        case IDC_BTN_START:{
            StartCheckingProcess();
            break;
        }
    }

}