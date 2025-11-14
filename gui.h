/*
 * gui.h - GUI function declarations and AppState structure
 * 
 * This header declares all GUI-related functions and the AppState structure.
 * Include this in any file that needs to create windows or handle GUI events.
 */

#ifndef GUI_H
#define GUI_H

#include <windows.h>    // Windows API (HWND, etc.)
#include "lockinfo.h"     // AppState structure (defined here, but uses LockArray)

// ============================================
// Window Creation Functions
// ============================================

/**
 * Create and register the main application window
 * @param hInstance - Application instance handle
 * @return Window handle on success, NULL on failure
 */
HWND CreateMainWindow(HINSTANCE hInstance);

/**
 * Create all child controls (buttons, edit boxes, ListView, etc.)
 * @param hwndParent - Handle to parent window
 * @param state      - Pointer to AppState (to store control handles)
 */
void CreateControls(HWND hwndParent, AppState* state);

/**
 * Update the ListView with current lock data (applies search filter)
 * @param state - Pointer to application state
 */
void UpdateListView(AppState* state);

/**
 * Main window procedure - handles all messages for the main window
 * @param hwnd   - Window handle
 * @param msg    - Message identifier
 * @param wParam - Message parameter
 * @param lParam - Message parameter
 * @return Message result
 */
LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ============================================
// Event Handler Functions
// ============================================

/**
 * Handle "Connect" button click
 * @param hwnd  - Window handle
 * @param state - Application state
 */
void OnConnect(HWND hwnd, AppState* state);

/**
 * Handle "Refresh" button click
 * @param hwnd  - Window handle
 * @param state - Application state
 */
void OnRefresh(HWND hwnd, AppState* state);

/**
 * Handle "Release Lock" button click or double-click
 * @param hwnd  - Window handle
 * @param state - Application state
 */
void OnReleaseLock(HWND hwnd, AppState* state);

/**
 * Handle search text box change event
 * @param hwnd  - Window handle
 * @param state - Application state
 */
void OnSearchChanged(HWND hwnd, AppState* state);

#endif // GUI_H

