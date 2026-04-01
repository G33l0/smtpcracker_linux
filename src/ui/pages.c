#include <ui/pages.h>
#include <resource.h>
#include <globals.h>
#include <ui/homepage.h>
#include <ui/aboutpage.h>
#include <ui/settingspage.h>


#define NUM_PAGES 3
static HWND g_hPages[NUM_PAGES];

enum {
    PAGE_HOME = 0,
    PAGE_SETTING,
    PAGE_ABOUT
};

static int g_pageIds[NUM_PAGES] = {
    IDC_SIDEBAR_HOME,
    IDC_SIDEBAR_SETTING,
    IDC_SIDEBAR_ABOUT
};


// Store the original window procedure for the STATIC control
static WNDPROC g_pfnOrigStaticProc = NULL;

/**
 * @brief The new window procedure for our page container windows.
 *
 * This procedure intercepts messages sent by child controls and forwards them
 * up to the main application window.
 */
LRESULT CALLBACK PageContainerProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_COMMAND:
            // Forward command messages (like button clicks) to the main window.
            return SendMessage(main_hwnd, WM_COMMAND, wParam, lParam);
        case WM_NOTIFY:
            // Forward notification messages (like link clicks) to the main window.
            return SendMessage(main_hwnd, WM_NOTIFY, wParam, lParam);

        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLORLISTBOX:
            return SendMessage(main_hwnd, uMsg, wParam, lParam);
    }

    // For any other message, call the original default procedure.
    return CallWindowProc(g_pfnOrigStaticProc, hwnd, uMsg, wParam, lParam);
}


void CreatePages(HWND hParent) {
    for (int i = 0; i < NUM_PAGES; ++i) {
        // These are the container windows for each page
        g_hPages[i] = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_BORDER,
            0, 0, 0, 0, hParent, (HMENU)(UINT_PTR)g_pageIds[i], main_hInstance, NULL);

        // The first time, get and store the original STATIC window procedure.
        if (g_pfnOrigStaticProc == NULL) {
            g_pfnOrigStaticProc = (WNDPROC)GetWindowLongPtr(g_hPages[i], GWLP_WNDPROC);
        }
        // Replace the default window procedure with our custom one.
        SetWindowLongPtr(g_hPages[i], GWLP_WNDPROC, (LONG_PTR)PageContainerProc);
    }

    // Create the content for each page inside its container
    CreateHomePage(g_hPages[PAGE_HOME]);
    CreateSettingsPage(g_hPages[PAGE_SETTING]);
    CreateAboutPage(g_hPages[PAGE_ABOUT]);
}

void ShowPage(int pageId) {
    for (int i = 0; i < NUM_PAGES; ++i) {
        ShowWindow(g_hPages[i], (g_pageIds[i] == pageId) ? SW_SHOW : SW_HIDE);
    }
}

void ResizePages(int sidebarWidth, int mainWidth, int mainHeight) {
    int x = sidebarWidth;
    int y = 0;
    int width = mainWidth - sidebarWidth;
    int height = mainHeight;

    for (int i = 0; i < NUM_PAGES; ++i) {
        SetWindowPos(g_hPages[i], NULL, x, y, width, height, SWP_NOZORDER);
    }

    // Now that each page container is resized, resize its contents.
    ResizeHomePage(width, height);
    ResizeSettingsPage(width, height);
    ResizeAboutPage(width, height);
    
}