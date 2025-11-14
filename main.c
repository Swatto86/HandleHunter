/*
 * main.c - Application entry point and event handlers
 * 
 * This file contains WinMain (the Windows GUI application entry point)
 * and event handler functions that respond to user actions.
 */

#include <windows.h>    // Windows API
#include <commctrl.h>   // Common controls (ListView, StatusBar)
#include <stdio.h>       // Standard I/O (for swprintf)
#include "gui.h"          // GUI functions and AppState
#include "netapi.h"       // Network API wrappers

/**
 * Windows GUI Application Entry Point
 * 
 * @param hInstance      - Handle to current application instance
 * @param hPrevInstance  - Always NULL in Win32 (not used)
 * @param lpCmdLine      - Command line arguments (ANSI string)
 * @param nCmdShow       - How window should be shown (minimized, maximized, etc.)
 * @return Exit code (0 = success, non-zero = error)
 * 
 * This is the main entry point for Windows GUI applications.
 * Unlike console apps (which use main()), GUI apps use WinMain.
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                   LPSTR lpCmdLine, int nCmdShow) {
    
    // Initialize Common Controls library
    // This must be called before creating any ListView, StatusBar, or other
    // common controls. It loads the modern themed controls (Windows XP+).
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);              // Structure size
    icex.dwICC = ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES;     // Request ListView and StatusBar
    InitCommonControlsEx(&icex);
    
    // Create the main application window
    // This registers the window class and creates the window (but doesn't show it yet)
    HWND hwndMain = CreateMainWindow(hInstance);
    if (!hwndMain) {
        // Window creation failed - exit with error code
        return 1;
    }
    
    // Show and update the window
    // nCmdShow tells us how to show it (normal, minimized, maximized, etc.)
    ShowWindow(hwndMain, nCmdShow);
    UpdateWindow(hwndMain);  // Force immediate paint (sends WM_PAINT)
    
    // Main message loop - this is the heart of every Windows GUI application
    // GetMessage retrieves messages from the message queue (clicks, keypresses, etc.)
    // It blocks when there are no messages (saves CPU)
    // Returns FALSE when WM_QUIT is received (PostQuitMessage was called)
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        // Translate virtual-key messages into character messages
        // (e.g., converts WM_KEYDOWN to WM_CHAR for text input)
        TranslateMessage(&msg);
        
        // Send message to the appropriate window procedure
        // This is where our MainWindowProc function gets called
        DispatchMessage(&msg);
    }
    
    // Return the exit code from WM_QUIT message
    // This is typically 0 for normal exit
    return (int)msg.wParam;
}

/**
 * Handle "Connect" button click
 * 
 * @param hwnd  - Handle to main window
 * @param state - Pointer to application state
 * 
 * Reads server name from edit box, validates it, adds "\\" prefix if needed,
 * and triggers a refresh to enumerate files.
 */
void OnConnect(HWND hwnd, AppState* state) {
    // Read server name from the edit control
    GetWindowTextW(state->hwndServer, state->currentServer, 256);
    
    // Validate that user entered something
    if (state->currentServer[0] == L'\0') {
        MessageBoxW(hwnd, L"Please enter a server name.", L"Input Required", MB_ICONINFORMATION);
        return;
    }
    
    // Ensure server name has "\\" prefix (UNC format)
    // Windows NetAPI functions expect "\\SERVERNAME" format
    if (wcsncmp(state->currentServer, L"\\\\", 2) != 0) {
        // No prefix - need to add it
        WCHAR temp[256];
        
        // Copy current server name to temp buffer
        wcsncpy(temp, state->currentServer, 255);
        temp[255] = L'\0';
        
        // Check if we have room for "\\" prefix (need 2 extra characters)
        if (wcslen(temp) < 254) {
            // Enough room - add prefix
            swprintf(state->currentServer, 256, L"\\\\%s", temp);
        } else {
            // Server name too long - truncate before adding prefix
            wcsncpy(temp, state->currentServer, 254);
            temp[254] = L'\0';
            swprintf(state->currentServer, 256, L"\\\\%s", temp);
        }
    }
    
    // Trigger refresh to enumerate files from the server
    OnRefresh(hwnd, state);
}

