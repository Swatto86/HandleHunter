/*
 * gui.c - GUI creation and window management
 * 
 * This file handles all user interface creation and message processing.
 * It contains functions for creating the main window, child controls,
 * and handling user interactions.
 * 
 * ARCHITECTURE OVERVIEW:
 * 1. CreateMainWindow()    - Registers window class and creates main window
 * 2. CreateControls()      - Creates all child controls (buttons, ListView, etc.)
 * 3. MainWindowProc()      - Processes all Windows messages (clicks, resize, etc.)
 * 4. Event handlers        - OnRefresh(), OnReleaseLock(), OnSearchChanged()
 * 
 * WINDOWS MESSAGE-DRIVEN ARCHITECTURE:
 * - Windows sends messages to MainWindowProc() for every user interaction
 * - We use a switch statement to handle different message types
 * - Each message type (WM_CREATE, WM_COMMAND, etc.) requires specific handling
 * 
 * STATE MANAGEMENT:
 * - AppState structure stores all window handles and data
 * - Stored in window's user data area via SetWindowLongPtr()
 * - Retrieved in message handlers via GetWindowLongPtr()
 * - WHY: Avoids global variables while keeping state accessible
 */

#include <windows.h>      // Core Windows API
#include <commctrl.h>     // Common Controls (ListView, StatusBar)
#include <stdio.h>        // swprintf for string formatting
#include <string.h>       // memset, wcslen, wcsstr for string operations
#include <uxtheme.h>      // SetWindowTheme for modern control styling
#include "resource.h"     // Control IDs (IDC_REFRESH_BTN, etc.)
#include "gui.h"          // Function declarations
#include "netapi.h"       // File lock enumeration and closing
#include "modern_ui.h"    // Dark mode and modern window styling

/**
 * CreateMainWindow - Create and register the main application window
 * 
 * This function performs two critical tasks:
 * 1. Register a window class (defines window behavior and appearance)
 * 2. Create an instance of that window class
 * 
 * WHY WE NEED TO REGISTER A WINDOW CLASS:
 * - Windows needs to know how to handle messages for our window
 * - The window class defines the window procedure (MainWindowProc)
 * - It also defines default cursor, background color, and icon
 * 
 * @param hInstance - Application instance (used to load icon and create window)
 * @return Window handle (HWND) on success, NULL on failure
 */
HWND CreateMainWindow(HINSTANCE hInstance) {
    // ============================================
    // STEP 1: Fill in the Window Class structure
    // ============================================
    // WHY: Windows needs metadata about our window before creating it
    WNDCLASSEXW wc = {0};  // Zero-initialize all fields
    
    wc.cbSize = sizeof(WNDCLASSEXW);    // Size of structure (for version detection)
    
    // CS_HREDRAW | CS_VREDRAW: Redraw entire window if width or height changes
    // WHY: Ensures controls reposition correctly when window is resized
    wc.style = CS_HREDRAW | CS_VREDRAW;
    
    // lpfnWndProc: Pointer to our window procedure function
    // WHY: This tells Windows which function to call for every message
    wc.lpfnWndProc = MainWindowProc;
    
    wc.hInstance = hInstance;  // Application instance (connects window to our app)
    
    // Default cursor when mouse is over our window
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);  // Standard arrow cursor
    
    // Background color brush (COLOR_WINDOW + 1 = standard window background)
    // WHY: Provides a clean background before controls are painted
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    
    // Class name (must be unique per application)
    // WHY: Used later to create windows of this class
    wc.lpszClassName = L"HandleHunterClass";
    
    // Application icons (large and small)
    // WHY: Shows in taskbar, alt+tab, and window title bar
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON));      // 32x32
    wc.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON));    // 16x16
    
    // ============================================
    // STEP 2: Register the window class
    // ============================================
    // WHY: Windows needs to store this metadata before we can create windows
    if (!RegisterClassExW(&wc)) {
        // Registration failed - show error and abort
        MessageBoxW(NULL, L"Window registration failed!", L"Error", MB_ICONERROR);
        return NULL;
    }
    
    // ============================================
    // STEP 3: Create the actual window
    // ============================================
    // CreateWindowExW creates a window instance of our registered class
    HWND hwnd = CreateWindowExW(
        0,                              // Extended window styles (none needed)
        L"HandleHunterClass",           // Class name (must match registered class)
        L"HandleHunter - Local File Locks",  // Window title (shown in title bar)
        WS_OVERLAPPEDWINDOW,            // Window style: standard window with title bar,
                                        // border, minimize/maximize/close buttons, resize
        CW_USEDEFAULT, CW_USEDEFAULT,   // X, Y position (let Windows decide)
        1000, 600,                      // Width, Height in pixels
        NULL,                           // Parent window (none - this is top-level)
        NULL,                           // Menu (none)
        hInstance,                      // Application instance
        NULL                            // Additional creation data (none)
    );
    // NOTE: CreateWindowExW sends WM_CREATE message to MainWindowProc
    
    if (!hwnd) {
        // Window creation failed - show error and abort
        MessageBoxW(NULL, L"Window creation failed!", L"Error", MB_ICONERROR);
        return NULL;
    }
    
    // ============================================
    // STEP 4: Apply modern Windows styling
    // ============================================
    // WHY: Makes the application look native to Windows 10/11
    
    // Enable dark mode title bar if Windows is in dark mode
    // This only affects the non-client area (title bar, borders)
    EnableDarkMode(hwnd);
    
    // Apply rounded corners on Windows 11
    ApplyModernWindowStyle(hwnd);
    
    return hwnd;  // Return window handle for use by caller
}

