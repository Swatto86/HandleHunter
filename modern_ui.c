/*
 * modern_ui.c - Windows theme detection and styling support
 * 
 * This file provides functions for detecting and applying Windows theme settings.
 * It handles dark mode detection, title bar styling, and modern UI features like
 * rounded corners on Windows 11.
 * 
 * WHY THIS FILE EXISTS:
 * - Windows 10+ supports dark mode, but it's not automatic
 * - Applications must explicitly enable dark mode for their windows
 * - Windows 11 introduced rounded corners that must be explicitly enabled
 * - Without this, the app would always have light theme regardless of system setting
 * 
 * APPROACH:
 * - Read dark mode setting from Windows registry
 * - Use DWM (Desktop Window Manager) API to style title bar
 * - Use UxTheme API to style controls
 * 
 * COMPATIBILITY:
 * - Functions gracefully degrade on older Windows versions
 * - DWM functions fail silently if not supported
 * - App works fine even if dark mode isn't available
 */

#include <windows.h>      // Core Windows API
#include <dwmapi.h>       // Desktop Window Manager API (for title bar styling)
#include <commctrl.h>     // Common Controls
#include <uxtheme.h>      // Theme API (for control styling)
#include "modern_ui.h"    // Function declarations

// ============================================
// DWM Constants (not in all SDKs)
// ============================================
// WHY DEFINE THESE: Some MinGW versions don't include these constants
//                   Defining them ensures compatibility across toolchains

// DWMWA_USE_IMMERSIVE_DARK_MODE: DWM attribute for dark mode title bar
// Available since Windows 10 version 1809 (October 2018 Update)
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

// DWMWA_WINDOW_CORNER_PREFERENCE: DWM attribute for window corner rounding
// Available since Windows 11
#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif

// DWMWCP_ROUND: Value for rounded corners
#ifndef DWMWCP_ROUND
#define DWMWCP_ROUND 2
#endif

/**
 * IsWindowsDarkMode - Check if Windows is using dark mode
 * 
 * This function reads the user's theme preference from the Windows registry.
 * Windows stores theme settings in the Personalize registry key.
 * 
 * WHY READ FROM REGISTRY:
 * - Windows doesn't provide a direct API to check dark mode
 * - Registry is the authoritative source for theme settings
 * - This is the officially documented approach from Microsoft
 * 
 * REGISTRY PATH:
 * - HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Themes\Personalize
 * - Value: "AppsUseLightTheme" (DWORD)
 *   - 0 = Dark mode (user wants dark theme for apps)
 *   - 1 = Light mode (user wants light theme for apps)
 * 
 * WHY "AppsUseLightTheme" AND NOT "AppsDarkMode":
 * - Microsoft chose to store it as "use light theme" with inverted logic
 * - 0 (false) = don't use light theme = use dark theme
 * - 1 (true) = use light theme
 * 
 * @return TRUE if dark mode is enabled, FALSE if light mode or error
 */
BOOL IsWindowsDarkMode(void) {
    HKEY hKey;           // Registry key handle
    DWORD value = 1;     // Default to light mode (1) if registry read fails
    DWORD size = sizeof(DWORD);  // Size of value we're reading
    
    // ============================================
    // STEP 1: Open the registry key
    // ============================================
    // RegOpenKeyExW opens a registry key for reading
    // WHY W suffix: Wide character version (Unicode)
    if (RegOpenKeyExW(
            HKEY_CURRENT_USER,  // Root key (current user's settings)
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",  // Path to key
            0,                  // Must be 0 (reserved)
            KEY_READ,           // Access rights (read-only)
            &hKey               // Output: handle to opened key
        ) == ERROR_SUCCESS) {
        
        // ============================================
        // STEP 2: Read the value from the key
        // ============================================
        // RegQueryValueExW reads a value from the opened key
        RegQueryValueExW(
            hKey,               // Key handle
            L"AppsUseLightTheme",  // Value name to read
            NULL,               // Reserved (must be NULL)
            NULL,               // Output: value type (we don't need it)
            (LPBYTE)&value,     // Output: buffer to receive value
            &size               // Input/Output: size of buffer
        );
        
        // ============================================
        // STEP 3: Close the registry key
        // ============================================
        // Always close registry keys to avoid resource leaks
        RegCloseKey(hKey);
    }
    // If RegOpenKeyExW fails, we keep default value (1 = light mode)
    
    // ============================================
    // STEP 4: Return result with inverted logic
    // ============================================
    // Return TRUE if dark mode (value is 0), FALSE if light mode (value is 1)
    // WHY INVERT: Registry stores "use light theme", we want "is dark mode"
    return (value == 0);
}

