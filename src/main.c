#include <globals.h>
#include <resource.h>
#include <utils/err.h>
#include <utils/utils.h>
#include <stdio.h>
#include <sidebar/sidebar.h>
#include <DarkMode.h>

HINSTANCE main_hInstance = NULL;
HWND main_hwnd = NULL;
HFONT g_hFontDefault = NULL;

// Initialize all AppSettings with default values
AppSettings g_settings = {
    .themeMode = 2, // System
    .themeIndex = 21,
    .maxRetries = 3,
    .numThreads = 10,
    .testEmail = L"",
    .saveFileName = L"results.txt",
    .telegramToken = L"",
    .telegramID = L""
};
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    main_hInstance = hInstance;
    
    g_hFontDefault = CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, 
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Tahoma");

    if (!DarkMode_LoadLibrary(L"darkmode.dll")) {
        ERR(-1,"Unable to load darkmode.dll");
        ERR_print_errors();
        MessageBoxW(NULL, L"Unable to load darkmode.dll", L"Error", MB_ICONERROR);
        return 1;
    }

    DarkMode_initDarkMode_NoIni();

    wchar_t* g_szClassName = LoadResString(hInstance,IDS_MAIN_CLASS_NAME);
    if (!g_szClassName){
        ERR_print_errors();
        return 1;
    }

    WNDCLASSW wc = {0};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = g_szClassName;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_APP_ICON));


    if (!RegisterClassW(&wc))
    {
        ERR(-1, "Window Registeration Failed!");
        ERR_print_errors();
        MessageBoxW(NULL, L"Window Registration Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    SidebarRegister(hInstance);
    HWND hwnd = CreateWindowExW(
        0,
        g_szClassName,
        L"SMTP Cracker",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 660,
        NULL, NULL, hInstance, NULL);

    if (hwnd == NULL)
    {
        ERR(-1, "Window Creation Failed!");
        ERR_print_errors();
        MessageBoxW(NULL, L"Window Creation Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG Msg;
    while (GetMessage(&Msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    if (g_hFontDefault) DeleteObject(g_hFontDefault);
    DarkMode_FreeLibrary();
    return Msg.wParam;
}