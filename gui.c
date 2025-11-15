/*
 * gui.c - GUI creation and window management
 * 
 * This file handles all user interface creation and message processing.
 */

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <string.h>
#include <uxtheme.h>
#include "resource.h"
#include "gui.h"
#include "netapi.h"
#include "modern_ui.h"

/**
 * Create and register the main application window
 */
HWND CreateMainWindow(HINSTANCE hInstance) {
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = MainWindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"HandleHunterClass";
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON));
    wc.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON));
    
    if (!RegisterClassExW(&wc)) {
        MessageBoxW(NULL, L"Window registration failed!", L"Error", MB_ICONERROR);
        return NULL;
    }
    
    HWND hwnd = CreateWindowExW(
        0,
        L"HandleHunterClass",
        L"HandleHunter - Local File Locks",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1000, 600,
        NULL, NULL,
        hInstance,
        NULL
    );
    
    if (!hwnd) {
        MessageBoxW(NULL, L"Window creation failed!", L"Error", MB_ICONERROR);
        return NULL;
    }
    
    // Enable dark mode and modern styling
    EnableDarkMode(hwnd);
    ApplyModernWindowStyle(hwnd);
    
    return hwnd;
}

/**
 * Create all child controls
 */
void CreateControls(HWND hwndParent, AppState* state) {
    HINSTANCE hInst = (HINSTANCE)GetWindowLongPtr(hwndParent, GWLP_HINSTANCE);
    int y = 15;
    
    // ============================================
    // Search Section
    // ============================================
    
    // Search label
    HWND hwndSearchLabel = CreateWindowW(L"STATIC",
        L"Search:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        15, y + 3,
        60, 20,
        hwndParent, NULL, hInst, NULL);
    
    // Set label font
    HFONT hFont = CreateFontW(
        19, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI"
    );
    SendMessage(hwndSearchLabel, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    // Search box (standard edit control)
    state->hwndSearch = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_TABSTOP,
        85, y - 2,
        300, 32,
        hwndParent,
        (HMENU)IDC_SEARCH_EDIT,
        hInst, NULL);
    SendMessage(state->hwndSearch, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    // Refresh button (standard button control)
    HWND hwndRefreshBtn = CreateWindowW(L"BUTTON", L"Refresh (F5)",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        395, y - 2, 130, 32,
        hwndParent, (HMENU)IDC_REFRESH_BTN, hInst, NULL);
    SendMessage(hwndRefreshBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    y += 40;
    
    // ============================================
    // ListView Section
    // ============================================
    
    RECT rcClient;
    GetClientRect(hwndParent, &rcClient);
    
    state->hwndListView = CreateWindowW(WC_LISTVIEW,
        L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS,
        15, y,
        rcClient.right - 30,
        rcClient.bottom - y - 90,
        hwndParent,
        (HMENU)IDC_LISTVIEW,
        hInst, NULL);
    
    // Apply standard theming to ListView
    SubclassListViewForDarkMode(state->hwndListView);
    
    // Set ListView font
    HFONT hListFont = CreateFontW(
        17, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI"
    );
    SendMessage(state->hwndListView, WM_SETFONT, (WPARAM)hListFont, TRUE);
    
    // Configure ListView columns
    LVCOLUMNW col = {0};
    col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
    col.fmt = LVCFMT_LEFT;
    
    col.cx = 200;
    col.pszText = L"File Name";
    ListView_InsertColumn(state->hwndListView, 0, &col);
    
    col.cx = 350;
    col.pszText = L"Path";
    ListView_InsertColumn(state->hwndListView, 1, &col);
    
    col.cx = 120;
    col.pszText = L"User";
    ListView_InsertColumn(state->hwndListView, 2, &col);
    
    col.cx = 80;
    col.pszText = L"Locks";
    ListView_InsertColumn(state->hwndListView, 3, &col);
    
    ListView_SetExtendedListViewStyle(state->hwndListView,
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    
    // ============================================
    // Action Buttons
    // ============================================
    
    // Position release button on the right side (standard button control)
    int btnWidth = 220;
    int btnX = rcClient.right - btnWidth - 15;
    HWND hwndReleaseBtn = CreateWindowW(L"BUTTON", L"Release Selected Lock(s)",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        btnX, rcClient.bottom - 72, btnWidth, 36,
        hwndParent, (HMENU)IDC_RELEASE_BTN, hInst, NULL);
    SendMessage(hwndReleaseBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    // ============================================
    // Status Bar
    // ============================================
    
    state->hwndStatus = CreateWindowExW(
        0, STATUSCLASSNAMEW,
        L"Ready",
        WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
        0, 0, 0, 0,
        hwndParent,
        (HMENU)IDC_STATUSBAR,
        hInst, NULL);
    
    // Set status bar font
    HFONT hStatusFont = CreateFontW(
        17, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI"
    );
    SendMessage(state->hwndStatus, WM_SETFONT, (WPARAM)hStatusFont, TRUE);
}

/**
 * Update the ListView with current lock data
 */
void UpdateListView(AppState* state) {
    if (!state || !state->hwndListView) return;
    
    ListView_DeleteAllItems(state->hwndListView);
    
    WCHAR searchText[256] = L"";
    if (state->hwndSearch) {
        GetWindowTextW(state->hwndSearch, searchText, 256);
        _wcslwr(searchText);
    }
    
    BOOL hasFilter = (wcslen(searchText) > 0);
    
    for (DWORD i = 0; i < state->locks.count; i++) {
        FileLockInfo* lock = &state->locks.items[i];
        
        // Apply search filter
        if (hasFilter) {
            WCHAR lowerFilename[512], lowerPath[512], lowerUsername[256];
            wcsncpy(lowerFilename, lock->fileName, 256);
            wcsncpy(lowerPath, lock->filePath, MAX_PATH);
            wcsncpy(lowerUsername, lock->username, 256);
            _wcslwr(lowerFilename);
            _wcslwr(lowerPath);
            _wcslwr(lowerUsername);
            
            if (!wcsstr(lowerFilename, searchText) &&
                !wcsstr(lowerPath, searchText) &&
                !wcsstr(lowerUsername, searchText)) {
                continue;
            }
        }
        
        LVITEMW item = {0};
        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = ListView_GetItemCount(state->hwndListView);
        item.lParam = (LPARAM)lock->fileId;
        item.pszText = lock->fileName;
        int index = ListView_InsertItem(state->hwndListView, &item);
        
        ListView_SetItemText(state->hwndListView, index, 1, lock->filePath);
        ListView_SetItemText(state->hwndListView, index, 2, lock->username);
        
        WCHAR lockCount[16];
        swprintf(lockCount, 16, L"%lu", lock->numLocks);
        ListView_SetItemText(state->hwndListView, index, 3, lockCount);
    }
    
    WCHAR statusText[256];
    swprintf(statusText, 256, L"Showing %d file(s)", ListView_GetItemCount(state->hwndListView));
    SetWindowTextW(state->hwndStatus, statusText);
}

/**
 * Handle "Refresh" button click
 */
void OnRefresh(HWND hwnd, AppState* state) {
    if (!state) return;
    
    SetWindowTextW(state->hwndStatus, L"Refreshing...");
    
    LockArray_Clear(&state->locks);
    
    // Always enumerate local machine (NULL server name)
    DWORD result = EnumerateOpenFiles(NULL, &state->locks);
    
    if (result == 0) {
        UpdateListView(state);
        WCHAR statusText[256];
        swprintf(statusText, 256, L"Found %lu open file(s)", state->locks.count);
        SetWindowTextW(state->hwndStatus, statusText);
    } else {
        WCHAR errorMsg[512];
        swprintf(errorMsg, 512, L"Failed to enumerate files. Error code: %lu\nMake sure you're running as Administrator.", result);
        MessageBoxW(hwnd, errorMsg, L"Error", MB_ICONERROR);
        SetWindowTextW(state->hwndStatus, L"Error refreshing file list");
    }
}

/**
 * Handle "Release Lock" button click - supports multiple selections
 */
void OnReleaseLock(HWND hwnd, AppState* state) {
    if (!state || !state->hwndListView) return;
    
    // Count selected items
    int selectedCount = ListView_GetSelectedCount(state->hwndListView);
    if (selectedCount == 0) {
        MessageBoxW(hwnd, L"Please select one or more files to release.", L"No Selection", MB_ICONINFORMATION);
        return;
    }
    
    // Build confirmation message
    WCHAR confirmMsg[768];
    if (selectedCount == 1) {
        int selectedIndex = ListView_GetNextItem(state->hwndListView, -1, LVNI_SELECTED);
        WCHAR filename[512];
        ListView_GetItemText(state->hwndListView, selectedIndex, 0, filename, 512);
        swprintf(confirmMsg, 768, 
            L"Are you sure you want to release the lock on:\n\n%ls\n\nWarning: This may cause unsaved data loss!", 
            filename);
    } else {
        swprintf(confirmMsg, 768, 
            L"Are you sure you want to release locks on %d selected files?\n\nWarning: This may cause unsaved data loss!", 
            selectedCount);
    }
    
    int response = MessageBoxW(hwnd, confirmMsg, L"Confirm Release", MB_YESNO | MB_ICONWARNING);
    
    if (response == IDYES) {
        WCHAR statusMsg[256];
        swprintf(statusMsg, 256, L"Releasing %d lock(s)...", selectedCount);
        SetWindowTextW(state->hwndStatus, statusMsg);
        
        int successCount = 0;
        int failCount = 0;
        int itemIndex = -1;
        
        // Iterate through all selected items
        while ((itemIndex = ListView_GetNextItem(state->hwndListView, itemIndex, LVNI_SELECTED)) != -1) {
            LVITEMW item = {0};
            item.mask = LVIF_PARAM;
            item.iItem = itemIndex;
            ListView_GetItem(state->hwndListView, &item);
            DWORD fileId = (DWORD)item.lParam;
            
            DWORD result = CloseFileLock(NULL, fileId);
            
            if (result == 0) {
                successCount++;
            } else {
                failCount++;
            }
        }
        
        // Show results
        WCHAR resultMsg[512];
        if (failCount == 0) {
            swprintf(resultMsg, 512, L"Successfully released %d lock(s)!", successCount);
            MessageBoxW(hwnd, resultMsg, L"Success", MB_ICONINFORMATION);
        } else {
            swprintf(resultMsg, 512, L"Released %d lock(s).\nFailed to release %d lock(s).", successCount, failCount);
            MessageBoxW(hwnd, resultMsg, L"Partial Success", MB_ICONWARNING);
        }
        
        OnRefresh(hwnd, state);
    }
}

/**
 * Handle search text box change
 */
void OnSearchChanged(HWND hwnd, AppState* state) {
    UpdateListView(state);
}

/**
 * Main window procedure
 */
LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static AppState* state = NULL;
    
    switch (msg) {
        case WM_CREATE: {
            state = (AppState*)malloc(sizeof(AppState));
            if (!state) {
                MessageBoxW(hwnd, L"Failed to allocate memory for AppState", L"Error", MB_ICONERROR);
                return -1;
            }
            
            memset(state, 0, sizeof(AppState));
            LockArray_Init(&state->locks);
            state->hwndServer = NULL;  // No server field anymore
            
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)state);
            
            CreateControls(hwnd, state);
            
            // Auto-refresh on startup
            PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_REFRESH_BTN, BN_CLICKED), 0);
            
            return 0;
        }
        
        case WM_SIZE: {
            if (state && state->hwndStatus) {
                SendMessage(state->hwndStatus, WM_SIZE, 0, 0);
                
                RECT rcClient;
                GetClientRect(hwnd, &rcClient);
                
                if (state->hwndListView) {
                    SetWindowPos(state->hwndListView, NULL,
                        15, 58,
                        rcClient.right - 30,
                        rcClient.bottom - 140,
                        SWP_NOZORDER);
                }
                
                HWND hwndReleaseBtn = GetDlgItem(hwnd, IDC_RELEASE_BTN);
                if (hwndReleaseBtn) {
                    int btnWidth = 220;
                    int btnX = rcClient.right - btnWidth - 15;
                    SetWindowPos(hwndReleaseBtn, NULL,
                        btnX, rcClient.bottom - 72,
                        btnWidth, 36,
                        SWP_NOZORDER);
                }
            }
            return 0;
        }
        
        case WM_COMMAND: {
            int controlId = LOWORD(wParam);
            int notifyCode = HIWORD(wParam);
            
            switch (controlId) {
                case IDC_REFRESH_BTN:
                    // Handle both button clicks (BN_CLICKED=0) and accelerator commands (notifyCode=1)
                    if (notifyCode == BN_CLICKED || notifyCode == 1) {
                        OnRefresh(hwnd, state);
                    }
                    break;
                    
                case IDC_RELEASE_BTN:
                    if (notifyCode == BN_CLICKED) {
                        OnReleaseLock(hwnd, state);
                    }
                    break;
                    
                case IDC_SEARCH_EDIT:
                    if (notifyCode == EN_CHANGE) {
                        OnSearchChanged(hwnd, state);
                    }
                    break;
            }
            
            return 0;
        }
        
        case WM_NOTIFY: {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            if (pnmh->idFrom == IDC_LISTVIEW && pnmh->code == NM_DBLCLK) {
                OnReleaseLock(hwnd, state);
            }
            return 0;
        }
        
        case WM_DESTROY: {
            if (state) {
                LockArray_Free(&state->locks);
                free(state);
            }
            PostQuitMessage(0);
            return 0;
        }
        
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}
