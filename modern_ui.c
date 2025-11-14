/*
 * modern_ui.c - Windows theme detection and styling support
 */

#include <windows.h>
#include <dwmapi.h>
#include <commctrl.h>
#include <uxtheme.h>
#include "modern_ui.h"

// DWM attribute for dark mode (Windows 10 1809+)
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

// Windows 11 corner rounding
#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif

#ifndef DWMWCP_ROUND
#define DWMWCP_ROUND 2
#endif

/**
 * Check if Windows is using dark mode
 * Reads from registry: HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Themes\Personalize
 * Value: AppsUseLightTheme (0 = dark mode, 1 = light mode)
 */
BOOL IsWindowsDarkMode(void) {
    HKEY hKey;
    DWORD value = 1; // Default to light mode if registry read fails
    DWORD size = sizeof(DWORD);
    
    if (RegOpenKeyExW(HKEY_CURRENT_USER, 
                      L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegQueryValueExW(hKey, L"AppsUseLightTheme", NULL, NULL, (LPBYTE)&value, &size);
        RegCloseKey(hKey);
    }
    
    // Return TRUE if dark mode (value is 0), FALSE if light mode (value is 1)
    return (value == 0);
}

/**
 * Enable dark mode for window title bar and borders (if Windows is in dark mode)
 */
void EnableDarkMode(HWND hwnd) {
    if (IsWindowsDarkMode()) {
        BOOL value = TRUE;
        DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
    }
}

/**
 * Apply modern window styling (Windows 11 rounded corners)
 */
void ApplyModernWindowStyle(HWND hwnd) {
    // Try to enable rounded corners (Windows 11)
    DWORD cornerPreference = DWMWCP_ROUND;
    DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &cornerPreference, sizeof(cornerPreference));
}

/**
 * Configure ListView with standard Windows theme
 */
void SubclassListViewForDarkMode(HWND hwndListView) {
    // Enable modern look with Explorer theme (uses system colors)
    SetWindowTheme(hwndListView, L"Explorer", NULL);
}

