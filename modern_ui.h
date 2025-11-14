/*
 * modern_ui.h - Windows theme detection and styling support
 */

#ifndef MODERN_UI_H
#define MODERN_UI_H

#include <windows.h>

// Function declarations
BOOL IsWindowsDarkMode(void);
void EnableDarkMode(HWND hwnd);
void ApplyModernWindowStyle(HWND hwnd);
void SubclassListViewForDarkMode(HWND hwndListView);

#endif // MODERN_UI_H

