#ifndef UTILS_H
#define UTILS_H

wchar_t* LoadResString(HINSTANCE hInst, UINT id);
wchar_t* utf8_to_wchar(const char* utf8_string);
char* wchar_to_utf8(const wchar_t* wide_string);
COLORREF AdjustColor(COLORREF color, int amount);
HICON RecolorMenuIcon(HINSTANCE hInst, WORD iconID, COLORREF newColor);
HBITMAP CreateMenuIconBitmapTransparent(HICON hIcon);
HICON LoadAndRecolorIcon(HINSTANCE hInst, WORD iconID, COLORREF newColor);
#endif