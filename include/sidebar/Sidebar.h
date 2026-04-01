#ifndef SIDEBAR_H
#define SIDEBAR_H

#include <windows.h>
#include <commctrl.h> // For NMHDR

#ifdef __cplusplus
extern "C" {
#endif

// Public class name (CreateWindowExW with this after SidebarRegister)
#define SIDEBAR_CLASS_NAME L"Win32_SidebarControl"
#define SIDEBAR_WIDE  150
#define SIDEBAR_NARROW 63

// Custom messages
#define SB_ADDITEM         (WM_USER + 100) // lParam: PSIDEBAR_ITEM
#define SB_GETCURSEL       (WM_USER + 101)
#define SB_SETCURSEL       (WM_USER + 102)
#define SB_RESETCONTENT    (WM_USER + 103)
#define SB_SETCOLORS       (WM_USER + 104) // lParam: PSIDEBAR_COLORS*
#define SB_TOGGLECOLLAPSE  (WM_USER + 105)
#define SB_ISCOLLAPSED     (WM_USER + 106)
#define SB_SETFOOTER       (WM_USER + 107) // lParam: PSIDEBAR_FOOTER_INFO*

// Data structures
typedef struct { wchar_t* pszText; LPARAM lParam; HICON hIcon; } SIDEBAR_ITEM, *PSIDEBAR_ITEM;
typedef struct { wchar_t* pszText; HICON hIcon; } SIDEBAR_FOOTER_INFO, *PSIDEBAR_FOOTER_INFO;
typedef struct { 
    COLORREF clrBackgroundTop, clrBackgroundBottom, clrSeparator,
    clrItemText, clrItemTextHot, clrItemTextSel, 
    clrItemBackgroundHot, clrItemBackgroundSel, 
    clrIndicator; 
} SIDEBAR_COLORS, *PSIDEBAR_COLORS;

// Notifications
#define SBN_SELCHANGED     (NM_FIRST - 1)
#define SBN_TOGGLECLICK    (NM_FIRST - 2)

typedef struct { NMHDR hdr; int iNewSel; LPARAM lParam; } NMSIDEBAR, *LPNMSIDEBAR;
typedef struct { NMHDR hdr; BOOL isCollapsed; } NMSBTOGGLE, *LPNMSBTOGGLE;

// Public API
BOOL SidebarRegister(HINSTANCE hInstance);

#ifdef __cplusplus
}
#endif

#endif // SIDEBAR_H