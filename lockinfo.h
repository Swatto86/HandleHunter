/*
 * lockinfo.h - Data structures for managing file locks
 * 
 * This header file defines the core data structures used throughout the application.
 * It should be included by any file that needs to work with file lock information.
 */

 #ifndef LOCKINFO_H
 #define LOCKINFO_H
 
 #include <windows.h>    // For DWORD, WCHAR, MAX_PATH, HWND, BOOL
 #include "modern_ui.h"  // For ThemeColors structure
 
 /**
  * FileLockInfo - Represents a single file lock on a server
  * 
  * This structure contains all information about one locked file, including
  * its location, who has it locked, and how to release the lock.
  */
 typedef struct {
     DWORD fileId;              // Windows file ID (required for NetFileClose)
                                 // This is the unique identifier returned by NetFileEnum
     
     WCHAR serverName[256];     // UNC name of server hosting the file
                                 // Format: "\\SERVERNAME" or "LOCAL" for local machine
                                 // 256 chars allows for long server names
     
     WCHAR filePath[MAX_PATH];  // Full UNC path to the file
                                 // Format: "\\SERVERNAME\\SHARE\\PATH\\FILE.EXT"
                                 // MAX_PATH is Windows constant (typically 260)
     
     WCHAR fileName[256];       // Just the filename portion (extracted from filePath)
                                 // Example: "Budget.xlsx"
                                 // Pre-extracted for faster display/search
     
     WCHAR username[256];       // Username of the person holding the lock
                                 // Format: "DOMAIN\\USERNAME" or just "USERNAME"
     
     DWORD permissions;         // Access permissions flags
                                 // Bit flags indicating read/write/delete permissions
                                 // See Windows FILE_INFO_3 documentation
     
     DWORD numLocks;            // Number of locks on this file
                                 // A file can have multiple locks (different users/processes)
 } FileLockInfo;
 
 /**
  * LockArray - Dynamic array (vector) for storing FileLockInfo structures
  * 
  * This is a simple dynamic array implementation that automatically grows
  * as items are added. Similar to std::vector in C++.
  */
 typedef struct {
     FileLockInfo* items;       // Pointer to dynamically allocated array
                                 // NULL if not initialized or after LockArray_Free()
     
     DWORD count;               // Number of items currently in the array
                                 // This is the logical size (how many are used)
     
     DWORD capacity;            // Total capacity of the array
                                 // This is the physical size (how many can fit)
                                 // capacity >= count always
 } LockArray;
 
 /**
  * AppState - Global application state structure
  * 
  * This structure holds all the state for a single window instance.
  * It's stored in the window's user data (via SetWindowLongPtr) so
  * we can access it from any message handler without global variables.
  */
 typedef struct {
     // Window and control handles
     HWND hwndMain;             // Handle to main application window
     HWND hwndListView;         // Handle to ListView control (shows file locks)
     HWND hwndSearch;           // Handle to search edit box
     HWND hwndStatus;           // Handle to status bar (bottom of window)
     
     // Data
     LockArray locks;           // All file locks retrieved from local machine
                                 // Populated by EnumerateOpenFiles() on refresh
     
     // Current state
     WCHAR searchText[256];     // Current search filter text
                                 // Empty string means "show all"
     
     BOOL isRefreshing;         // Flag indicating if refresh is in progress
                                 // Used to prevent multiple simultaneous refreshes
     
     // Theme state
     ThemeColors themeColors;   // Current theme colors for dark/light mode
     HBRUSH hBackgroundBrush;   // Brush for window background color
                                 // Must be deleted on theme change and window destroy
 } AppState;
 
 // Function declarations (implemented in lockinfo.c)
 void LockArray_Init(LockArray* array);
 void LockArray_Add(LockArray* array, const FileLockInfo* lock);
 void LockArray_Clear(LockArray* array);
 void LockArray_Free(LockArray* array);
 
 #endif // LOCKINFO_H