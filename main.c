/*
 * main.c - Application entry point
 * 
 * This file contains the WinMain entry point for the HandleHunter application.
 * It initializes Windows Common Controls, creates the main window, and runs
 * the message loop that keeps the application responsive.
 * 
 * WHY THIS FILE EXISTS:
 * - Every Windows GUI application needs an entry point (WinMain)
 * - This is the first code that runs when the user launches HandleHunter.exe
 * - It sets up the foundation before showing any UI
 * 
 * FLOW OF EXECUTION:
 * 1. Initialize Windows Common Controls (required for ListView, status bar)
 * 2. Create the main application window
 * 3. Load keyboard shortcuts (accelerators)
 * 4. Show the window to the user
 * 5. Enter message loop (handles all user interactions until exit)
 */

#include <windows.h>      // Core Windows API types and functions
#include <commctrl.h>     // Common Controls library (ListView, StatusBar, etc.)
#include "gui.h"          // Our GUI creation and management functions
#include "resource.h"     // Control IDs and resource identifiers

/**
 * WinMain - Windows GUI Application Entry Point
 * 
 * This is the entry point for all Windows GUI applications (console apps use main()).
 * The operating system calls this function when the user launches the .exe file.
 * 
 * WHY WE USE WinMain INSTEAD OF main():
 * - Windows GUI apps don't have a console by default
 * - WinMain receives GUI-specific parameters like window display state
 * - The linker flag -mwindows tells GCC to use WinMain instead of main()
 * 
 * @param hInstance     - Handle to current instance of the application
 *                        Used to load resources (icons, menus) and create windows
 * @param hPrevInstance - Always NULL in modern Windows (legacy parameter)
 *                        In 16-bit Windows, this detected if app was already running
 * @param lpCmdLine     - Command line arguments as a string (rarely used)
 *                        For complex parsing, use GetCommandLine() instead
 * @param nCmdShow      - How the window should be displayed initially
 *                        Values: SW_SHOW, SW_MINIMIZE, SW_MAXIMIZE, etc.
 * @return Exit code    - 0 = success, non-zero = error
 *                        This is what the OS sees as the program's exit code
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                   LPSTR lpCmdLine, int nCmdShow) {
    
    // ============================================
    // STEP 1: Initialize Windows Common Controls
    // ============================================
    // WHY: We use ListView and StatusBar controls which are part of the
    //      Common Controls library (comctl32.dll). We must initialize
    //      this library before creating any of these controls.
    // HOW: Fill in INITCOMMONCONTROLSEX structure and call InitCommonControlsEx()
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);  // Size validation for API
    icex.dwICC = ICC_LISTVIEW_CLASSES |           // Enable ListView control
                 ICC_BAR_CLASSES;                 // Enable StatusBar control
    InitCommonControlsEx(&icex);
    // NOTE: If this fails, ListView/StatusBar creation will fail later
    //       Most systems have this library, so we don't check for errors
    
    // ============================================
    // STEP 2: Create Main Application Window
    // ============================================
    // WHY: Every GUI app needs at least one window to show content
    // HOW: CreateMainWindow() registers a window class and creates the window
    HWND hwndMain = CreateMainWindow(hInstance);
    if (!hwndMain) {
        // Window creation failed - exit immediately
        // CreateMainWindow() already showed an error message to the user
        return 1;
    }
    
    // ============================================
    // STEP 3: Load Keyboard Shortcuts (Accelerators)
    // ============================================
    // WHY: Users expect keyboard shortcuts like F5 to work
    // HOW: Load accelerator table from resources (defined in HandleHunter.rc)
    //      The table maps VK_F5 to IDC_REFRESH_BTN, VK_DELETE to IDC_RELEASE_BTN
    HACCEL hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_ACCEL_REFRESH));
    // NOTE: If loading fails (hAccel is NULL), shortcuts won't work but app will run
    
    // ============================================
    // STEP 4: Show the Window
    // ============================================
    // WHY: Window exists but is hidden by default - we need to make it visible
    // HOW: ShowWindow() makes it visible, UpdateWindow() forces initial paint
    ShowWindow(hwndMain, nCmdShow);  // nCmdShow can be SW_MINIMIZE, SW_MAXIMIZE, etc.
    UpdateWindow(hwndMain);          // Forces WM_PAINT to draw the window immediately
    
    // ============================================
    // STEP 5: Main Message Loop
    // ============================================
    // WHY: Windows uses message-driven architecture. All user interactions
    //      (mouse clicks, keyboard, window resizing) are sent as messages.
    //      The message loop retrieves and processes these messages.
    // HOW: Loop forever until GetMessage returns 0 (WM_QUIT received)
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        // GetMessage() blocks until a message arrives in the queue
        // Returns 0 when WM_QUIT is received (app should exit)
        // Returns -1 on error (very rare, we ignore this case)
        
        // Try to handle as keyboard shortcut first
        // TranslateAccelerator checks if msg is in our accelerator table
        // Returns non-zero if it handled the message (converts to WM_COMMAND)
        if (!TranslateAccelerator(hwndMain, hAccel, &msg)) {
            // Not a shortcut - try dialog message handling
            // IsDialogMessage handles Tab key navigation between controls
            // WHY: Without this, Tab key wouldn't move focus between controls
            // Returns non-zero if it handled the message
            if (!IsDialogMessage(hwndMain, &msg)) {
                // Not a shortcut or dialog message - process normally
                // TranslateMessage converts WM_KEYDOWN to WM_CHAR (for text input)
                TranslateMessage(&msg);
                // DispatchMessage sends the message to the window procedure (MainWindowProc)
                DispatchMessage(&msg);
            }
        }
    }
    // Message loop exits when WM_QUIT is received (from PostQuitMessage)
    
    // Return the exit code (msg.wParam contains the value passed to PostQuitMessage)
    return (int)msg.wParam;
}
