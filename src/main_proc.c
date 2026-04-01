#include <globals.h>
#include <ui/sidebar_main.h>
#include <DarkMode.h>
#include <theme/theme.h>
#include <core/settings.h>
#include <core/size_msg.h>
#include <core/notify_msg.h>
#include <core/command_msg.h>
#include <resource.h>
#include <ui/pages.h>
#include <core/checker.h>
#include <commctrl.h>
#include <ui/homepage.h>
#include <utils/utils.h>
#include <strsafe.h>
#include <richedit.h> // Important for coloring

// Counters
static int s_ValidCount = 0;
static int s_FailedCount = 0;
static int s_DeliveryCount = 0;
#define WM_WORKER_RESET (WM_USER + 10)

// --- Helper: Append Colored Text and Force Scroll ---
void AppendTextToConsole(HWND hEdit, const wchar_t* text, COLORREF color) {

    int len = GetWindowTextLengthW(hEdit);
    SendMessageW(hEdit, EM_SETSEL, len, len);

    CHARFORMAT2W cf = {0};
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR;
    cf.crTextColor = color;
    cf.dwEffects = 0; 
    SendMessageW(hEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

    // 4. Append the text
    SendMessageW(hEdit, EM_REPLACESEL, 0, (LPARAM)text);

    SendMessageW(hEdit, WM_VSCROLL, SB_BOTTOM, 0);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        main_hwnd = hwnd;
        LoadSettings();
        CreateSidebar();
        CreatePages(hwnd);
        RECT rc;
        GetClientRect(hwnd, &rc);
        SendMessage(hwnd, WM_SIZE, 0, MAKELPARAM(rc.right, rc.bottom));
        ShowPage(IDC_SIDEBAR_HOME);
        BOOL isDark = (g_settings.themeMode == 0) || (g_settings.themeMode == 2 && DarkMode_isDarkModeReg());
        ApplyTheme(hwnd, g_settings.themeIndex, isDark);
        break;

    case WM_SIZE:
        HANDLE_WMSIZE(wParam,lParam);
        break;

    case WM_NOTIFY:
        HANDLE_WMNOTIFY(wParam,lParam);
        break;

    case WM_COMMAND:
        HANDLE_WMCOMMAND(wParam,lParam);
        break;

    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;

    case WM_WORKER_RESET: {
        s_ValidCount = 0;
        s_FailedCount = 0;
        s_DeliveryCount = 0;
        
        SetWindowTextW(g_HomeUI.hLblValid, L"Valid SMTPs: 0");
        SetWindowTextW(g_HomeUI.hLblFailed, L"Failed: 0");
        SetWindowTextW(g_HomeUI.hLblDelivery, L"Delivery Failed: 0");
        
        ListView_DeleteAllItems(g_HomeUI.hListMain);
        SetWindowTextW(g_HomeUI.hConsole, L"");
        break;
    }

    case WM_WORKER_LOG: {
        wchar_t* msg = (wchar_t*)lParam;
        if (msg) {
            COLORREF textColor = RGB(200, 200, 200); // Default: Light Gray

            // Color Logic matching Python output
            if (wcsstr(msg, L"✅")) {
                textColor = RGB(0, 255, 0);   // Green for Success
            } else if (wcsstr(msg, L"❌")) {
                textColor = RGB(255, 80, 80); // Red for Errors
            } else if (wcsstr(msg, L"🔍")) {
                textColor = RGB(100, 200, 255); // Cyan for Testing
            } else if (wcsstr(msg, L"⚠️")) {
                textColor = RGB(255, 200, 0);   // Yellow for Warnings
            } else if (wcsstr(msg, L"🚀") || wcsstr(msg, L"🛑") || wcsstr(msg, L"📁")) {
                textColor = RGB(255, 0, 255);   // Magenta for System
            }

            AppendTextToConsole(g_HomeUI.hConsole, msg, textColor);
            free(msg);
        }
        break;
    }

    case WM_WORKER_RESULT: {
        RESULT_DATA* res = (RESULT_DATA*)lParam;
        if (res) {
            BOOL isValid = (strstr(res->status, "Valid") != NULL);
            
            if (isValid) s_ValidCount++; else s_FailedCount++;
            
            WCHAR buf[64];
            StringCchPrintfW(buf, 64, L"Valid SMTPs: %d", s_ValidCount);
            SetWindowTextW(g_HomeUI.hLblValid, buf);
            
            StringCchPrintfW(buf, 64, L"Failed: %d", s_FailedCount);
            SetWindowTextW(g_HomeUI.hLblFailed, buf);
            
            if (isValid) {
                LVITEMW lvi = {0};
                lvi.mask = LVIF_TEXT;
                lvi.iItem = 0; lvi.iSubItem = 0;

                wchar_t* wHost = utf8_to_wchar(res->host);
                lvi.pszText = wHost ? wHost : L"";
                int row = (int)SendMessageW(g_HomeUI.hListMain, LVM_INSERTITEMW, 0, (LPARAM)&lvi);
                if (wHost) free(wHost);

                wchar_t* wEmail = utf8_to_wchar(res->email);
                if (wEmail) { ListView_SetItemText(g_HomeUI.hListMain, row, 1, wEmail); free(wEmail); }

                wchar_t* wPort = utf8_to_wchar(res->port);
                if (wPort) { ListView_SetItemText(g_HomeUI.hListMain, row, 2, wPort); free(wPort); }

                wchar_t* wPass = utf8_to_wchar(res->pass);
                if (wPass) { ListView_SetItemText(g_HomeUI.hListMain, row, 3, wPass); free(wPass); }

                wchar_t* wStatus = utf8_to_wchar(res->status);
                if (wStatus) { ListView_SetItemText(g_HomeUI.hListMain, row, 4, wStatus); free(wStatus); }

                wchar_t* wDel = utf8_to_wchar(res->delivery);
                if (wDel) { ListView_SetItemText(g_HomeUI.hListMain, row, 5, wDel); free(wDel); }
            }
            free(res); 
        }
        break;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}
