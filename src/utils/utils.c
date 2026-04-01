#include <windows.h>
#include <stdlib.h>
#include <utils/utils.h>
#include <utils/err.h>
#include <windows.h>
#include <stdlib.h>

wchar_t* LoadResString(HINSTANCE hInst, UINT id)
{
    const int BUF_SIZE = 512;

    wchar_t* buf = malloc(BUF_SIZE * sizeof(wchar_t));
    if (!buf) {
        ERR(-1, "Malloc failed");
        return NULL;
    }

    int len = LoadStringW(hInst, id, buf, BUF_SIZE);
    if (len == 0) {
        ERR(-1, "LoadStringW failed");
        free(buf);
        return NULL;
    }

    // shrink buffer to exact size
    wchar_t* tmp = realloc(buf, (len + 1) * sizeof(wchar_t));
    if (tmp)
        buf = tmp;  // use smaller buffer if realloc succeeds

    return buf;     // caller must free()
}


wchar_t* utf8_to_wchar(const char* utf8_string) {
    if (!utf8_string || *utf8_string == '\0') {
        wchar_t* empty_str = (wchar_t*)malloc(sizeof(wchar_t));
        if (empty_str) empty_str[0] = L'\0';
        return empty_str;
    }
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8_string, -1, NULL, 0);
    if (len == 0) return NULL;
    wchar_t* wide_string = (wchar_t*)malloc(len * sizeof(wchar_t));
    if (!wide_string) return NULL;
    MultiByteToWideChar(CP_UTF8, 0, utf8_string, -1, wide_string, len);
    return wide_string;
}
char* wchar_to_utf8(const wchar_t* wide_string) {
    if (!wide_string || *wide_string == L'\0') {
        char* empty_str = (char*)malloc(sizeof(char));
        if (empty_str) empty_str[0] = '\0';
        return empty_str;
    }

    int len = WideCharToMultiByte(
        CP_UTF8,
        0,
        wide_string,
        -1,
        NULL,
        0,
        NULL,
        NULL
    );
    if (len == 0) return NULL;

    char* utf8_string = (char*)malloc(len);
    if (!utf8_string) return NULL;

    WideCharToMultiByte(
        CP_UTF8,
        0,
        wide_string,
        -1,
        utf8_string,
        len,
        NULL,
        NULL
    );

    return utf8_string;
}

COLORREF AdjustColor(COLORREF color, int amount) {
    int r = GetRValue(color);
    int g = GetGValue(color);
    int b = GetBValue(color);

    r = max(0, min(255, r + amount));
    g = max(0, min(255, g + amount));
    b = max(0, min(255, b + amount));

    return RGB(r, g, b);
}


