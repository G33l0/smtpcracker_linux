#ifndef ABOUTPAGE_H
#define ABOUTPAGE_H
#include <windows.h>

HWND CreateAboutPage(HWND parent);
BOOL About_PageProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void ResizeAboutPage(int width, int height);
#endif