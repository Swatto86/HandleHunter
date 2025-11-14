/*
 * modern_ui.h - Modern UI styling and dark mode support
 */

#ifndef MODERN_UI_H
#define MODERN_UI_H

#include <windows.h>

// Modern color scheme
#define COLOR_BG_DARK           RGB(32, 32, 32)
#define COLOR_BG_DARKER         RGB(24, 24, 24)
#define COLOR_BG_LIGHTER        RGB(45, 45, 45)
#define COLOR_ACCENT            RGB(0, 120, 212)
#define COLOR_ACCENT_HOVER      RGB(0, 140, 232)
#define COLOR_ACCENT_PRESSED    RGB(0, 100, 192)
#define COLOR_TEXT_PRIMARY      RGB(255, 255, 255)
#define COLOR_TEXT_SECONDARY    RGB(200, 200, 200)
#define COLOR_BORDER            RGB(70, 70, 70)
#define COLOR_BUTTON_BG         RGB(45, 45, 45)
#define COLOR_BUTTON_HOVER      RGB(55, 55, 55)
#define COLOR_BUTTON_PRESSED    RGB(35, 35, 35)
#define COLOR_GRIDLINE          RGB(60, 60, 60)

// Button states
#define BTN_STATE_NORMAL    0
#define BTN_STATE_HOVER     1
#define BTN_STATE_PRESSED   2

// Button info structure
typedef struct {
    int state;
    BOOL isHovered;
} ButtonInfo;

// Function declarations
void EnableDarkMode(HWND hwnd);
void ApplyModernWindowStyle(HWND hwnd);
HWND CreateModernButton(HWND parent, const WCHAR* text, int x, int y, int width, int height, int id, HINSTANCE hInst);
void DrawModernButton(LPDRAWITEMSTRUCT lpDrawItem);
LRESULT CALLBACK ButtonSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
LRESULT CALLBACK CustomEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
void SubclassEditForDarkMode(HWND hwndEdit);
void SubclassListViewForDarkMode(HWND hwndListView);

#endif // MODERN_UI_H