/**
 * CreateControls - Create all child controls inside the main window
 * 
 * This function creates and positions all UI elements:
 * - Search label and text box
 * - Refresh button
 * - ListView (table showing file locks)
 * - Release button
 * - Status bar
 * 
 * WHY WE CREATE CONTROLS IN A SEPARATE FUNCTION:
 * - Keeps CreateMainWindow() focused on window creation
 * - Makes code more maintainable and readable
 * - Can be called from WM_CREATE message handler
 * 
 * COORDINATE SYSTEM:
 * - (0,0) is top-left corner of parent window's client area
 * - X increases to the right, Y increases downward
 * - All positions in pixels
 * 
 * @param hwndParent - Handle to parent window (controls are children of this)
 * @param state      - Pointer to AppState (stores handles for later access)
 */
void CreateControls(HWND hwndParent, AppState* state) {
    // Get application instance from parent window
    // WHY: Needed to create child windows
    HINSTANCE hInst = (HINSTANCE)GetWindowLongPtr(hwndParent, GWLP_HINSTANCE);
    
    // Start Y position for controls (15 pixels from top)
    int y = 15;
    
    // ============================================
    // SECTION 1: Search Controls
    // ============================================
    // WHY: Allow users to filter the file list in real-time
    
    // Create "Search:" label
    HWND hwndSearchLabel = CreateWindowW(
        L"STATIC",                  // Control class (STATIC = text label)
        L"Search:",                 // Text to display
        WS_CHILD | WS_VISIBLE |     // Child of parent, visible immediately
        SS_LEFT,                    // Text alignment: left
        15, y + 3,                  // X, Y position (+3 for vertical alignment with edit box)
        60, 20,                     // Width, Height
        hwndParent,                 // Parent window
        NULL,                       // No menu/ID (label doesn't send commands)
        hInst,                      // Application instance
        NULL                        // No creation data
    );
    
    // Create a modern font for all controls
    // WHY: Segoe UI is the standard Windows font, gives modern appearance
    HFONT hFont = CreateFontW(
        19,                         // Height in pixels
        0,                          // Width (0 = auto-calculate based on height)
        0,                          // Escapement (text rotation angle)
        0,                          // Orientation (baseline rotation angle)
        FW_NORMAL,                  // Weight (400 = normal, 700 = bold)
        FALSE,                      // Italic
        FALSE,                      // Underline
        FALSE,                      // Strikeout
        DEFAULT_CHARSET,            // Character set
        OUT_DEFAULT_PRECIS,         // Output precision
        CLIP_DEFAULT_PRECIS,        // Clipping precision
        CLEARTYPE_QUALITY,          // Font quality (ClearType antialiasing)
        DEFAULT_PITCH | FF_DONTCARE,// Pitch and family
        L"Segoe UI"                 // Font name
    );
    // NOTE: Font handle is not stored, so it leaks on exit
    //       For a simple app this is acceptable (OS cleans up on exit)
    //       For long-running apps, store handle and call DeleteObject() on WM_DESTROY
    
    // Apply font to label
    SendMessage(hwndSearchLabel, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    // Create search text box (edit control)
    // WHY: Users type here to filter the file list
    state->hwndSearch = CreateWindowExW(
        WS_EX_CLIENTEDGE,           // Extended style: sunken 3D border
        L"EDIT",                    // Control class (EDIT = text input box)
        L"",                        // Initial text (empty)
        WS_CHILD | WS_VISIBLE |     // Child window, visible
        ES_AUTOHSCROLL |            // Auto-scroll horizontally if text is long
        WS_TABSTOP,                 // Can receive keyboard focus via Tab key
        85, y - 2,                  // X, Y position (-2 to align with label)
        300, 32,                    // Width, Height
        hwndParent,                 // Parent window
        (HMENU)IDC_SEARCH_EDIT,     // Control ID (used to identify in WM_COMMAND)
        hInst,                      // Application instance
        NULL                        // No creation data
    );
    // Store handle in AppState so we can access it later
    // WHY: Need to read text value when filtering, set focus, etc.
    SendMessage(state->hwndSearch, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    // Create Refresh button
    // WHY: Users click this to reload the file list from the server
    HWND hwndRefreshBtn = CreateWindowW(
        L"BUTTON",                  // Control class
        L"Refresh (F5)",            // Button text (shows F5 shortcut)
        WS_CHILD | WS_VISIBLE |     // Child window, visible
        BS_PUSHBUTTON |             // Button style: standard push button
        WS_TABSTOP,                 // Can receive keyboard focus
        395, y - 2,                 // X, Y position
        130, 32,                    // Width, Height
        hwndParent,                 // Parent window
        (HMENU)IDC_REFRESH_BTN,     // Control ID (used in WM_COMMAND handler)
        hInst,                      // Application instance
        NULL                        // No creation data
    );
    SendMessage(hwndRefreshBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    // Move down for next row of controls
    y += 40;
    
    // ============================================
    // SECTION 2: ListView (File List)
    // ============================================
    // WHY: Displays file locks in a table format with columns
    //      Users can select files to release locks
    
    // Get client area size (needed for dynamic positioning)
    RECT rcClient;
    GetClientRect(hwndParent, &rcClient);
    
    // Create ListView control
    state->hwndListView = CreateWindowW(
        WC_LISTVIEW,                // Control class (Windows built-in ListView)
        L"",                        // No text
        WS_CHILD | WS_VISIBLE |     // Child window, visible
        WS_BORDER |                 // Border around control
        WS_TABSTOP |                // Can receive keyboard focus
        LVS_REPORT |                // Report view (table with columns)
        LVS_SHOWSELALWAYS,          // Show selection even when control loses focus
        15, y,                      // X, Y position
        rcClient.right - 30,        // Width (full window minus margins)
        rcClient.bottom - y - 90,   // Height (fill remaining space, leave room for button/status)
        hwndParent,                 // Parent window
        (HMENU)IDC_LISTVIEW,        // Control ID
        hInst,                      // Application instance
        NULL                        // No creation data
    );
    
    // Apply modern theming to ListView
    // WHY: Uses Windows Explorer theme for professional appearance
    SubclassListViewForDarkMode(state->hwndListView);
    
    // Create and apply font for ListView
    HFONT hListFont = CreateFontW(
        17, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI"
    );
    SendMessage(state->hwndListView, WM_SETFONT, (WPARAM)hListFont, TRUE);
    
    // ============================================
    // SECTION 3: ListView Column Configuration
    // ============================================
    // WHY: ListView needs columns defined before adding items
    
    // Column structure (reused for each column)
    LVCOLUMNW col = {0};
    col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;  // Which fields are valid
    
    // Column 0: Invisible column (width 0) for proper centering
    // WHY: Win32 ListView requires an invisible first column to properly center content
    col.fmt = LVCFMT_LEFT;
    col.cx = 0;                     // Width 0 (invisible)
    col.pszText = L"";              // Empty text
    ListView_InsertColumn(state->hwndListView, 0, &col);
    
    // Set center alignment for all visible columns
    col.fmt = LVCFMT_CENTER;
    
    // Column 1: File Name
    col.cx = 200;                   // Width in pixels
    col.pszText = L"File Name";     // Column header text
    ListView_InsertColumn(state->hwndListView, 1, &col);
    
    // Column 2: Path
    col.cx = 350;
    col.pszText = L"Path";
    ListView_InsertColumn(state->hwndListView, 2, &col);
    
    // Column 3: User
    col.cx = 120;
    col.pszText = L"User";
    ListView_InsertColumn(state->hwndListView, 3, &col);
    
    // Column 4: Locks
    col.cx = 80;
    col.pszText = L"Locks";
    ListView_InsertColumn(state->hwndListView, 4, &col);
    
    // Set extended ListView styles
    // WHY: These make the ListView more user-friendly
    ListView_SetExtendedListViewStyle(state->hwndListView,
        LVS_EX_FULLROWSELECT |      // Select entire row (not just first column)
        LVS_EX_GRIDLINES |          // Show grid lines between rows/columns
        LVS_EX_DOUBLEBUFFER);       // Double-buffer to eliminate flicker
    
    // ============================================
    // SECTION 4: Action Button
    // ============================================
    // WHY: Users click this to release selected file locks
    
    // Position button on the right side of window
    int btnWidth = 220;
    int btnX = rcClient.right - btnWidth - 15;  // Right-align with 15px margin
    
    HWND hwndReleaseBtn = CreateWindowW(
        L"BUTTON",                  // Control class
        L"Release Selected Lock(s)",// Button text
        WS_CHILD | WS_VISIBLE |     // Child window, visible
        BS_PUSHBUTTON |             // Standard push button
        WS_TABSTOP,                 // Can receive keyboard focus
        btnX, rcClient.bottom - 72, // X, Y position (72px from bottom)
        btnWidth, 36,               // Width, Height
        hwndParent,                 // Parent window
        (HMENU)IDC_RELEASE_BTN,     // Control ID
        hInst,                      // Application instance
        NULL                        // No creation data
    );
    SendMessage(hwndReleaseBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    // ============================================
    // SECTION 5: Status Bar
    // ============================================
    // WHY: Shows status messages and count of files displayed
    
    state->hwndStatus = CreateWindowExW(
        0,                          // No extended styles
        STATUSCLASSNAMEW,           // Status bar control class (Windows built-in)
        L"Ready",                   // Initial text
        WS_CHILD | WS_VISIBLE |     // Child window, visible
        SBARS_SIZEGRIP,             // Show resize grip in bottom-right corner
        0, 0, 0, 0,                 // Position/size (status bar auto-positions itself)
        hwndParent,                 // Parent window
        (HMENU)IDC_STATUSBAR,       // Control ID
        hInst,                      // Application instance
        NULL                        // No creation data
    );
    // NOTE: Status bar automatically resizes to fill bottom of window
    
    // Create and apply font for status bar
    HFONT hStatusFont = CreateFontW(
        17, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI"
    );
    SendMessage(state->hwndStatus, WM_SETFONT, (WPARAM)hStatusFont, TRUE);
}

/**
 * UpdateListView - Populate ListView with file locks from AppState
 * 
 * This function refreshes the ListView to display the current file locks.
 * It applies the search filter if the user has typed in the search box.
 * 
 * WHY WE NEED THIS FUNCTION:
 * - ListView needs to be updated after refresh or search text changes
 * - Filtering logic is complex enough to warrant separate function
 * - Can be called multiple times without side effects
 * 
 * HOW FILTERING WORKS:
 * 1. Get search text from search box
 * 2. Convert to lowercase for case-insensitive matching
 * 3. For each lock, check if search text appears in filename, path, or username
 * 4. Only add matching items to ListView
 * 
 * @param state - Pointer to AppState (contains locks array and window handles)
 */
void UpdateListView(AppState* state) {
    // Validate inputs
    if (!state || !state->hwndListView) return;
    
    // ============================================
    // STEP 1: Clear existing ListView items
    // ============================================
    // WHY: We're repopulating from scratch, not appending
    ListView_DeleteAllItems(state->hwndListView);
    
    // ============================================
    // STEP 2: Get and normalize search text
    // ============================================
    WCHAR searchText[256] = L"";
    if (state->hwndSearch) {
        // Read text from search box
        GetWindowTextW(state->hwndSearch, searchText, 256);
        // Convert to lowercase for case-insensitive comparison
        // WHY: Users expect "test" to match "Test", "TEST", etc.
        _wcslwr(searchText);  // Modifies string in-place
    }
    
    // Check if filter is active (non-empty search text)
    BOOL hasFilter = (wcslen(searchText) > 0);
    
    // ============================================
    // STEP 3: Add matching items to ListView
    // ============================================
    for (DWORD i = 0; i < state->locks.count; i++) {
        FileLockInfo* lock = &state->locks.items[i];
        
        // Apply search filter if active
        if (hasFilter) {
            // Create lowercase copies of fields to search
            WCHAR lowerFilename[512], lowerPath[512], lowerUsername[256];
            wcsncpy(lowerFilename, lock->fileName, 256);
            wcsncpy(lowerPath, lock->filePath, MAX_PATH);
            wcsncpy(lowerUsername, lock->username, 256);
            _wcslwr(lowerFilename);
            _wcslwr(lowerPath);
            _wcslwr(lowerUsername);
            
            // Check if search text appears in any field
            // WHY: Search should check filename, path, AND username
            //      User might remember any of these details
            if (!wcsstr(lowerFilename, searchText) &&   // Not in filename
                !wcsstr(lowerPath, searchText) &&       // Not in path
                !wcsstr(lowerUsername, searchText)) {   // Not in username
                // No match - skip this item
                continue;
            }
        }
        
        // Item matches filter (or no filter) - add to ListView
        
        // Prepare item structure for insertion
        LVITEMW item = {0};
        item.mask = LVIF_TEXT | LVIF_PARAM;  // We're setting text and lParam
        item.iItem = ListView_GetItemCount(state->hwndListView);  // Add at end
        item.lParam = (LPARAM)lock->fileId;  // Store file ID for later (release lock)
        item.pszText = L"";                   // Text for column 0 (invisible column)
        
        // Insert the item
        int index = ListView_InsertItem(state->hwndListView, &item);
        // WHY: ListView_InsertItem returns the index where item was inserted
        //      We use this index to set text for other columns
        
        // Set text for all visible columns (columns 1-4, since 0 is invisible)
        ListView_SetItemText(state->hwndListView, index, 1, lock->fileName);  // Column 1: File Name
        ListView_SetItemText(state->hwndListView, index, 2, lock->filePath);  // Column 2: Path
        ListView_SetItemText(state->hwndListView, index, 3, lock->username);  // Column 3: User
        
        // Format lock count as string
        WCHAR lockCount[16];
        swprintf(lockCount, 16, L"%lu", lock->numLocks);
        ListView_SetItemText(state->hwndListView, index, 4, lockCount);  // Column 4: Locks
    }
    
    // ============================================
    // STEP 4: Update status bar with count
    // ============================================
    WCHAR statusText[256];
    swprintf(statusText, 256, L"Showing %d file(s)", ListView_GetItemCount(state->hwndListView));
    SetWindowTextW(state->hwndStatus, statusText);
    // WHY: Provides feedback about how many items are visible
    //      If filtered, user knows how many matches were found
}

/**
 * OnRefresh - Handle "Refresh" button click (or F5 key)
 * 
 * This function queries the local machine for all open files and updates the display.
 * 
 * WHY THIS IS SEPARATE FROM UpdateListView():
 * - OnRefresh fetches NEW data from the server
 * - UpdateListView displays EXISTING data from memory
 * - Separation of concerns: data fetching vs. display
 * 
 * FLOW:
 * 1. Show "Refreshing..." status
 * 2. Clear old data
 * 3. Call EnumerateOpenFiles() to get new data from Windows API
 * 4. Update ListView to show new data
 * 5. Show success/error message in status bar
 * 
 * @param hwnd  - Window handle (parent window)
 * @param state - Pointer to AppState (contains data and window handles)
 */
void OnRefresh(HWND hwnd, AppState* state) {
    if (!state) return;  // Safety check
    
    // ============================================
    // STEP 1: Update status bar
    // ============================================
    SetWindowTextW(state->hwndStatus, L"Refreshing...");
    // WHY: Provides immediate feedback that action was received
    
    // ============================================
    // STEP 2: Clear old lock data
    // ============================================
    // LockArray_Clear sets count to 0 but keeps memory allocated
    // WHY: Memory can be reused, avoiding reallocation overhead
    LockArray_Clear(&state->locks);
    
    // ============================================
    // STEP 3: Enumerate files on local machine
    // ============================================
    // NULL server name means "local machine"
    // WHY: This app only manages local file locks, not remote servers
    DWORD result = EnumerateOpenFiles(NULL, &state->locks);
    
    // ============================================
    // STEP 4: Handle result
    // ============================================
    if (result == 0) {
        // Success - update display
        UpdateListView(state);
        
        // Show count in status bar
        WCHAR statusText[256];
        swprintf(statusText, 256, L"Found %lu open file(s)", state->locks.count);
        SetWindowTextW(state->hwndStatus, statusText);
    } else {
        // Error occurred
        // WHY: Show error code to help diagnose issues
        //      Common error: 5 (Access Denied) = not running as Administrator
        WCHAR errorMsg[512];
        swprintf(errorMsg, 512, 
            L"Failed to enumerate files. Error code: %lu\n"
            L"Make sure you're running as Administrator.", 
            result);
        MessageBoxW(hwnd, errorMsg, L"Error", MB_ICONERROR);
        SetWindowTextW(state->hwndStatus, L"Error refreshing file list");
    }
}

/**
 * OnReleaseLock - Handle "Release Lock" button click or double-click on item
 * 
 * This function releases file locks for all selected items in the ListView.
 * It supports multi-selection (Ctrl+Click, Shift+Click).
 * 
 * WHY REQUIRE CONFIRMATION:
 * - Releasing a lock can cause data loss if file is being edited
 * - User might click accidentally
 * - Warning message helps prevent mistakes
 * 
 * FLOW:
 * 1. Count selected items (if 0, show message and return)
 * 2. Build confirmation message (shows filename or count)
 * 3. Show confirmation dialog
 * 4. If confirmed, iterate through selected items
 * 5. Call CloseFileLock() for each item
 * 6. Show success/failure message
 * 7. Refresh the list to reflect changes
 * 
 * @param hwnd  - Window handle (parent window)
 * @param state - Pointer to AppState (contains ListView handle)
 */
void OnReleaseLock(HWND hwnd, AppState* state) {
    if (!state || !state->hwndListView) return;  // Safety check
    
    // ============================================
    // STEP 1: Check if any items are selected
    // ============================================
    int selectedCount = ListView_GetSelectedCount(state->hwndListView);
    if (selectedCount == 0) {
        // No selection - inform user
        MessageBoxW(hwnd, 
            L"Please select one or more files to release.", 
            L"No Selection", 
            MB_ICONINFORMATION);
        return;
    }
    
    // ============================================
    // STEP 2: Build confirmation message
    // ============================================
    WCHAR confirmMsg[768];
    if (selectedCount == 1) {
        // Single selection - show filename in confirmation
        int selectedIndex = ListView_GetNextItem(state->hwndListView, -1, LVNI_SELECTED);
        WCHAR filename[512];
        ListView_GetItemText(state->hwndListView, selectedIndex, 1, filename, 512);  // Column 1: File Name
        swprintf(confirmMsg, 768, 
            L"Are you sure you want to release the lock on:\n\n%ls\n\n"
            L"Warning: This may cause unsaved data loss!", 
            filename);
    } else {
        // Multiple selection - show count
        swprintf(confirmMsg, 768, 
            L"Are you sure you want to release locks on %d selected files?\n\n"
            L"Warning: This may cause unsaved data loss!", 
            selectedCount);
    }
    
    // ============================================
    // STEP 3: Show confirmation dialog
    // ============================================
    int response = MessageBoxW(hwnd, confirmMsg, L"Confirm Release", 
                               MB_YESNO | MB_ICONWARNING);
    
    if (response == IDYES) {
        // User confirmed - proceed with release
        
        // Update status bar
        WCHAR statusMsg[256];
        swprintf(statusMsg, 256, L"Releasing %d lock(s)...", selectedCount);
        SetWindowTextW(state->hwndStatus, statusMsg);
        
        // Track success/failure counts
        int successCount = 0;
        int failCount = 0;
        
        // ============================================
        // STEP 4: Iterate through selected items
        // ============================================
        // Start at -1, ListView_GetNextItem finds first selected item
        int itemIndex = -1;
        
        while ((itemIndex = ListView_GetNextItem(state->hwndListView, itemIndex, LVNI_SELECTED)) != -1) {
            // Get item data
            LVITEMW item = {0};
            item.mask = LVIF_PARAM;  // We want the lParam field
            item.iItem = itemIndex;
            ListView_GetItem(state->hwndListView, &item);
            
            // Extract file ID (stored in lParam during UpdateListView)
            DWORD fileId = (DWORD)item.lParam;
            
            // ============================================
            // STEP 5: Call NetFileClose to release the lock
            // ============================================
            // NULL server name = local machine
            DWORD result = CloseFileLock(NULL, fileId);
            
            if (result == 0) {
                successCount++;  // Success
            } else {
                failCount++;     // Failure
                // Note: We continue trying other files even if one fails
            }
        }
        
        // ============================================
        // STEP 6: Show results to user
        // ============================================
        WCHAR resultMsg[512];
        if (failCount == 0) {
            // All succeeded
            swprintf(resultMsg, 512, L"Successfully released %d lock(s)!", successCount);
            MessageBoxW(hwnd, resultMsg, L"Success", MB_ICONINFORMATION);
        } else {
            // Some or all failed
            swprintf(resultMsg, 512, 
                L"Released %d lock(s).\nFailed to release %d lock(s).", 
                successCount, failCount);
            MessageBoxW(hwnd, resultMsg, L"Partial Success", MB_ICONWARNING);
        }
        
        // ============================================
        // STEP 7: Refresh to show updated state
        // ============================================
        // WHY: Locks we released should no longer appear in the list
        OnRefresh(hwnd, state);
    }
    // If user clicked "No", we do nothing
}

/**
 * OnSearchChanged - Handle search text box change event
 * 
 * This function is called every time the user types in the search box.
 * It filters the ListView to show only matching items.
 * 
 * WHY THIS IS SO SIMPLE:
 * - All filtering logic is in UpdateListView()
 * - This function just triggers a refresh
 * - Separation of concerns: event handler vs. display logic
 * 
 * @param hwnd  - Window handle (not used, but required by callback signature)
 * @param state - Pointer to AppState (passed to UpdateListView)
 */
void OnSearchChanged(HWND hwnd, AppState* state) {
    // Simply update the display - UpdateListView() reads search box
    // and applies filtering
    UpdateListView(state);
}

/**
 * MainWindowProc - Main window procedure (message handler)
 * 
 * This is the most important function in a Windows application.
 * Windows calls this function for EVERY event related to our window:
 * - Window creation/destruction
 * - Button clicks
 * - Keyboard input
 * - Mouse events
 * - Window resize
 * - etc.
 * 
 * WINDOWS MESSAGE-DRIVEN ARCHITECTURE:
 * - Windows doesn't call our functions directly
 * - Instead, it sends "messages" (events) to our window
 * - Each message has an ID (msg parameter) and parameters (wParam, lParam)
 * - We use a switch statement to handle different message types
 * - Messages we don't handle go to DefWindowProc for default behavior
 * 
 * WHY USE A STATIC VARIABLE FOR STATE:
 * - Window procedures can't have custom parameters
 * - Static variable persists across function calls
 * - We also store it in window's user data for redundancy
 * - WHY: Ensures state is available in all message handlers
 * 
 * @param hwnd   - Handle to the window receiving the message
 * @param msg    - Message identifier (WM_CREATE, WM_COMMAND, etc.)
 * @param wParam - First message parameter (meaning varies by message)
 * @param lParam - Second message parameter (meaning varies by message)
 * @return Result depends on message type (usually 0 = handled)
 */
LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // Static variable to hold application state across all message handlers
    // WHY STATIC: Regular local variables would be lost between function calls
    //             Static variables persist for lifetime of program
    static AppState* state = NULL;
    
    // ============================================
    // MESSAGE DISPATCHER - Route message to handler
    // ============================================
    switch (msg) {
        // ========================================
        // WM_CREATE - Window is being created
        // ========================================
        // This is the FIRST message sent to a new window
        // Perfect place to allocate memory and create child controls
        case WM_CREATE: {
            // ============================================
            // STEP 1: Allocate memory for application state
            // ============================================
            state = (AppState*)malloc(sizeof(AppState));
            if (!state) {
                // Allocation failed - show error and abort window creation
                MessageBoxW(hwnd, 
                    L"Failed to allocate memory for AppState", 
                    L"Error", 
                    MB_ICONERROR);
                return -1;  // Non-zero return = abort window creation
            }
            
            // ============================================
            // STEP 2: Initialize state structure
            // ============================================
            // Zero all fields to ensure no garbage data
            memset(state, 0, sizeof(AppState));
            
            // Initialize the dynamic array for file locks
            LockArray_Init(&state->locks);
            
            // No server field needed (always local machine)
            state->hwndServer = NULL;
            
            // ============================================
            // STEP 2a: Initialize theme state
            // ============================================
            // Get current theme colors and create background brush
            GetThemeColors(&state->themeColors);
            state->hBackgroundBrush = CreateSolidBrush(state->themeColors.background);
            
            // Store main window handle for later access
            state->hwndMain = hwnd;
            
            // ============================================
            // STEP 3: Store state in window's user data
            // ============================================
            // WHY: Allows retrieving state later without static variable
            //      Useful if we ever need multiple windows
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)state);
            
            // ============================================
            // STEP 4: Create all child controls
            // ============================================
            CreateControls(hwnd, state);
            
            // ============================================
            // STEP 5: Auto-refresh on startup
            // ============================================
            // Post a message to trigger refresh after window is fully created
            // WHY PostMessage instead of calling OnRefresh directly:
            //   - Window isn't fully initialized yet during WM_CREATE
            //   - PostMessage queues the command for later, after WM_CREATE completes
            //   - Ensures all initialization is done before refresh runs
            PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_REFRESH_BTN, BN_CLICKED), 0);
            
            return 0;  // 0 = success, continue with window creation
        }
        
        // ========================================
        // WM_SIZE - Window is being resized
        // ========================================
        // Sent when user resizes window or maximizes/minimizes
        // We need to reposition/resize controls to fit new size
        case WM_SIZE: {
            // Only process if state is initialized
            if (state && state->hwndStatus) {
                // ============================================
                // STEP 1: Resize status bar
                // ============================================
                // Status bar auto-adjusts to window width when sent WM_SIZE
                SendMessage(state->hwndStatus, WM_SIZE, 0, 0);
                
                // ============================================
                // STEP 2: Get new window dimensions
                // ============================================
                RECT rcClient;
                GetClientRect(hwnd, &rcClient);
                // rcClient.right = new width, rcClient.bottom = new height
                
                // ============================================
                // STEP 3: Resize ListView to fill available space
                // ============================================
                if (state->hwndListView) {
                    SetWindowPos(state->hwndListView, 
                        NULL,                           // No Z-order change
                        15, 58,                         // X, Y (fixed position)
                        rcClient.right - 30,            // Width (fill window minus margins)
                        rcClient.bottom - 140,          // Height (fill, leave room for button/status)
                        SWP_NOZORDER);                  // Don't change Z-order
                }
                
                // ============================================
                // STEP 4: Reposition Release button (keep right-aligned)
                // ============================================
                HWND hwndReleaseBtn = GetDlgItem(hwnd, IDC_RELEASE_BTN);
                if (hwndReleaseBtn) {
                    int btnWidth = 220;
                    // Calculate X position to keep button right-aligned
                    int btnX = rcClient.right - btnWidth - 15;
                    SetWindowPos(hwndReleaseBtn, 
                        NULL,                           // No Z-order change
                        btnX, rcClient.bottom - 72,     // New position
                        btnWidth, 36,                   // Size (unchanged)
                        SWP_NOZORDER);                  // Don't change Z-order
                }
            }
            return 0;
        }
        
        // ========================================
        // WM_COMMAND - Control notification or menu command
        // ========================================
        // Sent when user clicks button, types in edit box, selects menu item, etc.
        // wParam contains control ID and notification code
        case WM_COMMAND: {
            // Extract control ID and notification code from wParam
            // WHY: wParam packs two values using LOWORD/HIWORD macros
            int controlId = LOWORD(wParam);   // Which control sent the message
            int notifyCode = HIWORD(wParam);  // What happened (clicked, changed, etc.)
            
            // Route to appropriate handler based on control ID
            switch (controlId) {
                // ========================================
                // Refresh Button or F5 Key
                // ========================================
                case IDC_REFRESH_BTN:
                    // Handle both button clicks (BN_CLICKED=0) and accelerator commands (notifyCode=1)
                    // WHY: Accelerator sends notifyCode=1, button sends BN_CLICKED=0
                    if (notifyCode == BN_CLICKED || notifyCode == 1) {
                        OnRefresh(hwnd, state);
                    }
                    break;
                    
                // ========================================
                // Release Button or Delete Key
                // ========================================
                case IDC_RELEASE_BTN:
                    // Handle both button clicks and accelerator commands (Del key)
                    if (notifyCode == BN_CLICKED || notifyCode == 1) {
                        OnReleaseLock(hwnd, state);
                    }
                    break;
                    
                // ========================================
                // Search Edit Box
                // ========================================
                case IDC_SEARCH_EDIT:
                    // EN_CHANGE = text changed (user typed something)
                    if (notifyCode == EN_CHANGE) {
                        OnSearchChanged(hwnd, state);
                    }
                    break;
            }
            
            return 0;
        }
        
        // ========================================
        // WM_NOTIFY - Complex control notification
        // ========================================
        // Sent by ListView for events like double-click, selection change, etc.
        // More complex than WM_COMMAND, uses NMHDR structure
        case WM_NOTIFY: {
            // Cast lParam to NMHDR to access notification header
            // WHY: WM_NOTIFY uses NMHDR structure for all notifications
            //      It contains info about which control and what event
            LPNMHDR pnmh = (LPNMHDR)lParam;
            
            // Check if it's from our ListView and is a double-click
            if (pnmh->idFrom == IDC_LISTVIEW && pnmh->code == NM_DBLCLK) {
                // User double-clicked an item - release the lock
                // WHY: Double-click is a natural action for "do something with this item"
                OnReleaseLock(hwnd, state);
            }
            return 0;
        }
        
        // ========================================
        // WM_DESTROY - Window is being destroyed
        // ========================================
        // This is the LAST message sent before window is destroyed
        // Perfect place to free memory and clean up resources
        case WM_DESTROY: {
            // ============================================
            // Clean up resources before exit
            // ============================================
            if (state) {
                // Free the dynamic array memory
                // WHY: Prevents memory leak when application exits
                LockArray_Free(&state->locks);
                
                // Delete background brush
                // WHY: GDI objects must be explicitly deleted to prevent resource leak
                if (state->hBackgroundBrush) {
                    DeleteObject(state->hBackgroundBrush);
                    state->hBackgroundBrush = NULL;
                }
                
                // Free the AppState structure itself
                free(state);
                state = NULL;  // Prevent use-after-free bugs
            }
            
            // ============================================
            // Tell Windows to exit the message loop
            // ============================================
            // PostQuitMessage sends WM_QUIT to message loop
            // WHY: This is how we exit the application
            //      GetMessage returns 0 when WM_QUIT is received
            PostQuitMessage(0);  // 0 = exit code (success)
            return 0;
        }
        
        // ========================================
        // WM_SETTINGCHANGE - System settings changed
        // ========================================
        // Sent when user changes system settings, including theme
        // This allows app to update when user switches between light/dark mode
        case WM_SETTINGCHANGE: {
            // Check if this is a theme/immersive color change
            // lParam contains the area that changed (can be NULL)
            if (lParam && (wcscmp((LPCWSTR)lParam, L"ImmersiveColorSet") == 0)) {
                if (state) {
                    // ============================================
                    // Theme changed - update colors and repaint
                    // ============================================
                    
                    // Delete old background brush
                    if (state->hBackgroundBrush) {
                        DeleteObject(state->hBackgroundBrush);
                    }
                    
                    // Get new theme colors
                    GetThemeColors(&state->themeColors);
                    
                    // Create new background brush
                    state->hBackgroundBrush = CreateSolidBrush(state->themeColors.background);
                    
                    // Apply theme changes to window
                    ApplyThemeToWindow(hwnd);
                }
            }
            return 0;
        }
        
        // ========================================
        // WM_CTLCOLORSTATIC - Color static controls (labels)
        // ========================================
        // Sent before drawing static controls (labels)
        // We customize colors to match dark/light theme
        case WM_CTLCOLORSTATIC: {
            if (state) {
                HDC hdc = (HDC)wParam;
                
                // Set text color to match theme
                SetTextColor(hdc, state->themeColors.text);
                
                // Set background color to match theme
                SetBkColor(hdc, state->themeColors.background);
                
                // Return brush for background
                // WHY: Windows uses this to paint control background
                return (LRESULT)state->hBackgroundBrush;
            }
            break;  // Fall through to default if no state
        }
        
        // ========================================
        // WM_CTLCOLOREDIT - Color edit controls
        // ========================================
        // Sent before drawing edit controls (text boxes)
        // We customize colors to match dark/light theme
        case WM_CTLCOLOREDIT: {
            if (state) {
                HDC hdc = (HDC)wParam;
                
                // Set text color for edit controls
                SetTextColor(hdc, state->themeColors.editText);
                
                // Set background color for edit controls
                SetBkColor(hdc, state->themeColors.editBg);
                
                // Return brush for edit control background
                return (LRESULT)state->hBackgroundBrush;
            }
            break;  // Fall through to default if no state
        }
        
        // ========================================
        // DEFAULT - All other messages
        // ========================================
        // For messages we don't handle, let Windows provide default behavior
        // WHY: Windows has default handlers for resize, minimize, maximize, etc.
        //      We only override the messages we care about
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}
