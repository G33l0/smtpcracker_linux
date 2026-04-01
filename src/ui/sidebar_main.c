#include <windows.h>
#include <sidebar/sidebar.h>
#include <globals.h>
#include <ui/sidebar_main.h>
#include <resource.h>
#include <utils/utils.h>


HWND hwnd_sidebar;


SIDEBAR_COLORS DeriveSidebarColors(DarkMode_Colors* themeColors) {
    SIDEBAR_COLORS sb;
    sb.clrBackgroundTop = themeColors->background;
    sb.clrBackgroundBottom = AdjustColor(themeColors->background, -10); // Slightly darker for gradient
    sb.clrSeparator = AdjustColor(themeColors->background, 25); // Slightly lighter for separator line
    sb.clrItemText = themeColors->text;
    sb.clrItemTextHot = themeColors->linkText;
    sb.clrItemTextSel = themeColors->darkerText; 
    sb.clrItemBackgroundHot = themeColors->hotBackground;
    sb.clrItemBackgroundSel = themeColors->ctrlBackground; 
    sb.clrIndicator = themeColors->hotEdge; // Use hot edge for the active indicator
    return sb;
}

void ChangeSidebarTheme(DarkMode_Colors themeColors) {
    SIDEBAR_COLORS colors = DeriveSidebarColors(&themeColors);
    SendMessageW(hwnd_sidebar, SB_SETCOLORS, 0, (LPARAM)&colors);
}

void CreateSidebar() {
    hwnd_sidebar = CreateWindowW(SIDEBAR_CLASS_NAME,L"", WS_VISIBLE | WS_CHILD,
                0,
                0,
                SIDEBAR_WIDE,
                570,
                main_hwnd,(HMENU) IDC_SIDEBAR,main_hInstance,NULL);

    WCHAR szItemText[128];

    SIDEBAR_ITEM item;

    LoadStringW(main_hInstance, IDS_SIDEBAR_HOME_TEXT, szItemText, _countof(szItemText));
    item.pszText = szItemText;
        item.lParam = IDC_SIDEBAR_HOME;
        item.hIcon = LoadAndRecolorIcon(main_hInstance, IDI_ICON_HOME, RGB(111, 66, 193));
        SendMessageW(hwnd_sidebar, SB_ADDITEM, 0, (LPARAM)&item);
    
    LoadStringW(main_hInstance, IDS_SIDEBAR_SETTINGS_TEXT, szItemText, _countof(szItemText));
    item.pszText = szItemText;
        item.lParam = IDC_SIDEBAR_SETTING;
        item.hIcon = LoadAndRecolorIcon(main_hInstance, IDI_ICON_SETTING, RGB(111, 66, 193));
        SendMessageW(hwnd_sidebar, SB_ADDITEM, 0, (LPARAM)&item);

    LoadStringW(main_hInstance, IDS_SIDEBAR_ABOUT_TEXT, szItemText, _countof(szItemText));
    item.pszText = szItemText;
        item.lParam = IDC_SIDEBAR_ABOUT;
        item.hIcon = LoadAndRecolorIcon(main_hInstance, IDI_ICON_ABOUT, RGB(111, 66, 193));
        SendMessageW(hwnd_sidebar, SB_ADDITEM, 0, (LPARAM)&item);


    WCHAR szFooterText[128];
    SIDEBAR_FOOTER_INFO footer;
    LoadStringW(main_hInstance, IDS_SIDEBAR_FOOTER_TEXT, szFooterText, _countof(szFooterText));
    footer.pszText = szFooterText;
    footer.hIcon = (HICON)LoadImageW(main_hInstance, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON, 32, 32, LR_SHARED);
    SendMessageW(hwnd_sidebar, SB_SETFOOTER, 0, (LPARAM)&footer);

    SendMessageW(hwnd_sidebar, SB_SETCURSEL, 0, 0);
}