/**
 * Handle "Refresh" button click or auto-refresh
 * 
 * @param hwnd  - Handle to main window
 * @param state - Pointer to application state
 * 
 * Enumerates all open files from the current server and updates the ListView.
 */
void OnRefresh(HWND hwnd, AppState* state) {
    // Check that we have a server name
    if (state->currentServer[0] == L'\0') {
        MessageBoxW(hwnd, L"Please connect to a server first.", L"No Server", MB_ICONINFORMATION);
        return;
    }
    
    // Show "Loading..." message in status bar
    // This gives user feedback that something is happening
    SetWindowTextW(state->hwndStatus, L"Loading...");
    
    // Enumerate all open files from the server
    // This may take a few seconds on servers with many open files
    if (EnumerateOpenFiles(state->currentServer, &state->locks)) {
        // Success - update the ListView with new data
        UpdateListView(state);
        
        // Update status bar with count of files found
        WCHAR msg[256];
        swprintf(msg, 256, L"Found %d open files on %s", 
                 (int)state->locks.count, state->currentServer);
        SetWindowTextW(state->hwndStatus, msg);
    } else {
        // Enumeration failed - show error in status bar
        // (Detailed error message was already shown by EnumerateOpenFiles)
        SetWindowTextW(state->hwndStatus, L"Failed to enumerate files");
    }
}

/**
 * Handle "Release Lock" button click or double-click on ListView item
 * 
 * @param hwnd  - Handle to main window
 * @param state - Pointer to application state
 * 
 * Gets the selected file lock, shows confirmation dialog, and closes the file
 * handle if user confirms. Then refreshes the list.
 */
void OnReleaseLock(HWND hwnd, AppState* state) {
    // Get the index of the currently selected item in the ListView
    // -1 means start search from beginning, LVNI_SELECTED means find selected item
    int selected = ListView_GetNextItem(state->hwndListView, -1, LVNI_SELECTED);
    
    // Check if user actually selected something
    if (selected == -1) {
        MessageBoxW(hwnd, L"Please select a file to release.", L"No Selection", MB_ICONINFORMATION);
        return;
    }
    
    // Retrieve the lock information from the ListView item
    // We stored the array index in lParam when we added the item
    LVITEMW item = {0};
    item.mask = LVIF_PARAM;      // We want the lParam value
    item.iItem = selected;       // Which item to get
    ListView_GetItem(state->hwndListView, &item);
    
    // Get pointer to the actual FileLockInfo structure
    // item.lParam contains the index we stored when adding the item
    FileLockInfo* lock = &state->locks.items[item.lParam];
    
    // Show confirmation dialog before closing the file
    // This prevents accidental lock releases
    WCHAR confirmMsg[512];
    swprintf(confirmMsg, 512, 
        L"Release lock on:\n\n%s\n\nHeld by: %s\n\nAre you sure?",
        lock->fileName, lock->username);
    
    int result = MessageBoxW(hwnd, confirmMsg, L"Confirm Release", 
                            MB_YESNO | MB_ICONQUESTION);
    
    // Only proceed if user clicked "Yes"
    if (result == IDYES) {
        // Close the file handle (releases the lock)
        if (CloseFileLock(state->currentServer, lock->fileId)) {
            // Success - show confirmation and refresh the list
            MessageBoxW(hwnd, L"Lock released successfully!", L"Success", MB_ICONINFORMATION);
            OnRefresh(hwnd, state);  // Refresh to show updated list
        }
        // If CloseFileLock failed, it already showed an error message
    }
}

/**
 * Handle search text box change event
 * 
 * @param hwnd  - Handle to main window
 * @param state - Pointer to application state
 * 
 * Called whenever the user types in the search box. Updates the ListView
 * to show only matching items. The actual filtering happens in UpdateListView().
 */
void OnSearchChanged(HWND hwnd, AppState* state) {
    // Simply refresh the ListView display
    // UpdateListView() reads the search text and filters accordingly
    UpdateListView(state);
}

