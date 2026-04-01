#include <windows.h>
#include <globals.h>
#include <sidebar/sidebar.h>
#include <ui/sidebar_main.h>
#include <ui/pages.h> // Include the new pages header

void HANDLE_WMSIZE(WPARAM wParam, LPARAM lParam){
    RECT rcClient;
    GetClientRect(main_hwnd, &rcClient);

    BOOL isCollapsed = (BOOL)SendMessageW(hwnd_sidebar, SB_ISCOLLAPSED, 0, 0);
    int sidebarWidth = isCollapsed ? SIDEBAR_NARROW : SIDEBAR_WIDE;

    SetWindowPos(hwnd_sidebar, NULL, 0, 0, sidebarWidth, rcClient.bottom, SWP_NOZORDER);

    // Also resize the pages to fit the remaining area.
    ResizePages(sidebarWidth, rcClient.right, rcClient.bottom);
}