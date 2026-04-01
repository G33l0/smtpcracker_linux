#include <sidebar/sidebar.h>
#include <windows.h>
#include <windowsx.h>
#include <stdlib.h>
#include <wchar.h>
#include <math.h>
#include <commctrl.h>

#pragma comment(lib, "Msimg32.lib") // for AlphaBlend
#pragma comment(lib, "Comctl32.lib")

#define IDT_ANIMATION 1
#define PI 3.14159265358979323846
#define ANIMATION_INTERVAL 15
#define SLIDE_DURATION 200.0f
#define GLOW_DURATION 250.0f
#define JIGGLE_DURATION 300.0f
#define COLLAPSE_DURATION 240.0f

#define powf(x, y)  (float)pow(x, y)
#define sinf(x)     (float)sin(x)

typedef struct {
    wchar_t* pszText;
    LPARAM   lParam;
    HICON    hIcon;
} _SIDEBAR_ITEM;

typedef struct {
    _SIDEBAR_ITEM*  items;
    int             itemCount;
    int             itemCapacity;
    int             iSel;
    int             iHot;
    int             itemHeight;
    HFONT           hFont;
    SIDEBAR_COLORS  colors;
    BOOL            bIsCollapsed;
    wchar_t*        pszFooterText;
    HICON           hFooterIcon;
    BOOL            bOwnsFont;

    // Animation / timing state
    DWORD           dwHotStartTime;
    int             nSlideStartY;
    int             nSlideTargetY;

    // collapse animation
    int             nExpandedWidth;
    int             nCollapsedWidth;
    int             nCurrentWidth;
    int             nTargetWidth;
    DWORD           dwCollapseStartTime;

    // tooltip
    HWND            hwndToolTip;

} SIDEBAR_DATA, *PSIDEBAR_DATA;

// Forward
static LRESULT CALLBACK _Sidebar_WndProc(HWND, UINT, WPARAM, LPARAM);
static void _Sidebar_OnPaint(HWND);
static void _Sidebar_SetDefaults(PSIDEBAR_DATA);
static int  _Sidebar_AddItem(PSIDEBAR_DATA, PSIDEBAR_ITEM);
static int  GetClientRectWidth(HWND hWnd);
static int  GetClientRectHeight(HWND hWnd);

static HDC _CreateARGBBuffer(HDC hdcSrc, int w, int h, HBITMAP *outBitmap) {
    HDC hdc = CreateCompatibleDC(hdcSrc);
    if (!hdc) return NULL;
    BITMAPINFO bmi;
    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    void *pv = NULL;
    HBITMAP hbm = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pv, NULL, 0);
    if (hbm) {
        SelectObject(hdc, hbm);
        if (outBitmap) *outBitmap = hbm;
    } else {
        DeleteDC(hdc);
        hdc = NULL;
    }
    return hdc;
}

static void _DrawGlow(HDC hdcTarget, RECT rcItem, DWORD animStart, DWORD now, COLORREF clrGlow) {
    int w = rcItem.right - rcItem.left;
    int h = rcItem.bottom - rcItem.top;
    if (w <= 0 || h <= 0) return;

    float progress = (now - animStart) / GLOW_DURATION;
    if (progress < 0) progress = 0;
    if (progress > 1.0f) progress = 1.0f;

    float ease = 1.0f - powf(1.0f - progress, 2.0f);
    BYTE alpha = (BYTE)(80.0f * ease); // slightly toned down

    HBITMAP hbm = NULL;
    HDC hdcAlpha = _CreateARGBBuffer(hdcTarget, w, h, &hbm);
    if (!hdcAlpha) return;

    // fill with glow color (we'll use alpha in AlphaBlend)
    HBRUSH hbr = CreateSolidBrush(clrGlow);
    FillRect(hdcAlpha, &(RECT){0, 0, w, h}, hbr);
    DeleteObject(hbr);

    BLENDFUNCTION bf = { AC_SRC_OVER, 0, alpha, AC_SRC_ALPHA };
    GdiAlphaBlend(hdcTarget, rcItem.left, rcItem.top, w, h, hdcAlpha, 0, 0, w, h, bf);

    DeleteObject(hbm);
    DeleteDC(hdcAlpha);
}

