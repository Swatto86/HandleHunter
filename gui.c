/*
 * gui.c - GUI creation and window management
 * 
 * This file handles all user interface creation and message processing.
 * It creates the main window, child controls (buttons, edit boxes, ListView),
 * and processes Windows messages.
 */

#include <windows.h>    // Windows API
#include <commctrl.h>   // Common controls (ListView, StatusBar)
#include <stdio.h>       // Standard I/O (for swprintf)
#include <string.h>      // String functions (for wcsncpy, wcsstr, _wcslwr)
#include "resource.h"     // Control ID definitions
#include "gui.h"          // AppState structure
#include "netapi.h"       // Network API wrappers

// Link against common controls library (required for ListView, StatusBar)
#pragma comment(lib, "comctl32.lib")

// Control ID definitions
// These IDs are used to identify controls in WM_COMMAND messages
#define IDC_SERVER_EDIT      1001    // Server name input box
#define IDC_SEARCH_EDIT      1002    // Search filter input box
#define IDC_LISTVIEW         1003    // ListView showing file locks
#define IDC_CONNECT_BTN      1004    // Connect button
#define IDC_REFRESH_BTN      1005    // Refresh button
#define IDC_RELEASE_BTN      1006    // Release lock button
#define IDC_STATUSBAR        1007    // Status bar at bottom

/**
 * Create and register the main application window
 * 
 * @param hInstance - Application instance handle
 * @return Window handle on success, NULL on failure
 * 
 * This function registers a window class and creates the main window.
 * The window is created but not shown (call ShowWindow() after this).
 */
HWND CreateMainWindow(HINSTANCE hInstance) {
    // Define window class structure
    // WNDCLASSEXW is the extended version (supports small icon)
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);                    // Structure size (required)
    wc.style = CS_HREDRAW | CS_VREDRAW;                  // Redraw on horizontal/vertical resize
    wc.lpfnWndProc = MainWindowProc;                     // Pointer to window procedure (message handler)
    wc.hInstance = hInstance;                            // Application instance
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);            // Default arrow cursor
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);       // System window background color
    wc.lpszClassName = L"HandleHunterClass";            // Unique class name
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);          // Default application icon
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);        // Small icon for taskbar
    
    // Register the window class with Windows
    // This must be done before creating any windows of this class
    if (!RegisterClassExW(&wc)) {
        MessageBoxW(NULL, L"Window registration failed!", L"Error", MB_ICONERROR);
        return NULL;
    }
    
    // Create the main window using the registered class
    HWND hwnd = CreateWindowExW(
        0,                              // Extended window style (none)
        L"HandleHunterClass",          // Class name (must match registered class)
        L"HandleHunter",               // Window title (shown in title bar)
        WS_OVERLAPPEDWINDOW,           // Window style (has title bar, min/max buttons, etc.)
        CW_USEDEFAULT,                 // X position (let Windows choose)
        CW_USEDEFAULT,                 // Y position (let Windows choose)
        1000,                          // Window width in pixels
        600,                           // Window height in pixels
        NULL,                          // Parent window (NULL = top-level)
        NULL,                          // Menu handle (NULL = no menu)
        hInstance,                     // Application instance
        NULL                           // Additional creation data (NULL = none)
    );
    
    // Check if window creation succeeded
    if (!hwnd) {
        MessageBoxW(NULL, L"Window creation failed!", L"Error", MB_ICONERROR);
        return NULL;
    }
    
    return hwnd;  // Return window handle
}

/**
 * Create all child controls (buttons, edit boxes, ListView, etc.)
 * 
 * @param hwndParent - Handle to parent window
 * @param state      - Pointer to application state (to store control handles)
 * 
 * This function creates all the UI controls that appear inside the main window.
 * Control handles are stored in the AppState structure for later use.
 */