/**
 * EnableDarkMode - Enable dark mode for window title bar and borders
 * 
 * This function tells the Desktop Window Manager (DWM) to use dark theme
 * colors for the non-client area (title bar, borders, resize handles).
 * 
 * WHY ONLY IF DARK MODE IS ENABLED:
 * - We respect the user's theme choice
 * - If user has light mode, we don't force dark title bar
 * - This makes the app feel native to Windows
 * 
 * WHAT THIS AFFECTS:
 * - Title bar background color (white -> dark gray)
 * - Title bar text color (black -> white)
 * - Window border color
 * - Minimize/maximize/close button colors
 * 
 * WHAT THIS DOESN'T AFFECT:
 * - Client area (our controls) - those use system colors automatically
 * - ListView, buttons, etc. - they respect Windows theme independently
 * 
 * COMPATIBILITY:
 * - Windows 10 1809+: Works as expected
 * - Older Windows: DwmSetWindowAttribute fails silently, no effect
 * 
 * @param hwnd - Handle to window to apply dark mode to
 */
void EnableDarkMode(HWND hwnd) {
    // Only enable if Windows is in dark mode
    if (IsWindowsDarkMode()) {
        // ============================================
        // Tell DWM to use dark theme for title bar
        // ============================================
        BOOL value = TRUE;  // TRUE = enable dark mode
        
        // DwmSetWindowAttribute sets various DWM attributes for a window
        DwmSetWindowAttribute(
            hwnd,                             // Window to modify
            DWMWA_USE_IMMERSIVE_DARK_MODE,   // Which attribute to set
            &value,                           // Pointer to new value
            sizeof(value)                     // Size of value
        );
        // If this fails (old Windows), nothing happens - graceful degradation
    }
}

/**
 * ApplyModernWindowStyle - Apply Windows 11 styling (rounded corners)
 * 
 * Windows 11 introduced rounded window corners as part of the modern design language.
 * By default, windows have sharp corners. This function enables rounded corners.
 * 
 * WHY ROUNDED CORNERS:
 * - Part of Windows 11 design system (Fluent Design)
 * - Makes app look modern and native
 * - Consistent with other Windows 11 apps
 * 
 * COMPATIBILITY:
 * - Windows 11: Corners are rounded
 * - Windows 10 and older: DwmSetWindowAttribute fails silently, sharp corners remain
 * - No negative effects on older Windows
 * 
 * @param hwnd - Handle to window to apply styling to
 */
void ApplyModernWindowStyle(HWND hwnd) {
    // ============================================
    // Try to enable rounded corners (Windows 11)
    // ============================================
    DWORD cornerPreference = DWMWCP_ROUND;  // ROUND = rounded corners
    
    // DwmSetWindowAttribute sets various DWM attributes for a window
    DwmSetWindowAttribute(
        hwnd,                                  // Window to modify
        DWMWA_WINDOW_CORNER_PREFERENCE,       // Which attribute to set
        &cornerPreference,                     // Pointer to new value
        sizeof(cornerPreference)               // Size of value
    );
    // If this fails (Windows 10 or older), nothing happens - graceful degradation
}

/**
 * SubclassListViewForDarkMode - Configure ListView with Windows Explorer theme
 * 
 * This function applies the "Explorer" theme to our ListView control.
 * The Explorer theme makes the ListView look like Windows File Explorer.
 * 
 * WHY USE EXPLORER THEME:
 * - More polished appearance than default ListView
 * - Automatically respects Windows dark/light mode for content
 * - Selection colors, hover effects match File Explorer
 * - Professional, familiar look to users
 * 
 * HOW IT WORKS:
 * - SetWindowTheme tells Windows to apply a named theme to a control
 * - "Explorer" theme is built into Windows (used by File Explorer)
 * - The theme handles colors, selection, hover, etc. automatically
 * 
 * WHAT THIS AFFECTS:
 * - Row selection color
 * - Hover highlighting
 * - Grid line colors
 * - Column header styling
 * - Automatically adapts to dark/light mode
 * 
 * COMPATIBILITY:
 * - Windows Vista+: Works as expected
 * - Older Windows: SetWindowTheme fails, ListView uses default theme
 * 
 * @param hwndListView - Handle to ListView control to style
 */
void SubclassListViewForDarkMode(HWND hwndListView) {
    // ============================================
    // Apply Explorer theme to ListView
    // ============================================
    // SetWindowTheme tells Windows which theme to use for this control
    SetWindowTheme(
        hwndListView,    // Control to theme
        L"Explorer",     // Theme name (Windows Explorer theme)
        NULL             // Sub-app name (NULL = use default)
    );
    // WHY "Explorer": It's a built-in Windows theme that looks professional
    //                 and automatically handles dark/light mode
    //
    // ALTERNATIVE: Could use NULL for default theme, but Explorer looks better
}