// Gradient helper (vertical)
static void _FillVerticalGradient(HDC hdc, RECT rc, COLORREF top, COLORREF bottom) {
    TRIVERTEX v[2];
    v[0].x = rc.left; v[0].y = rc.top;
    v[0].Red   = ((GetRValue(top)   << 8) & 0xFF00);
    v[0].Green = ((GetGValue(top)   << 8) & 0xFF00);
    v[0].Blue  = ((GetBValue(top)   << 8) & 0xFF00);
    v[0].Alpha = 0x0000;

    v[1].x = rc.right; v[1].y = rc.bottom;
    v[1].Red   = ((GetRValue(bottom) << 8) & 0xFF00);
    v[1].Green = ((GetGValue(bottom) << 8) & 0xFF00);
    v[1].Blue  = ((GetBValue(bottom) << 8) & 0xFF00);
    v[1].Alpha = 0x0000;

    GRADIENT_RECT g; g.UpperLeft = 0; g.LowerRight = 1;
    GradientFill(hdc, v, 2, &g, 1, GRADIENT_FILL_RECT_V);
}

// Rounded rect fill using path (anti-aliased on modern GDI)
static void _FillRoundedRect(HDC hdc, const RECT rc, int radius, HBRUSH hbr) {
    HGDIOBJ oldBrush = SelectObject(hdc, hbr);

    BeginPath(hdc);
    RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, radius, radius);
    EndPath(hdc);
    FillPath(hdc);

    SelectObject(hdc, oldBrush);
}

// --- Public API ---
BOOL SidebarRegister(HINSTANCE hInstance) {
    WNDCLASSEXW wc = {0};
    if (GetClassInfoExW(hInstance, SIDEBAR_CLASS_NAME, &wc)) {
        return TRUE;
    }

    wc.cbSize        = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc   = _Sidebar_WndProc;
    wc.cbWndExtra    = sizeof(PSIDEBAR_DATA);
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszClassName = SIDEBAR_CLASS_NAME;
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    return RegisterClassExW(&wc) != 0;
}

// Add tooltip helper
static void _Sidebar_InitToolTip(PSIDEBAR_DATA pData, HWND hWnd) {
    if (pData->hwndToolTip && IsWindow(pData->hwndToolTip)) return;
    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_WIN95_CLASSES };
    InitCommonControlsEx(&icc);

    pData->hwndToolTip = CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, NULL,
        WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        hWnd, NULL, GetModuleHandle(NULL), NULL);
    if (!pData->hwndToolTip) return;

    // set max width to allow wrapping
    SendMessageW(pData->hwndToolTip, TTM_SETMAXTIPWIDTH, 0, 300);
    SendMessageW(pData->hwndToolTip, TTM_SETDELAYTIME, (WPARAM)TTDT_INITIAL, (LPARAM)300);
}

static void _Sidebar_UpdateToolTips(PSIDEBAR_DATA pData, HWND hWnd) {
    if (!pData->hwndToolTip) return;

    // Remove existing tools by enumerating and deleting isn't straightforward,
    // so we'll simply re-add tools: add each tool (TTF_SUBCLASS will manage)
    for (int i = 0; i < pData->itemCount; ++i) {
        RECT rc = { 0, (i + 1) * pData->itemHeight, pData->nCurrentWidth, (i + 2) * pData->itemHeight };
        TOOLINFOW ti;
        ZeroMemory(&ti, sizeof(ti));
        ti.cbSize = sizeof(ti);
        ti.uFlags = TTF_SUBCLASS;
        ti.hwnd = hWnd;
        ti.uId = (UINT_PTR)(i + 1); // unique id per item
        ti.lpszText = pData->items[i].pszText ? pData->items[i].pszText : L"";
        ti.rect = rc;
        SendMessageW(pData->hwndToolTip, TTM_ADDTOOLW, 0, (LPARAM)&ti);
    }
}

