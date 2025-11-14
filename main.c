/*
 * main.c - Application entry point
 */

#include <windows.h>
#include <commctrl.h>
#include "gui.h"

/**
 * Windows GUI Application Entry Point
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                   LPSTR lpCmdLine, int nCmdShow) {
    
    // Initialize Common Controls library
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);
    
    // Create the main application window
    HWND hwndMain = CreateMainWindow(hInstance);
    if (!hwndMain) {
        return 1;
    }
    
    // Show the window
    ShowWindow(hwndMain, nCmdShow);
    UpdateWindow(hwndMain);
    
    // Main message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return (int)msg.wParam;
}
