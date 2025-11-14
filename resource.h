/*
 * resource.h - Control IDs and resource identifiers
 * 
 * This header defines all the control IDs used in the application.
 * These IDs are used to identify which control sent a WM_COMMAND message.
 * 
 * ID ranges:
 * - 1001-1099: Edit boxes and static controls
 * - 1100-1199: Buttons
 * - 1200-1299: ListView and other complex controls
 * - 2000-2099: Menu items (if menus are added)
 */

#ifndef RESOURCE_H
#define RESOURCE_H

// ============================================
// Control IDs (used in WM_COMMAND messages)
// ============================================

// Edit boxes
#define IDC_SERVER_EDIT      1001    // Server name input box
#define IDC_SEARCH_EDIT      1002    // Search filter input box

// Complex controls
#define IDC_LISTVIEW         1003    // ListView showing file locks

// Buttons
#define IDC_CONNECT_BTN      1004    // "Connect" button
#define IDC_REFRESH_BTN      1005    // "Refresh" button
#define IDC_RELEASE_BTN      1006    // "Release Selected Lock" button

// Status bar
#define IDC_STATUSBAR        1007    // Status bar at bottom of window

// ============================================
// Menu IDs (for future menu implementation)
// ============================================
#define IDM_FILE_EXIT        2001    // File → Exit menu item
#define IDM_HELP_ABOUT       2002    // Help → About menu item
#define IDM_HELP_DIAGNOSTICS 2003    // Help → Network Diagnostics menu item

#endif // RESOURCE_H