// --- Window Proc ---
static LRESULT CALLBACK _Sidebar_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    PSIDEBAR_DATA pData = (PSIDEBAR_DATA)GetWindowLongPtrW(hWnd, GWLP_USERDATA);

    switch (uMsg) {
        case WM_CREATE: {
            pData = (PSIDEBAR_DATA)calloc(1, sizeof(SIDEBAR_DATA));
            if (!pData) return -1;
            SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)pData);
            _Sidebar_SetDefaults(pData);

            // sizes: expanded (default) and collapsed
            RECT rc; GetClientRect(hWnd, &rc);
            pData->nExpandedWidth  = rc.right;
            pData->nCollapsedWidth = 64; // chosen collapsed width
            pData->nCurrentWidth   = pData->nExpandedWidth;
            pData->nTargetWidth    = pData->nExpandedWidth;

            _Sidebar_InitToolTip(pData, hWnd);
            return 0;
        }

        case WM_NCDESTROY: {
            if (pData) {
                KillTimer(hWnd, IDT_ANIMATION);
                SendMessageW(hWnd, SB_RESETCONTENT, 0, 0);
                if (pData->hwndToolTip) DestroyWindow(pData->hwndToolTip);
                free(pData->items);
                free(pData->pszFooterText);
                if (pData->bOwnsFont) {
                    DeleteObject(pData->hFont);
                }
                free(pData);
            }
            return 0;
        }

        case WM_PAINT:
            _Sidebar_OnPaint(hWnd);
            return 0;

        case WM_ERASEBKGND:
            return 1;

        case WM_SIZE: {
            InvalidateRect(hWnd, NULL, FALSE);
            if (pData) {
                RECT rc;
                GetClientRect(hWnd, &rc);
                if (!pData->bIsCollapsed) {
                    pData->nCurrentWidth = rc.right;
                    pData->nExpandedWidth = rc.right;
                }
            }
            return 0;
        }

        case WM_TIMER:
            if (wParam == IDT_ANIMATION && pData) {
                DWORD now = GetTickCount();
                float elapsed = (float)(now - pData->dwHotStartTime);
                float collapseElapsed = (float)(now - pData->dwCollapseStartTime);

                BOOL animationFinished = TRUE; 
                BOOL needInvalidate = FALSE;

                if (elapsed < SLIDE_DURATION || elapsed < GLOW_DURATION || elapsed < JIGGLE_DURATION) {
                    needInvalidate = TRUE;
                    animationFinished = FALSE;
                }

                if (pData->nCurrentWidth != pData->nTargetWidth) {
                    animationFinished = FALSE;
                    needInvalidate = TRUE;

                    float t = collapseElapsed / COLLAPSE_DURATION;
                    if (t >= 1.0f) {
                        t = 1.0f;
                        pData->nCurrentWidth = pData->nTargetWidth;
                        PostMessage(GetParent(hWnd), WM_SIZE, 0, 0);
                    } else {
                        float ease = 1.0f - powf(1.0f - t, 3.0f);
                        if(pData->bIsCollapsed){
                             pData->nCurrentWidth = (int)((float)pData->nCollapsedWidth * ease + (float)pData->nExpandedWidth * (1.0f - ease));
                        }else{
                            pData->nCurrentWidth = (int)((float)pData->nExpandedWidth * ease + (float)pData->nCollapsedWidth * (1.0f - ease));
                        }
                    }
                }

                if (needInvalidate) {
                    InvalidateRect(hWnd, NULL, FALSE);
                }

                if (animationFinished) {
                    KillTimer(hWnd, IDT_ANIMATION);
                }
            }
            return 0;

        case WM_MOUSEMOVE: {
            if (!IsWindowEnabled(hWnd)) return 0;
            int y = GET_Y_LPARAM(lParam);
            int iHit = y / pData->itemHeight;
            if (iHit >= pData->itemCount + 1) iHit = -1;

            if (iHit != pData->iHot) {
                pData->nSlideStartY = (pData->iHot != -1) ? pData->iHot * pData->itemHeight : iHit * pData->itemHeight;
                pData->nSlideTargetY = iHit * pData->itemHeight;
                pData->dwHotStartTime = GetTickCount();
                pData->iHot = iHit;

                SetTimer(hWnd, IDT_ANIMATION, ANIMATION_INTERVAL, NULL);
                InvalidateRect(hWnd, NULL, FALSE);

                TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hWnd, 0 };
                TrackMouseEvent(&tme);
            }
            if (pData->iHot > 0) {
                SetCursor(LoadCursor(NULL, IDC_HAND));
            } else {
                SetCursor(LoadCursor(NULL, IDC_ARROW));
            }
            return 0;
        }

        case WM_MOUSELEAVE:
            if (pData && pData->iHot != -1) {
                pData->nSlideStartY = pData->iHot * pData->itemHeight;
                pData->nSlideTargetY = pData->iHot * pData->itemHeight;
            }
            if (pData) pData->iHot = -1;
            InvalidateRect(hWnd, NULL, FALSE);
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return 0;

        case WM_LBUTTONDOWN: {
            if (!IsWindowEnabled(hWnd)) return 0;
            int y = GET_Y_LPARAM(lParam);
            int iHit = y / pData->itemHeight;

            if (iHit == 0) {
                pData->bIsCollapsed = !pData->bIsCollapsed;
                pData->nTargetWidth = pData->bIsCollapsed ? pData->nCollapsedWidth : pData->nExpandedWidth;
                pData->dwCollapseStartTime = GetTickCount();
                SetTimer(hWnd, IDT_ANIMATION, ANIMATION_INTERVAL, NULL);

                NMSBTOGGLE nm = { {hWnd, GetDlgCtrlID(hWnd), SBN_TOGGLECLICK}, pData->bIsCollapsed };
                SendMessageW(GetParent(hWnd), WM_NOTIFY, (WPARAM)nm.hdr.idFrom, (LPARAM)&nm);
                InvalidateRect(hWnd, NULL, TRUE);

                _Sidebar_UpdateToolTips(pData, hWnd);

            } else if (iHit > 0 && (iHit - 1) < pData->itemCount) {
                int iItem = iHit - 1;
                if (iItem != pData->iSel) {
                    pData->iSel = iItem;
                    InvalidateRect(hWnd, NULL, FALSE);
                    NMSIDEBAR nm = { {hWnd, GetDlgCtrlID(hWnd), SBN_SELCHANGED}, pData->iSel, pData->items[pData->iSel].lParam };
                    SendMessageW(GetParent(hWnd), WM_NOTIFY, (WPARAM)nm.hdr.idFrom, (LPARAM)&nm);
                }
            }
            return 0;
        }

        case WM_ENABLE:
            InvalidateRect(hWnd, NULL, TRUE);
            return 0;

        case WM_SETFONT: {
            if (pData && pData->bOwnsFont) {
                DeleteObject(pData->hFont);
            }
            if (pData) {
                pData->hFont = (HFONT)wParam;
                pData->bOwnsFont = FALSE;
                if (LOWORD(lParam)) InvalidateRect(hWnd, NULL, TRUE);
            }
            return 0;
        }

        case SB_ADDITEM: {
            int result = _Sidebar_AddItem(pData, (PSIDEBAR_ITEM)lParam);
            if (result != -1) {
                InvalidateRect(hWnd, NULL, TRUE);
                _Sidebar_UpdateToolTips(pData, hWnd);
            }
            return result;
        }

        case SB_GETCURSEL:
            return pData ? pData->iSel : -1;

        case SB_SETCURSEL: {
            if (pData) {
                int oldSel = pData->iSel;
                int newSel = (int)wParam;
                if (newSel >= -1 && newSel < pData->itemCount) {
                    pData->iSel = newSel;
                    InvalidateRect(hWnd, NULL, TRUE);
                }
                return oldSel;
            }
            return -1;
        }

        case SB_RESETCONTENT: {
            if (pData && pData->items) {
                for (int i = 0; i < pData->itemCount; ++i) {
                    if (pData->items[i].pszText) free(pData->items[i].pszText);
                }
                pData->itemCount = 0;
                pData->iSel = -1;
                pData->iHot = -1;
                InvalidateRect(hWnd, NULL, TRUE);
            }
            return 0;
        }

        case SB_SETCOLORS:
            if (pData && lParam) {
                pData->colors = *((PSIDEBAR_COLORS)lParam);
                InvalidateRect(hWnd, NULL, TRUE);
            }
            return 0;

        case SB_TOGGLECOLLAPSE:
            if (pData) {
                pData->bIsCollapsed = !pData->bIsCollapsed;
                pData->nTargetWidth = pData->bIsCollapsed ? pData->nCollapsedWidth : pData->nExpandedWidth;
                pData->dwCollapseStartTime = GetTickCount();
                SetTimer(hWnd, IDT_ANIMATION, ANIMATION_INTERVAL, NULL);
                InvalidateRect(hWnd, NULL, TRUE);
                _Sidebar_UpdateToolTips(pData, hWnd);
                return pData->bIsCollapsed;
            }
            return 0;

        case SB_ISCOLLAPSED:
            return pData ? pData->bIsCollapsed : FALSE;

        case SB_SETFOOTER:
            if (pData && lParam) {
                PSIDEBAR_FOOTER_INFO pFooterInfo = (PSIDEBAR_FOOTER_INFO)lParam;
                if (pData->pszFooterText) free(pData->pszFooterText);
                pData->pszFooterText = pFooterInfo->pszText ? _wcsdup(pFooterInfo->pszText) : NULL;
                pData->hFooterIcon = pFooterInfo->hIcon;
                InvalidateRect(hWnd, NULL, TRUE);
            }
            return 0;
    }

    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