void CreateControls(HWND hwndParent, AppState* state) {
    // Get the instance handle from the parent window
    // Needed for creating child windows
    HINSTANCE hInst = (HINSTANCE)GetWindowLongPtr(hwndParent, GWLP_HINSTANCE);
    
    // Y coordinate for positioning controls (starts at top)
    int y = 10;
    
    // ============================================
    // Server Connection Section (top of window)
    // ============================================
    
    // Label: "Server:"
    CreateWindowW(L"STATIC",                    // Static text control
        L"Server:",                             // Text to display
        WS_CHILD | WS_VISIBLE,                  // Styles: child window, visible
        10, y + 3,                              // X, Y position (offset Y by 3 for alignment)
        60, 20,                                 // Width, Height
        hwndParent,                             // Parent window
        NULL,                                   // No menu/ID needed for static text
        hInst,                                  // Instance handle
        NULL);                                  // No creation data
    
    // Edit box: Server name input
    state->hwndServer = CreateWindowW(L"EDIT",  // Edit control class
        L"",                                    // Initial text (empty)
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,  // Styles
        80, y,                                  // Position (to right of label)
        300, 24,                                // Width, Height
        hwndParent,                             // Parent
        (HMENU)IDC_SERVER_EDIT,                // Control ID (for WM_COMMAND)
        hInst,                                  // Instance
        NULL);                                  // No creation data
    
    // Button: "Connect"
    CreateWindowW(L"BUTTON",                    // Button control class
        L"Connect",                             // Button text
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, // Styles: child, visible, push button
        390, y,                                 // Position (to right of edit box)
        80, 24,                                 // Width, Height
        hwndParent,                             // Parent
        (HMENU)IDC_CONNECT_BTN,                // Control ID
        hInst,                                  // Instance
        NULL);                                  // No creation data
    
    // Button: "Refresh"
    CreateWindowW(L"BUTTON",
        L"Refresh",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        480, y,                                 // Position (to right of Connect button)
        80, 24,
        hwndParent,
        (HMENU)IDC_REFRESH_BTN,
        hInst,
        NULL);
    
    // Move Y coordinate down for next row of controls
    y += 35;
    
    // ============================================
    // Search Section
    // ============================================
    
    // Label: "Search:"
    CreateWindowW(L"STATIC",
        L"Search:",
        WS_CHILD | WS_VISIBLE,
        10, y + 3,                              // Same X as Server label
        60, 20,
        hwndParent,
        NULL,
        hInst,
        NULL);
    
    // Edit box: Search filter input
    state->hwndSearch = CreateWindowW(L"EDIT",
        L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        80, y,                                  // Same X as Server edit box
        300, 24,
        hwndParent,
        (HMENU)IDC_SEARCH_EDIT,
        hInst,
        NULL);
    
    // Move Y coordinate down for ListView
    y += 35;
    
    // ============================================
    // ListView Section (main content area)
    // ============================================
    
    // Get client area dimensions to size ListView properly
    RECT rcClient;
    GetClientRect(hwndParent, &rcClient);
    
    // Create ListView control (the main data display)
    state->hwndListView = CreateWindowW(WC_LISTVIEW,  // ListView class name
        L"",                                    // No text (ListView doesn't use it)
        WS_CHILD | WS_VISIBLE | WS_BORDER |    // Basic styles
        LVS_REPORT |                            // Report view (like a table)
        LVS_SINGLESEL |                        // Only one item can be selected
        LVS_SHOWSELALWAYS,                     // Keep selection visible
        10, y,                                  // Position (left margin, below search)
        rcClient.right - 20,                   // Width (full width minus margins)
        rcClient.bottom - y - 80,              // Height (fill remaining space)
        hwndParent,                             // Parent
        (HMENU)IDC_LISTVIEW,                   // Control ID
        hInst,                                  // Instance
        NULL);                                  // No creation data
    
    // Configure ListView columns
    // Each column represents one piece of data (filename, path, user, etc.)
    LVCOLUMNW col = {0};
    col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;  // We're setting text, width, and format
    col.fmt = LVCFMT_LEFT;                          // Left-align text
    
    // Column 0: File Name
    col.cx = 180;                                   // Column width in pixels
    col.pszText = L"File Name";                    // Column header text
    ListView_InsertColumn(state->hwndListView, 0, &col);
    
    // Column 1: Full Path
    col.cx = 300;
    col.pszText = L"Path";
    ListView_InsertColumn(state->hwndListView, 1, &col);
    
    // Column 2: Username
    col.cx = 120;
    col.pszText = L"User";
    ListView_InsertColumn(state->hwndListView, 2, &col);
    
    // Column 3: Server Name
    col.cx = 100;
    col.pszText = L"Server";
    ListView_InsertColumn(state->hwndListView, 3, &col);
    
    // Column 4: Number of Locks
    col.cx = 60;
    col.pszText = L"Locks";
    ListView_InsertColumn(state->hwndListView, 4, &col);
    
    // Set extended ListView styles for better appearance
    ListView_SetExtendedListViewStyle(state->hwndListView,
        LVS_EX_FULLROWSELECT |    // Select entire row, not just first column
        LVS_EX_GRIDLINES |        // Show grid lines between rows/columns
        LVS_EX_DOUBLEBUFFER);      // Smooth scrolling (reduces flicker)
    
    // ============================================
    // Action Button Section (bottom)
    // ============================================
    
    // Button: "Release Selected Lock"
    CreateWindowW(L"BUTTON",
        L"Release Selected Lock",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        rcClient.right - 180,                   // Position at right edge
        rcClient.bottom - 60,                   // Position near bottom
        160, 30,                                // Width, Height
        hwndParent,
        (HMENU)IDC_RELEASE_BTN,
        hInst,
        NULL);
    
    // ============================================
    // Status Bar (very bottom of window)
    // ============================================
    
    // Create status bar control
    state->hwndStatus = CreateWindowW(STATUSCLASSNAME,  // Status bar class name
        L"Ready",                               // Initial text
        WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, // Styles: child, visible, resize grip
        0, 0,                                   // Position (status bar positions itself)
        0, 0,                                   // Size (status bar sizes itself)
        hwndParent,
        (HMENU)IDC_STATUSBAR,
        hInst,
        NULL);
}

