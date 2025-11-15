/*
 * modern_ui.h - Windows theme detection and styling support
 */

#ifndef MODERN_UI_H
#define MODERN_UI_H

#include <windows.h>

// Theme color structure
typedef struct ThemeColors {
    COLORREF background;  // Background color for client area
    COLORREF text;        // Text color for labels and controls
    COLORREF editBg;      // Background color for edit controls
    COLORREF editText;    // Text color for edit controls
} ThemeColors;

// Function declarations
BOOL IsWindowsDarkMode(void);
void EnableDarkMode(HWND hwnd);
void ApplyModernWindowStyle(HWND hwnd);
void SubclassListViewForDarkMode(HWND hwndListView);
void GetThemeColors(ThemeColors* colors);
void ApplyThemeToWindow(HWND hwnd);

#endif // MODERN_UI_H