// --- Helpers ---
static void _Sidebar_SetDefaults(PSIDEBAR_DATA pData) {
    pData->iSel = -1;
    pData->iHot = -1;
    pData->itemHeight = 52;
    pData->bIsCollapsed = FALSE;
    pData->dwHotStartTime = 0;
    pData->nSlideStartY = -1;
    pData->nSlideTargetY = -1;

    pData->colors.clrBackgroundTop      = RGB(34, 36, 44);
    pData->colors.clrBackgroundBottom   = RGB(24, 26, 32);
    pData->colors.clrSeparator          = RGB(60, 60, 68);
    pData->colors.clrItemText           = RGB(220, 220, 220);
    pData->colors.clrItemTextHot        = RGB(255, 255, 255);
    pData->colors.clrItemTextSel        = RGB(255, 255, 255);
    pData->colors.clrItemBackgroundHot  = RGB(75, 85, 99);
    pData->colors.clrItemBackgroundSel  = RGB(0, 120, 215);
    pData->colors.clrIndicator          = RGB(0, 150, 255);

    LOGFONTW lf;
    ZeroMemory(&lf, sizeof(lf));
    lf.lfHeight = -16;
    lf.lfWeight = FW_NORMAL;
    wcsncpy(lf.lfFaceName, L"Segoe UI", LF_FACESIZE);
    pData->hFont = CreateFontIndirectW(&lf);
    pData->bOwnsFont = TRUE;

    pData->nExpandedWidth = 240;
    pData->nCollapsedWidth = 64;
    pData->nCurrentWidth = pData->nExpandedWidth;
    pData->nTargetWidth = pData->nExpandedWidth;
}