/**
 * Update the ListView with current lock data (with filtering)
 * 
 * @param state - Pointer to application state
 * 
 * This function:
 * 1. Clears the ListView
 * 2. Reads the search filter text
 * 3. Iterates through all locks
 * 4. Filters based on search text (if any)
 * 5. Adds matching items to the ListView
 * 6. Updates the status bar with count information
 */
void UpdateListView(AppState* state) {
    // Clear all existing items from the ListView
    // This is faster than removing items one by one
    ListView_DeleteAllItems(state->hwndListView);
    
    // Read the current search filter text from the search edit box
    GetWindowTextW(state->hwndSearch, state->searchText, 256);
    
    // Counter for items actually displayed (after filtering)
    int displayCount = 0;
    
    // Iterate through all locks in the array
    for (size_t i = 0; i < state->locks.count; i++) {
        // Get pointer to current lock
        FileLockInfo* lock = &state->locks.items[i];
        
        // ============================================
        // Apply search filter (if user entered search text)
        // ============================================
        if (state->searchText[0] != L'\0') {
            // User has entered search text - filter results
            // We'll do case-insensitive substring matching
            
            // Create lowercase copies for comparison
            // (We don't modify originals, just create temp copies)
            WCHAR searchLower[256];
            WCHAR fileNameLower[256];
            WCHAR pathLower[256];
            WCHAR userLower[256];
            
            // Copy strings to temp buffers
            wcsncpy(searchLower, state->searchText, 255);
            searchLower[255] = L'\0';
            wcsncpy(fileNameLower, lock->fileName, 255);
            fileNameLower[255] = L'\0';
            wcsncpy(pathLower, lock->filePath, 255);
            pathLower[255] = L'\0';
            wcsncpy(userLower, lock->username, 255);
            userLower[255] = L'\0';
            
            // Convert to lowercase for case-insensitive comparison
            _wcslwr(searchLower);      // Convert search text to lowercase
            _wcslwr(fileNameLower);    // Convert filename to lowercase
            _wcslwr(pathLower);        // Convert path to lowercase
            _wcslwr(userLower);        // Convert username to lowercase
            
            // Check if search text appears in filename, path, or username
            // If it doesn't match any of these, skip this item
            if (!wcsstr(fileNameLower, searchLower) &&
                !wcsstr(pathLower, searchLower) &&
                !wcsstr(userLower, searchLower)) {
                continue;  // Skip this item - doesn't match filter
            }
        }
        // If no search text, show all items
        
        // ============================================
        // Add item to ListView
        // ============================================
        
        // Prepare ListView item structure
        LVITEMW item = {0};
        item.mask = LVIF_TEXT | LVIF_PARAM;     // We're setting text and lParam
        item.iItem = displayCount;               // Row index (0-based)
        item.lParam = i;                        // Store array index in lParam
                                                 // (We'll use this to get the lock data later)
        
        // Column 0: File Name
        // This is the primary column - set it when inserting the item
        item.pszText = lock->fileName;          // Text to display
        ListView_InsertItem(state->hwndListView, &item);
        
        // Column 1: Full Path
        // Set text for additional columns after item is inserted
        ListView_SetItemText(state->hwndListView, displayCount, 1, lock->filePath);
        
        // Column 2: Username
        ListView_SetItemText(state->hwndListView, displayCount, 2, lock->username);
        
        // Column 3: Server Name
        ListView_SetItemText(state->hwndListView, displayCount, 3, lock->serverName);
        
        // Column 4: Number of Locks (convert number to string)
        WCHAR lockCount[32];
        swprintf(lockCount, 32, L"%u", lock->numLocks);
        ListView_SetItemText(state->hwndListView, displayCount, 4, lockCount);
        
        // Increment display counter (only items that pass filter are counted)
        displayCount++;
    }
    
    // ============================================
    // Update status bar with count information
    // ============================================
    WCHAR statusText[512];
    swprintf(statusText, 512, 
        L"Showing %d of %d files",              // Format: "Showing X of Y files"
        displayCount,                           // Number displayed (after filter)
        (int)state->locks.count);               // Total number available
    SetWindowTextW(state->hwndStatus, statusText);
}

