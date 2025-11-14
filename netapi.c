/*
 * netapi.c - Windows NetAPI wrapper functions
 * 
 * This file provides a clean interface to Windows Network API functions
 * for enumerating and closing file locks on remote servers.
 */

 #include <windows.h>    // Windows API definitions
 #include <lm.h>          // LAN Manager API (NetFileEnum, NetFileClose)
 #include <stdio.h>       // Standard I/O (for swprintf)
 #include "lockinfo.h"      // Our data structures
 
 // Link against netapi32.lib (required for NetFileEnum, NetFileClose)
 #pragma comment(lib, "netapi32.lib")
 
 /**
  * Enumerate all open files on a server
  * 
  * @param serverName - UNC path to server (e.g., L"\\\\SERVER01" or NULL for local)
  * @param outArray   - Pointer to LockArray to populate with results
  * @return TRUE if successful, FALSE on error (error message shown to user)
  * 
  * This function queries the server for all currently open files and populates
  * the output array with FileLockInfo structures containing details about each lock.
  */
 BOOL EnumerateOpenFiles(const WCHAR* serverName, LockArray* outArray) {
     PFILE_INFO_3 pFileInfo = NULL;      // Buffer returned by NetFileEnum
     DWORD dwEntriesRead = 0;            // Number of entries actually read
     DWORD dwTotalEntries = 0;          // Total entries available (may be more)
     NET_API_STATUS nStatus;            // Return status from NetFileEnum
     
     // Clear any existing data in the output array
     LockArray_Clear(outArray);
     
     // Call NetFileEnum to retrieve list of open files
     // Level 3 (FILE_INFO_3) provides the most detailed information including:
     // - File ID (needed for closing)
     // - Path, username, permissions, number of locks
     nStatus = NetFileEnum(
         serverName,                     // Server name (NULL = local machine)
         NULL,                           // Base path filter (NULL = all files)
         NULL,                           // Username filter (NULL = all users)
         3,                              // Information level (3 = FILE_INFO_3)
         (LPBYTE*)&pFileInfo,           // Output buffer (API allocates memory)
         MAX_PREFERRED_LENGTH,          // Request maximum data (API allocates as needed)
         &dwEntriesRead,                // Number of entries returned
         &dwTotalEntries,               // Total entries available (may be more than read)
         NULL                           // Resume handle (NULL = start from beginning)
     );
     
     // Check if the API call succeeded
     if (nStatus != NERR_Success) {
         // Handle common error conditions with user-friendly messages
         switch (nStatus) {
             case ERROR_ACCESS_DENIED:
                 // Most common error - user needs admin rights
                 MessageBoxW(NULL, 
                     L"Access denied. Run as administrator and ensure you have rights on the target server.",
                     L"Connection Error", MB_ICONERROR);
                 break;
                 
             case ERROR_BAD_NETPATH:
                 // Server name incorrect or network unreachable
                 MessageBoxW(NULL, 
                     L"Server not found. Check the server name and network connectivity.",
                     L"Connection Error", MB_ICONERROR);
                 break;
                 
             default:
                 // Unknown error - show error code for debugging
                 {
                     WCHAR errMsg[512];
                     swprintf(errMsg, 512, L"NetFileEnum failed with error code: %d", nStatus);
                     MessageBoxW(NULL, errMsg, L"Error", MB_ICONERROR);
                 }
                 break;
         }
         return FALSE;  // Indicate failure
     }
     
     // Process each file entry returned by the API
     for (DWORD i = 0; i < dwEntriesRead; i++) {
         FILE_INFO_3* pFile = &pFileInfo[i];
         
         // Initialize our FileLockInfo structure
         FileLockInfo lock = {0};
         
         // Copy file ID (required for NetFileClose)
         lock.fileId = pFile->fi3_id;
         
         // Copy permission flags (read, write, etc.)
         lock.permissions = pFile->fi3_permissions;
         
         // Copy number of locks on this file
         lock.numLocks = pFile->fi3_num_locks;
         
         // Copy server name (ensure null termination)
         if (serverName) {
             wcsncpy(lock.serverName, serverName, 255);
         } else {
             wcsncpy(lock.serverName, L"LOCAL", 255);
         }
         lock.serverName[255] = L'\0';
         
         // Copy full file path if available
         if (pFile->fi3_pathname) {
             // Copy path with bounds checking
             wcsncpy(lock.filePath, pFile->fi3_pathname, MAX_PATH - 1);
             lock.filePath[MAX_PATH - 1] = L'\0';
             
             // Extract just the filename from the full path
             // Find the last backslash in the path
             WCHAR* lastSlash = wcsrchr(lock.filePath, L'\\');
             if (lastSlash) {
                 // Copy everything after the last backslash
                 wcsncpy(lock.fileName, lastSlash + 1, 255);
             } else {
                 // No backslash found, entire path is the filename
                 wcsncpy(lock.fileName, lock.filePath, 255);
             }
             lock.fileName[255] = L'\0';
         } else {
             // No path available, set to empty
             lock.fileName[0] = L'\0';
             lock.filePath[0] = L'\0';
         }
         
         // Copy username who has the file open
         if (pFile->fi3_username) {
             wcsncpy(lock.username, pFile->fi3_username, 255);
             lock.username[255] = L'\0';
         } else {
             // No username available, use default
             wcscpy(lock.username, L"Unknown");
         }
         
         // Add this lock to our output array
         LockArray_Add(outArray, &lock);
     }
     
     // CRITICAL: Free the buffer allocated by NetFileEnum
     // Always use NetApiBufferFree, never free() or delete
     if (pFileInfo != NULL) {
         NetApiBufferFree(pFileInfo);
         pFileInfo = NULL;  // Prevent double-free
     }
     
     return TRUE;  // Success
 }
 
 /**
  * Close a specific file lock by file ID
  * 
  * @param serverName - UNC path to server (e.g., L"\\\\SERVER01")
  * @param fileId     - File ID returned by NetFileEnum (fi3_id)
  * @return TRUE if successful, FALSE on error (error message shown to user)
  * 
  * This function closes a file handle on the remote server, effectively
  * releasing the lock. Requires administrator privileges.
  */
 BOOL CloseFileLock(const WCHAR* serverName, DWORD fileId) {
     NET_API_STATUS nStatus;
     
     // Call NetFileClose to close the file handle
     // This will release the lock and allow other users to access the file
     nStatus = NetFileClose(serverName, fileId);
     
     // Check if the operation succeeded
     if (nStatus != NERR_Success) {
         // Show error message to user
         WCHAR errMsg[512];
         swprintf(errMsg, 512, 
             L"Failed to close file.\nError code: %d\n\nMake sure you have administrator rights.",
             nStatus);
         MessageBoxW(NULL, errMsg, L"Error", MB_ICONERROR);
         return FALSE;
     }
     
     return TRUE;  // Success
 }
 
 /**
  * Test if we can successfully connect to a server
  * 
  * @param serverName - UNC path to server (e.g., L"\\\\SERVER01")
  * @return TRUE if connection successful, FALSE otherwise
  * 
  * This is a lightweight test that attempts to enumerate files.
  * Even if 0 files are returned, a successful return means we can connect.
  */
 BOOL TestServerConnection(const WCHAR* serverName) {
     PFILE_INFO_3 pFileInfo = NULL;
     DWORD dwEntriesRead = 0;
     DWORD dwTotalEntries = 0;
     NET_API_STATUS nStatus;
     
     // Try to enumerate files (we don't care about the results, just if it works)
     nStatus = NetFileEnum(
         serverName,                     // Server to test
         NULL,                           // No path filter
         NULL,                           // No user filter
         3,                              // Level 3 info
         (LPBYTE*)&pFileInfo,           // Output buffer
         MAX_PREFERRED_LENGTH,          // Request max data
         &dwEntriesRead,                // Entries read
         &dwTotalEntries,               // Total available
         NULL                           // No resume handle
     );
     
     // Free buffer if allocated
     if (pFileInfo != NULL) {
         NetApiBufferFree(pFileInfo);
     }
     
     // Return TRUE if enumeration succeeded (even with 0 files)
     return (nStatus == NERR_Success);
 }