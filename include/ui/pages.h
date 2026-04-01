#ifndef PAGES_H
#define PAGES_H

#include <windows.h>

extern HWND hwnd_home_page;
extern HWND hwnd_settings_page;
extern HWND hwnd_about_page;

void CreatePages(HWND parent);
void ShowPage(int pageId);
void ResizePages(int sidebarWidth, int parentWidth, int parentHeight);

#endif