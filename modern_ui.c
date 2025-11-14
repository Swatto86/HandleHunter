/*
 * modern_ui.c - Modern UI implementation with dark mode
 */

#include <windows.h>
#include <dwmapi.h>
#include <commctrl.h>
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
 * Enable dark mode for window title bar and borders
 */
void EnableDarkMode(HWND hwnd) {
    BOOL value = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
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
 * Button subclass procedure for hover tracking
 */
LRESULT CALLBACK ButtonSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    ButtonInfo* btnInfo = (ButtonInfo*)dwRefData;
    
    switch (msg) {
        case WM_MOUSEMOVE: {
            if (!btnInfo->isHovered) {
                btnInfo->isHovered = TRUE;
                InvalidateRect(hwnd, NULL, FALSE);
                
                // Track mouse leave
                TRACKMOUSEEVENT tme;
                tme.cbSize = sizeof(TRACKMOUSEEVENT);
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = hwnd;
                TrackMouseEvent(&tme);
            }
            break;
        }
        
        case WM_MOUSELEAVE: {
            btnInfo->isHovered = FALSE;
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        
        case WM_LBUTTONDOWN: {
            btnInfo->state = BTN_STATE_PRESSED;
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        
        case WM_LBUTTONUP: {
            btnInfo->state = BTN_STATE_NORMAL;
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        
        case WM_NCDESTROY: {
            RemoveWindowSubclass(hwnd, ButtonSubclassProc, uIdSubclass);
            if (btnInfo) {
                free(btnInfo);
            }
            break;
        }
    }
    
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

/**
 * Create a modern owner-draw button with hover support
 */
HWND CreateModernButton(HWND parent, const WCHAR* text, int x, int y, int width, int height, int id, HINSTANCE hInst) {
    HWND hwndButton = CreateWindowW(L"BUTTON", text,
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | WS_TABSTOP,
        x, y, width, height,
        parent, (HMENU)(LONG_PTR)id, hInst, NULL);
    
    // Allocate button info
    ButtonInfo* btnInfo = (ButtonInfo*)malloc(sizeof(ButtonInfo));
    if (btnInfo) {
        btnInfo->state = BTN_STATE_NORMAL;
        btnInfo->isHovered = FALSE;
        SetWindowLongPtr(hwndButton, GWLP_USERDATA, (LONG_PTR)btnInfo);
        
        // Subclass for mouse tracking
        SetWindowSubclass(hwndButton, ButtonSubclassProc, 0, (DWORD_PTR)btnInfo);
    }
    
    return hwndButton;
}

/**
 * Draw a modern rounded button (double-buffered to avoid artifacts)
 */
void DrawModernButton(LPDRAWITEMSTRUCT lpDrawItem) {
    HDC hdc = lpDrawItem->hDC;
    RECT rect = lpDrawItem->rcItem;
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    
    // Offscreen buffer
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBmp = CreateCompatibleBitmap(hdc, width, height);
    HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);
    
    RECT localRect = {0, 0, width, height};
    
    // Fill background with parent color to prevent halos
    HBRUSH parentBrush = CreateSolidBrush(COLOR_BG_DARK);
    FillRect(memDC, &localRect, parentBrush);
    DeleteObject(parentBrush);
    
    // Determine colors based on state
    ButtonInfo* btnInfo = (ButtonInfo*)GetWindowLongPtr(lpDrawItem->hwndItem, GWLP_USERDATA);
    COLORREF bgColor;
    COLORREF textColor = COLOR_TEXT_PRIMARY;
    
    if (lpDrawItem->itemState & ODS_SELECTED) {
        bgColor = COLOR_ACCENT_PRESSED;
    } else if (btnInfo && btnInfo->isHovered) {
        bgColor = COLOR_ACCENT_HOVER;
    } else {
        bgColor = COLOR_ACCENT;
    }
    
    // Draw rounded shape
    HBRUSH hBrush = CreateSolidBrush(bgColor);
    HPEN hPen = CreatePen(PS_SOLID, 1, bgColor);
    HBRUSH oldBrush = (HBRUSH)SelectObject(memDC, hBrush);
    HPEN oldPen = (HPEN)SelectObject(memDC, hPen);
    RoundRect(memDC, 0, 0, width, height, 12, 12);
    SelectObject(memDC, oldBrush);
    SelectObject(memDC, oldPen);
    DeleteObject(hBrush);
    DeleteObject(hPen);
    
    // Draw text
    WCHAR text[256];
    GetWindowTextW(lpDrawItem->hwndItem, text, 256);
    SetBkMode(memDC, TRANSPARENT);
    SetTextColor(memDC, textColor);
    
    HFONT hFont = CreateFontW(
        18, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI"
    );
    HFONT oldFont = (HFONT)SelectObject(memDC, hFont);
    DrawTextW(memDC, text, -1, &localRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(memDC, oldFont);
    DeleteObject(hFont);
    
    // Copy to screen
    BitBlt(hdc, rect.left, rect.top, width, height, memDC, 0, 0, SRCCOPY);
    
    SelectObject(memDC, oldBmp);
    DeleteObject(memBmp);
    DeleteDC(memDC);
}

/**
 * Custom edit control window procedure for full owner-draw
 */
LRESULT CALLBACK CustomEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            RECT rect;
            GetClientRect(hwnd, &rect);
            int width = rect.right - rect.left;
            int height = rect.bottom - rect.top;
            
            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBmp = CreateCompatibleBitmap(hdc, width, height);
            HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);
            
            RECT localRect = {0, 0, width, height};
            
            // Base fill with parent background color
            HBRUSH parentBrush = CreateSolidBrush(COLOR_BG_DARK);
            FillRect(memDC, &localRect, parentBrush);
            DeleteObject(parentBrush);
            
            // Draw rounded background
            HBRUSH boxBrush = CreateSolidBrush(COLOR_BG_LIGHTER);
            HPEN borderPen = CreatePen(PS_SOLID, 1, COLOR_BORDER);
            HBRUSH oldBrush = (HBRUSH)SelectObject(memDC, boxBrush);
            HPEN oldPen = (HPEN)SelectObject(memDC, borderPen);
            RoundRect(memDC, 0, 0, width, height, 12, 12);
            SelectObject(memDC, oldBrush);
            SelectObject(memDC, oldPen);
            DeleteObject(boxBrush);
            DeleteObject(borderPen);
            
            // Draw text
            int textLen = GetWindowTextLengthW(hwnd);
            WCHAR* text = (WCHAR*)malloc((textLen + 1) * sizeof(WCHAR));
            if (text) {
                GetWindowTextW(hwnd, text, textLen + 1);
                
                SetBkMode(memDC, TRANSPARENT);
                SetTextColor(memDC, COLOR_TEXT_PRIMARY);
                
                HFONT hFont = CreateFontW(
                    18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                    L"Segoe UI"
                );
                HFONT hOldFont = (HFONT)SelectObject(memDC, hFont);
                
                RECT textRect = localRect;
                textRect.left += 10;
                textRect.right -= 10;
                textRect.top += 2;
                
                DrawTextW(memDC, text, -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
                
                if (GetFocus() == hwnd) {
                    SIZE textSize;
                    GetTextExtentPoint32W(memDC, text, textLen, &textSize);
                    int cursorX = textRect.left + textSize.cx;
                    if (cursorX > textRect.right - 2) cursorX = textRect.right - 2;
                    
                    HPEN caretPen = CreatePen(PS_SOLID, 1, COLOR_TEXT_PRIMARY);
                    HPEN caretOld = (HPEN)SelectObject(memDC, caretPen);
                    MoveToEx(memDC, cursorX, textRect.top + 6, NULL);
                    LineTo(memDC, cursorX, textRect.bottom - 6);
                    SelectObject(memDC, caretOld);
                    DeleteObject(caretPen);
                }
                
                SelectObject(memDC, hOldFont);
                DeleteObject(hFont);
                free(text);
            }
            
            // Blit back
            BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);
            SelectObject(memDC, oldBmp);
            DeleteObject(memBmp);
            DeleteDC(memDC);
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_SETFOCUS:
            CreateCaret(hwnd, NULL, 0, 0);        // register an invisible caret
            HideCaret(hwnd);                      // never let Windows show it
            RedrawWindow(hwnd, NULL, NULL,
                RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE);
            return 0;

        case WM_KILLFOCUS:
            DestroyCaret();
            RedrawWindow(hwnd, NULL, NULL,
                RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE);
            return 0;
        
        case WM_CHAR:
        case WM_KEYDOWN:
        case WM_KEYUP:
            // Let default processing handle text input
            DefSubclassProc(hwnd, msg, wParam, lParam);
            RedrawWindow(hwnd, NULL, NULL,
                RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE);
            return 0;
            
        case WM_ERASEBKGND:
            return 1;
            
        case WM_NCDESTROY:
            RemoveWindowSubclass(hwnd, CustomEditProc, uIdSubclass);
            break;
    }
    
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

/**
 * Subclass edit control for dark mode
 */
void SubclassEditForDarkMode(HWND hwndEdit) {
    SetWindowSubclass(hwndEdit, CustomEditProc, 0, 0);
}

/**
 * Subclass ListView for dark mode
 */
void SubclassListViewForDarkMode(HWND hwndListView) {
    // Set dark mode colors for ListView
    ListView_SetBkColor(hwndListView, COLOR_BG_DARKER);
    ListView_SetTextBkColor(hwndListView, COLOR_BG_DARKER);
    ListView_SetTextColor(hwndListView, COLOR_TEXT_PRIMARY);
    
    // Set outline (grid) color for better visibility
    ListView_SetOutlineColor(hwndListView, COLOR_GRIDLINE);
    
    // Enable modern look
    SetWindowTheme(hwndListView, L"Explorer", NULL);
}