/**
 * Main Window Procedure - Handles all messages for the main window
 * 
 * @param hwnd   - Handle to the window receiving the message
 * @param msg    - Message identifier (WM_CREATE, WM_COMMAND, etc.)
 * @param wParam - Additional message-specific information
 * @param lParam - Additional message-specific information
 * @return Result code (0 = handled, non-zero = pass to DefWindowProc)
 * 
 * This function is called by Windows whenever a message needs to be processed
 * for our main window. It's the central message dispatcher for the application.
 */
LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // Retrieve application state from window's user data
    // We store a pointer to AppState here so we can access it in any message handler
    AppState* state = (AppState*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    
    // Process different message types
    switch (msg) {
        // ============================================
        // WM_CREATE - Window is being created
        // ============================================
        case WM_CREATE: {
            // Allocate memory for application state structure
            // calloc() zeros the memory, so all fields start at 0/NULL
            state = (AppState*)calloc(1, sizeof(AppState));
            if (!state) {
                // Memory allocation failed - abort window creation
                return -1;
            }
            
            // Store window handle in state for easy access
            state->hwndMain = hwnd;
            
            // Initialize the lock array (allocates initial memory)
            LockArray_Init(&state->locks);
            
            // Create all child controls (buttons, edit boxes, ListView, etc.)
            CreateControls(hwnd, state);
            
            // Store state pointer in window's user data
            // This allows us to retrieve it in other message handlers
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)state);
            
            return 0;  // Window creation successful
        }
        
        // ============================================
        // WM_COMMAND - Button clicks, menu selections, etc.
        // ============================================
        case WM_COMMAND: {
            // Safety check: retrieve state if not already available
            // (Shouldn't happen, but protects against edge cases)
            if (!state) {
                state = (AppState*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
            }
            if (!state) {
                // No state available - pass to default handler
                return DefWindowProc(hwnd, msg, wParam, lParam);
            }
            
            // Extract control ID and notification code from wParam
            int wmId = LOWORD(wParam);      // Control ID (which button/control)
            int wmEvent = HIWORD(wParam);   // Notification code (click, change, etc.)
            
            // Route to appropriate handler based on control ID
            switch (wmId) {
                case IDC_CONNECT_BTN:
                    // "Connect" button was clicked
                    OnConnect(hwnd, state);
                    break;
                    
                case IDC_REFRESH_BTN:
                    // "Refresh" button was clicked
                    OnRefresh(hwnd, state);
                    break;
                    
                case IDC_RELEASE_BTN:
                    // "Release Selected Lock" button was clicked
                    OnReleaseLock(hwnd, state);
                    break;
                    
                case IDC_SEARCH_EDIT:
                    // Search edit box notification
                    if (wmEvent == EN_CHANGE) {
                        // Text in search box changed - update filter
                        OnSearchChanged(hwnd, state);
                    }
                    break;
                    
                // Add more control handlers here as needed
            }
            return 0;  // Message handled
        }
        
        // ============================================
        // WM_SIZE - Window is being resized
        // ============================================
        case WM_SIZE: {
            // Window size changed - resize child controls to fit
            if (state) {
                // Status bar automatically resizes itself when it receives WM_SIZE
                SendMessage(state->hwndStatus, WM_SIZE, 0, 0);
                
                // TODO: Resize ListView and buttons to fit new window size
                // This would involve:
                // 1. Get new client area size
                // 2. Calculate new positions/sizes for controls
                // 3. Call SetWindowPos() for each control
            }
            return 0;
        }
        
        // ============================================
        // WM_NOTIFY - Notification messages from controls
        // ============================================
        case WM_NOTIFY: {
            // Retrieve state if needed
            if (!state) {
                state = (AppState*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
            }
            if (!state) {
                return DefWindowProc(hwnd, msg, wParam, lParam);
            }
            
            // Get notification header (contains control ID and notification code)
            LPNMHDR pnmhdr = (LPNMHDR)lParam;
            
            // Check which control sent the notification
            if (pnmhdr->idFrom == IDC_LISTVIEW) {
                // ListView sent a notification
                if (pnmhdr->code == NM_DBLCLK) {
                    // User double-clicked on a ListView item
                    // Treat this as "release lock" action
                    OnReleaseLock(hwnd, state);
                }
                // Could handle other ListView notifications here:
                // - NM_CLICK: single click
                // - LVN_ITEMCHANGED: selection changed
                // - etc.
            }
            return 0;
        }
        
        // ============================================
        // WM_DESTROY - Window is being destroyed
        // ============================================
        case WM_DESTROY: {
            // Cleanup: free all allocated memory
            if (state) {
                // Free the lock array memory
                LockArray_Free(&state->locks);
                
                // Free the application state structure
                free(state);
                
                // Clear the pointer in window data (prevent use-after-free)
                SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
            }
            
            // Post quit message to exit the message loop
            // The wParam (0) becomes the exit code
            PostQuitMessage(0);
            return 0;
        }
        
        // ============================================
        // Default: Let Windows handle unhandled messages
        // ============================================
        default:
            // Pass unhandled messages to default window procedure
            // This handles standard Windows behavior (resizing, painting, etc.)
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