HICON RecolorMenuIcon(HINSTANCE hInst, WORD iconID, COLORREF newColor)
{
    // Load original icon
    HICON hIcon = (HICON)LoadImageW(hInst, MAKEINTRESOURCEW(iconID), IMAGE_ICON, GetSystemMetrics(SM_CXMENUCHECK), GetSystemMetrics(SM_CYMENUCHECK), LR_SHARED);
    if(!hIcon) return NULL;

    ICONINFO ii = {0};
    if(!GetIconInfo(hIcon, &ii)) return hIcon; // fallback

    BITMAP bmpColor;
    GetObject(ii.hbmColor, sizeof(BITMAP), &bmpColor);

    // Prepare memory DCs
    HDC hdcMem = CreateCompatibleDC(NULL);
    HBITMAP hBmpNew = CreateCompatibleBitmap(GetDC(NULL), bmpColor.bmWidth, bmpColor.bmHeight);
    SelectObject(hdcMem, hBmpNew);

    // Copy icon into memory DC
    DrawIconEx(hdcMem, 0, 0, hIcon, bmpColor.bmWidth, bmpColor.bmHeight, 0, NULL, DI_NORMAL);

    // Recolor pixels
    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = bmpColor.bmWidth;
    bmi.bmiHeader.biHeight = -bmpColor.bmHeight; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    DWORD *pixels = malloc(bmpColor.bmWidth * bmpColor.bmHeight * sizeof(DWORD));
    GetDIBits(hdcMem, hBmpNew, 0, bmpColor.bmHeight, pixels, &bmi, DIB_RGB_COLORS);

    for(int i=0; i<bmpColor.bmWidth*bmpColor.bmHeight; i++){
        BYTE alpha = (pixels[i] >> 24) & 0xFF;
        if(alpha > 0){ // recolor non-transparent pixels
            pixels[i] = (alpha << 24) | (GetRValue(newColor) | (GetGValue(newColor)<<8) | (GetBValue(newColor)<<16));
        }
    }

    SetDIBits(hdcMem, hBmpNew, 0, bmpColor.bmHeight, pixels, &bmi, DIB_RGB_COLORS);
    free(pixels);

    // Create new icon
    ICONINFO iiNew = ii;
    iiNew.hbmColor = hBmpNew;
    HICON hIconNew = CreateIconIndirect(&iiNew);

    // Clean up
    DeleteObject(ii.hbmColor);
    DeleteObject(ii.hbmMask);
    DeleteDC(hdcMem);
    DestroyIcon(hIcon);

    return hIconNew;
}
HBITMAP CreateMenuIconBitmapTransparent(HICON hIcon) {
    int cx = GetSystemMetrics(SM_CXMENUCHECK);
    int cy = GetSystemMetrics(SM_CYMENUCHECK);

    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = cx;
    bmi.bmiHeader.biHeight = -cy; // top-down DIB
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pvBits = NULL;
    HDC hdc = GetDC(NULL);
    HBITMAP hbm = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0);
    ReleaseDC(NULL, hdc);

    if (!hbm) return NULL;

    HDC memDC = CreateCompatibleDC(NULL);
    HGDIOBJ oldBmp = SelectObject(memDC, hbm);

    // Fill with transparent color (ARGB = 0)
    RECT rc = {0, 0, cx, cy};
    HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(memDC, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));
    GdiSetBatchLimit(1); // force GDI to flush alpha properly

    // Important: Use DI_NORMAL | DI_COMPAT to keep alpha
    DrawIconEx(memDC, 0, 0, hIcon, cx, cy, 0, NULL, DI_NORMAL);

    SelectObject(memDC, oldBmp);
    DeleteDC(memDC);

    return hbm;
}
HICON LoadAndRecolorIcon(HINSTANCE hInst, WORD iconID, COLORREF newColor)
{
    // Load original icon
    HICON hIcon = (HICON)LoadImageW(hInst, MAKEINTRESOURCEW(iconID), IMAGE_ICON, 256, 256, LR_SHARED);
    if(!hIcon) return NULL;

    ICONINFO ii = {0};
    if(!GetIconInfo(hIcon, &ii)) return hIcon; // fallback

    BITMAP bmpColor;
    GetObject(ii.hbmColor, sizeof(BITMAP), &bmpColor);

    // Prepare memory DCs
    HDC hdcMem = CreateCompatibleDC(NULL);
    HBITMAP hBmpNew = CreateCompatibleBitmap(GetDC(NULL), bmpColor.bmWidth, bmpColor.bmHeight);
    SelectObject(hdcMem, hBmpNew);

    // Copy icon into memory DC
    DrawIconEx(hdcMem, 0, 0, hIcon, bmpColor.bmWidth, bmpColor.bmHeight, 0, NULL, DI_NORMAL);

    // Recolor pixels
    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = bmpColor.bmWidth;
    bmi.bmiHeader.biHeight = -bmpColor.bmHeight; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    DWORD *pixels = malloc(bmpColor.bmWidth * bmpColor.bmHeight * sizeof(DWORD));
    GetDIBits(hdcMem, hBmpNew, 0, bmpColor.bmHeight, pixels, &bmi, DIB_RGB_COLORS);

    for(int i=0; i<bmpColor.bmWidth*bmpColor.bmHeight; i++){
        BYTE alpha = (pixels[i] >> 24) & 0xFF;
        if(alpha > 0){ // recolor non-transparent pixels
            pixels[i] = (alpha << 24) | (GetRValue(newColor) | (GetGValue(newColor)<<8) | (GetBValue(newColor)<<16));
        }
    }

    SetDIBits(hdcMem, hBmpNew, 0, bmpColor.bmHeight, pixels, &bmi, DIB_RGB_COLORS);
    free(pixels);

    // Create new icon
    ICONINFO iiNew = ii;
    iiNew.hbmColor = hBmpNew;
    HICON hIconNew = CreateIconIndirect(&iiNew);

    // Clean up
    DeleteObject(ii.hbmColor);
    DeleteObject(ii.hbmMask);
    DeleteDC(hdcMem);
    DestroyIcon(hIcon);

    return hIconNew;
}