static int _Sidebar_AddItem(PSIDEBAR_DATA pData, PSIDEBAR_ITEM pItem) {
    if (!pData || !pItem) return -1;
    if (pData->itemCount >= pData->itemCapacity) {
        int newCapacity = (pData->itemCapacity == 0) ? 8 : pData->itemCapacity * 2;
        _SIDEBAR_ITEM* newItems = realloc(pData->items, newCapacity * sizeof(_SIDEBAR_ITEM));
        if (!newItems) return -1;
        pData->items = newItems;
        pData->itemCapacity = newCapacity;
    }
    _SIDEBAR_ITEM* newItem = &pData->items[pData->itemCount];
    newItem->pszText = _wcsdup(pItem->pszText);
    newItem->lParam = pItem->lParam;
    newItem->hIcon = pItem->hIcon;
    if (!newItem->pszText) return -1;
    return pData->itemCount++;
}

// Helper: get client height/width
static int GetClientRectWidth(HWND hWnd) {
    RECT rc; GetClientRect(hWnd, &rc); return rc.right - rc.left;
}
static int GetClientRectHeight(HWND hWnd) {
    RECT rc; GetClientRect(hWnd, &rc); return rc.bottom - rc.top;
}

// --- Paint ---
static void _Sidebar_OnPaint(HWND hWnd) {
    PSIDEBAR_DATA pData = (PSIDEBAR_DATA)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
    if (!pData) return;

    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);
    RECT rcClient;
    GetClientRect(hWnd, &rcClient);
    
    RECT rcDraw;
    rcDraw.left = 0;
    rcDraw.top = 0;
    rcDraw.right = pData->nCurrentWidth; // Draw only the animated width
    rcDraw.bottom = rcClient.bottom;

    if (rcDraw.right <= 0) {
        EndPaint(hWnd, &ps);
        return;
    }

    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hbmMem = CreateCompatibleBitmap(hdc, rcDraw.right, rcDraw.bottom);
    HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);
    DWORD now = GetTickCount();
    BOOL bEnabled = IsWindowEnabled(hWnd);

    _FillVerticalGradient(hdcMem, rcDraw, pData->colors.clrBackgroundTop, pData->colors.clrBackgroundBottom);

    int barHeight = pData->itemHeight;
    HPEN hPen = CreatePen(PS_SOLID, 1, pData->colors.clrSeparator);
    HPEN hOldPen = (HPEN)SelectObject(hdcMem, hPen);
    MoveToEx(hdcMem, 8, barHeight - 1, NULL);
    LineTo(hdcMem, rcDraw.right - 8, barHeight - 1);
    SelectObject(hdcMem, hOldPen);
    DeleteObject(hPen);

    if (bEnabled && pData->iHot != -1 && pData->iHot > 0) {
        int hotIndex = pData->iHot - 1;
        RECT rcHotItem = { 8, (hotIndex + 1) * pData->itemHeight + 6, rcDraw.right - 8, (hotIndex + 2) * pData->itemHeight - 6 };

        if (hotIndex != pData->iSel) {
            _DrawGlow(hdcMem, rcHotItem, pData->dwHotStartTime, now, pData->colors.clrItemBackgroundHot);
        }

        float slideElapsed = (float)(now - pData->dwHotStartTime);
        float slideProgress = slideElapsed / SLIDE_DURATION;
        if (slideProgress > 1.0f) slideProgress = 1.0f;
        float ease = 1.0f - powf(1.0f - slideProgress, 3);
        int currentY = pData->nSlideStartY + (int)((pData->nSlideTargetY - pData->nSlideStartY) * ease);
        RECT rcIndicator = {4, currentY + 6, 12, currentY + 6 + pData->itemHeight - 12};

        TRIVERTEX v[2];
        v[0].x = rcIndicator.left; v[0].y = rcIndicator.top;
        v[0].Red   = ((GetRValue(pData->colors.clrIndicator) << 8) & 0xFF00);
        v[0].Green = ((GetGValue(pData->colors.clrIndicator) << 8) & 0xFF00);
        v[0].Blue  = ((GetBValue(pData->colors.clrIndicator) << 8) & 0xFF00);
        v[0].Alpha = 0x0000;
        v[1].x = rcIndicator.right; v[1].y = rcIndicator.bottom;
        v[1].Red   = ((GetRValue(RGB(0, 220, 255)) << 8) & 0xFF00);
        v[1].Green = ((GetGValue(RGB(0, 220, 255)) << 8) & 0xFF00);
        v[1].Blue  = ((GetBValue(RGB(0, 220, 255)) << 8) & 0xFF00);
        v[1].Alpha = 0x0000;
        GRADIENT_RECT gr = {0,1};
        GradientFill(hdcMem, v, 2, &gr, 1, GRADIENT_FILL_RECT_V);

        RECT rcGlow = { rcIndicator.left - 6, rcIndicator.top - 6, rcIndicator.right + 6, rcIndicator.bottom + 6 };
        _DrawGlow(hdcMem, rcGlow, pData->dwHotStartTime, now, pData->colors.clrIndicator);
    }

    if (pData->iSel != -1) {
        RECT rcSel = { 8, (pData->iSel + 1) * pData->itemHeight + 6, rcDraw.right - 8, (pData->iSel + 2) * pData->itemHeight - 6 };
        HBRUSH hbrSel = CreateSolidBrush(pData->colors.clrItemBackgroundSel);
        _FillRoundedRect(hdcMem, rcSel, 10, hbrSel);
        DeleteObject(hbrSel);
    }

    HFONT hOldFont = (HFONT)SelectObject(hdcMem, pData->hFont);
    SetBkMode(hdcMem, TRANSPARENT);

    int barW = 24, barH = 3, barS = 6;
    int barX = (rcDraw.right - barW) / 2, barY = (pData->itemHeight - (3*barH + 2*barS)) / 2 + 2;
    HBRUSH hbrBar = CreateSolidBrush(bEnabled ? pData->colors.clrItemText : GetSysColor(COLOR_GRAYTEXT));
    for (int b = 0; b < 3; ++b) {
        RECT r = { barX, barY + b * (barH + barS), barX + barW, barY + b * (barH + barS) + barH };
        FillRect(hdcMem, &r, hbrBar);
    }
    DeleteObject(hbrBar);

    for (int i = 0; i < pData->itemCount; ++i) {
        RECT rcItem = { 8, (i + 1) * pData->itemHeight, rcDraw.right - 8, (i + 2) * pData->itemHeight };
        BOOL isSelected = (i == pData->iSel);
        BOOL isHot = ((i + 1) == pData->iHot);

        COLORREF txtColor;
        if (!bEnabled) txtColor = GetSysColor(COLOR_GRAYTEXT);
        else txtColor = isSelected ? pData->colors.clrItemTextSel : (isHot ? pData->colors.clrItemTextHot : pData->colors.clrItemText);

        if (pData->items[i].hIcon) {
            int iconSize = 28;
            int iconX;
            if (pData->nCurrentWidth < pData->nExpandedWidth - 10) { // In collapsed or animating state
                iconX = (pData->nCurrentWidth - iconSize) / 2;
                if (iconX < 6) iconX = 6;
            } else { // Fully expanded
                iconX = rcItem.left + 8;
            }
            int iconY = rcItem.top + (pData->itemHeight - iconSize) / 2;
            if (bEnabled && isHot) {
                float jiggleElapsed = (float)(now - pData->dwHotStartTime);
                float jiggleProgress = jiggleElapsed / JIGGLE_DURATION;
                if (jiggleProgress > 1.0f) jiggleProgress = 1.0f;
                iconY += (int)(sinf(jiggleProgress * 2.0f * (float)PI) * 2.0f);
            }
            DrawIconEx(hdcMem, iconX, iconY, pData->items[i].hIcon, iconSize, iconSize, 0, NULL, DI_NORMAL);
        }

        if (pData->nCurrentWidth > pData->nCollapsedWidth + 20) { // Start fading in text
            int currentX = rcItem.left + 8;
            if (pData->items[i].hIcon) currentX += 28 + 12;
            SetTextColor(hdcMem, txtColor);
            RECT rcText = { currentX, rcItem.top, rcItem.right - 10, rcItem.bottom };
            DrawTextW(hdcMem, pData->items[i].pszText, -1, &rcText, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS);
        }
    }

    if (pData->hFooterIcon || pData->pszFooterText) {
        RECT rcFooter = { 8, rcClient.bottom - pData->itemHeight - 10, rcDraw.right - 8, rcClient.bottom - 6 };

        // Check if we are in the collapsed state (or animating towards it)
        if (pData->nCurrentWidth < pData->nExpandedWidth - 10) {
            
            if (pData->hFooterIcon) {
                int bigIconSize = 40;
                
                int iconX = (pData->nCurrentWidth - bigIconSize) / 2;
                if (iconX < 0) iconX = 0;


                int iconY = rcFooter.top + (pData->itemHeight - bigIconSize) / 2;
                

                DrawIconEx(hdcMem, iconX, iconY, pData->hFooterIcon, bigIconSize, bigIconSize, 0, NULL, DI_NORMAL);
            }

        } else {
            // --- EXPANDED STATE (Standard Size) ---
            int currentX = 12;
            int smallIconSize = 28;

            if (pData->hFooterIcon) {
                DrawIconEx(hdcMem, currentX, rcFooter.top + 4, pData->hFooterIcon, smallIconSize, smallIconSize, 0, NULL, DI_NORMAL);
                currentX += smallIconSize + 10;
            }
            if (pData->pszFooterText) {
                SetTextColor(hdcMem, bEnabled ? pData->colors.clrItemText : GetSysColor(COLOR_GRAYTEXT));
                RECT rcText = { currentX, rcFooter.top + 4, rcFooter.right - 10, rcFooter.bottom - 4 };
                DrawTextW(hdcMem, pData->pszFooterText, -1, &rcText, DT_WORDBREAK | DT_LEFT);
            }
        }
    }

    SelectObject(hdcMem, hOldFont);
    BitBlt(hdc, 0, 0, rcDraw.right, rcDraw.bottom, hdcMem, 0, 0, SRCCOPY);
    SelectObject(hdcMem, hbmOld);
    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
    EndPaint(hWnd, &ps);
}