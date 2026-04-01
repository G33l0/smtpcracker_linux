#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <resource.h>
#include <globals.h>
#include <sidebar/sidebar.h>
#include <ui/pages.h>
#include <core/size_msg.h>
#include <stdio.h>

void HANDLE_WMNOTIFY(WPARAM wParam, LPARAM lParam) {
    LPNMHDR nmhdr = (LPNMHDR)lParam;
    if (!nmhdr) return;

    // --- Handle Sidebar Notifications ---
    if (nmhdr->idFrom == IDC_SIDEBAR) {
        switch (nmhdr->code) {
            case SBN_SELCHANGED: {
                
                LPNMSIDEBAR info = (LPNMSIDEBAR)lParam;
                ShowPage(info->lParam);
                break;
            }
            case SBN_TOGGLECLICK: {
                PostMessage(main_hwnd, WM_SIZE, 0, 0);
                break;
            }
        }
        return;
    }
}