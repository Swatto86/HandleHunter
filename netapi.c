/*
 * netapi.c - Windows NetAPI wrapper functions
 * 
 * This file provides a clean interface to Windows Network API functions
 * for enumerating and closing file locks on servers (local or remote).
 * 
 * WHY THIS FILE EXISTS:
 * - Windows NetAPI functions are complex with many parameters
 * - Wrapping them makes the rest of the code simpler and cleaner
 * - Handles error checking and memory management in one place
 * - Translates Windows API structures to our FileLockInfo structure
 * 
 * WINDOWS NETWORK API OVERVIEW:
 * - NetFileEnum: Lists all files currently open on a server
 * - NetFileClose: Forcibly closes a file handle
 * - NetApiBufferFree: Frees memory allocated by NetAPI functions
 * 
 * WHY USE NETAPI:
 * - Only way to enumerate/close file locks programmatically on Windows
 * - Requires Administrator privileges (security restriction)
 * - Used by "Computer Management" > "Shared Folders" > "Open Files"
 * 
 * MEMORY MANAGEMENT:
 * - NetAPI functions allocate memory internally
 * - MUST free with NetApiBufferFree, NOT free() or delete
 * - Failure to free causes memory leaks
 */

#include <windows.h>     // Core Windows API
#include <lm.h>          // LanManager API (NetFileEnum, NetFileClose, etc.)
#include <stdio.h>       // Not currently used, but included for potential debugging
#include "lockinfo.h"    // FileLockInfo and LockArray structures

/**
 * EnumerateOpenFiles - Enumerate all open files on a server
 * 
 * This function queries Windows for all files currently open on a server.
 * Each open file has metadata like who opened it, the path, and a unique ID.
 * 
 * WHY THIS IS NEEDED:
 * - To see which files are locked (preventing deletion/modification)
 * - To identify who has files open before releasing locks
 * - To populate the ListView in the GUI
 * 
 * HOW IT WORKS:
 * 1. Call NetFileEnum to get array of FILE_INFO_3 structures from Windows
 * 2. Loop through each FILE_INFO_3 structure
 * 3. Extract relevant fields and copy to our FileLockInfo structure
 * 4. Add FileLockInfo to the output array
 * 5. Free the memory allocated by NetFileEnum
 * 
 * ADMINISTRATOR PRIVILEGES REQUIRED:
 * - NetFileEnum requires Administrator rights
 * - Returns error code 5 (Access Denied) if not running as Administrator
 * - This is a Windows security restriction, not a bug
 * 
 * @param serverName - UNC path to server (e.g., L"\\\\SERVER01" or NULL for local machine)
 * @param outArray   - Pointer to LockArray to populate with results (must be initialized)
 * @return DWORD - 0 (NERR_Success) on success, error code on failure
 */
DWORD EnumerateOpenFiles(WCHAR* serverName, LockArray* outArray) {
     // ============================================
     // Local Variables
     // ============================================
     PFILE_INFO_3 pFileInfo = NULL;      // Buffer returned by NetFileEnum (API allocates)
     DWORD dwEntriesRead = 0;            // Number of entries actually read and returned
     DWORD dwTotalEntries = 0;           // Total entries available (may be more than returned)
     NET_API_STATUS nStatus;             // Return status from NetFileEnum
     
     // ============================================
     // STEP 1: Clear any existing data
     // ============================================
     // WHY: We're repopulating from scratch, not appending
     // LockArray_Clear sets count to 0 but keeps memory allocated
     LockArray_Clear(outArray);
     
     // ============================================
     // STEP 2: Call NetFileEnum to get open files
     // ============================================
     // NetFileEnum is a Windows LanManager API function that enumerates
     // all files currently open on a server (local or remote)
     //
     // WHY LEVEL 3 (FILE_INFO_3):
     // - Level 3 provides the most detailed information
     // - Includes file ID (required for NetFileClose)
     // - Includes path, username, permissions, lock count
     // - Lower levels (0, 1, 2) provide less information
     nStatus = NetFileEnum(
         serverName,                     // Server name (NULL = local machine)
                                         // Format: L"\\\\SERVERNAME" for remote
         NULL,                           // Base path filter (NULL = all files)
                                         // Could filter to specific share: L"C:\\Share\\*"
         NULL,                           // Username filter (NULL = all users)
                                         // Could filter to specific user: L"DOMAIN\\USER"
         3,                              // Information level (3 = FILE_INFO_3)
                                         // Higher level = more details returned
         (LPBYTE*)&pFileInfo,            // Output buffer (API allocates memory)
                                         // CRITICAL: Must free with NetApiBufferFree
         MAX_PREFERRED_LENGTH,           // Request maximum data (API allocates as needed)
                                         // Alternative: Fixed buffer size (more complex)
         &dwEntriesRead,                 // Output: Number of entries returned
                                         // Usually equals dwTotalEntries
         &dwTotalEntries,                // Output: Total entries available
                                         // May be more than dwEntriesRead if buffer too small
         NULL                            // Resume handle (NULL = start from beginning)
                                         // For pagination: Pass non-NULL to continue
     );
     
     // ============================================
     // STEP 3: Check if the API call succeeded
     // ============================================
     if (nStatus != NERR_Success) {
         // NetFileEnum failed - return error code to caller
         // Common errors:
         // - 5 (ERROR_ACCESS_DENIED): Not running as Administrator
         // - 53 (ERROR_BAD_NETPATH): Server not found
         // - 2310 (NERR_ClientNameNotFound): Invalid server name
         return nStatus;
     }
     
     // ============================================
     // STEP 4: Process each file entry
     // ============================================
     // Loop through all files returned by NetFileEnum
     for (DWORD i = 0; i < dwEntriesRead; i++) {
         FILE_INFO_3* pFile = &pFileInfo[i];  // Pointer to current file's info
         
         // Initialize our FileLockInfo structure (zero all fields)
         // WHY: Ensures no garbage data in fields we don't set
         FileLockInfo lock = {0};
         
         // ========================================
         // Copy File ID
         // ========================================
         // fi3_id is a unique identifier for this open file
         // WHY NEEDED: NetFileClose requires this ID to close the file
         // ID is only valid until file is closed or server is rebooted
         lock.fileId = pFile->fi3_id;
         
         // ========================================
         // Copy Permission Flags
         // ========================================
         // fi3_permissions is a bitmask indicating access rights
         // Possible values (can be combined with OR):
         // - PERM_FILE_READ (0x1): Read access
         // - PERM_FILE_WRITE (0x2): Write access
         // - PERM_FILE_CREATE (0x4): Create access
         // WHY STORE: Could be used to display read-only vs. read-write locks
         lock.permissions = pFile->fi3_permissions;
         
         // ========================================
         // Copy Number of Locks
         // ========================================
         // fi3_num_locks indicates how many locks this file has
         // WHY MULTIPLE LOCKS: A file can be opened multiple times by:
         // - Same user, different processes
         // - Different users (if share allows)
         // - Same process, multiple handles
         lock.numLocks = pFile->fi3_num_locks;
         
         // ========================================
         // Copy Server Name
         // ========================================
         if (serverName) {
             // Remote server - copy the server name
             wcsncpy(lock.serverName, serverName, 255);
         } else {
             // Local machine - use "LOCAL" as placeholder
             wcsncpy(lock.serverName, L"LOCAL", 255);
         }
         lock.serverName[255] = L'\0';  // Ensure null termination
         // WHY NULL TERMINATE: wcsncpy doesn't guarantee null termination if source is too long
         
         // ========================================
         // Copy and Parse File Path
         // ========================================
         if (pFile->fi3_pathname) {
             // Copy full file path with bounds checking
             wcsncpy(lock.filePath, pFile->fi3_pathname, MAX_PATH - 1);
             lock.filePath[MAX_PATH - 1] = L'\0';  // Ensure null termination
             
             // Extract just the filename from the full path
             // WHY: ListView displays filename separately from path for readability
             //      Users often search by filename, not full path
             
             // Find the last backslash in the path
             WCHAR* lastSlash = wcsrchr(lock.filePath, L'\\');
             if (lastSlash) {
                 // Backslash found - everything after it is the filename
                 // lastSlash + 1 skips the backslash itself
                 wcsncpy(lock.fileName, lastSlash + 1, 255);
             } else {
                 // No backslash found - entire path is the filename
                 // This shouldn't happen with UNC paths, but handle it gracefully
                 wcsncpy(lock.fileName, lock.filePath, 255);
             }
             lock.fileName[255] = L'\0';  // Ensure null termination
         } else {
             // No path available (shouldn't happen, but be defensive)
             lock.fileName[0] = L'\0';
             lock.filePath[0] = L'\0';
         }
         
         // ========================================
         // Copy Username
         // ========================================
         if (pFile->fi3_username) {
             // Copy username with bounds checking
             wcsncpy(lock.username, pFile->fi3_username, 255);
             lock.username[255] = L'\0';  // Ensure null termination
         } else {
             // No username available (very rare, but be defensive)
             wcscpy(lock.username, L"Unknown");
         }
         
         // ========================================
         // Add to Output Array
         // ========================================
         // LockArray_Add copies the structure into the array
         // Array automatically grows if needed
         LockArray_Add(outArray, &lock);
     }
     
     // ============================================
     // STEP 5: Free the buffer allocated by NetFileEnum
     // ============================================
     // CRITICAL: Always use NetApiBufferFree, NEVER free() or delete
     // WHY: NetAPI functions use a special memory allocator
     //      Using wrong free function causes crashes
     if (pFileInfo != NULL) {
         NetApiBufferFree(pFileInfo);
         // After this, pFileInfo is invalid - don't use it
     }
     
     // ============================================
     // STEP 6: Return success
     // ============================================
     return 0;  // 0 = NERR_Success (no error)
  }
 
/**
 * CloseFileLock - Close a specific file lock by file ID
 * 
 * This function forcibly closes a file handle on the server, effectively
 * releasing the lock and making the file available for other operations.
 * 
 * WHY THIS IS NEEDED:
 * - To release locks when files are stuck open
 * - To allow deletion/modification of locked files
 * - To free resources when processes crash without closing files
 * 
 * HOW IT WORKS:
 * - NetFileClose tells Windows to close the specified file handle
 * - The file ID comes from NetFileEnum (fi3_id field)
 * - After closing, the file is no longer listed by NetFileEnum
 * 
 * ADMINISTRATOR PRIVILEGES REQUIRED:
 * - NetFileClose requires Administrator rights
 * - Returns error code 5 (Access Denied) if not running as Administrator
 * - This is a Windows security restriction
 * 
 * WARNING TO USERS:
 * - Forcibly closing files can cause DATA LOSS
 * - User may lose unsaved changes in the closed application
 * - Application using the file may crash or behave unexpectedly
 * - Always warn user before calling this function
 * 
 * WHAT HAPPENS TO THE APPLICATION:
 * - Application receives an error when trying to read/write the file
 * - Well-written apps show "file not found" or "access denied" error
 * - Poorly-written apps may crash
 * - Unsaved data is lost
 * 
 * @param serverName - UNC path to server (e.g., L"\\\\SERVER01" or NULL for local)
 * @param fileId     - File ID from NetFileEnum (fi3_id field)
 * @return DWORD - 0 (NERR_Success) on success, error code on failure
 */
DWORD CloseFileLock(WCHAR* serverName, DWORD fileId) {
     NET_API_STATUS nStatus;
     
     // ============================================
     // Call NetFileClose to close the file handle
     // ============================================
     // NetFileClose is a Windows LanManager API function
     // WHY SIMPLE: This API does exactly one thing - close a file
     //             No complex parameters or data structures needed
     nStatus = NetFileClose(
         serverName,    // Server name (NULL = local machine)
         fileId         // File ID from NetFileEnum
     );
     // If successful, the file is closed immediately
     // Applications using the file will get errors on next access
     
     // Return status code (0 = success, non-zero = error)
     // Common errors:
     // - 5 (ERROR_ACCESS_DENIED): Not running as Administrator
     // - 2314 (NERR_FileIdNotFound): File already closed or invalid ID
     return nStatus;
}
 
/**
 * TestServerConnection - Test if we can successfully connect to a server
 * 
 * This function performs a lightweight test to check if we can access a server.
 * It attempts to enumerate files but doesn't care about the results.
 * 
 * WHY THIS IS NEEDED:
 * - To validate server name before attempting operations
 * - To provide user feedback if server is unreachable
 * - To distinguish between "server not found" vs. "access denied"
 * 
 * HOW IT WORKS:
 * - Calls NetFileEnum with the server name
 * - If it returns NERR_Success, server is accessible
 * - Even if 0 files are open, success means connection works
 * - Immediately frees any returned data (we don't need it)
 * 
 * WHY NOT JUST TRY THE REAL OPERATION:
 * - This is lighter weight (no need to process results)
 * - Can validate server name in UI before user clicks button
 * - Provides better user experience with early error detection
 * 
 * NOTE: This function is currently not used in HandleHunter (we only
 *       connect to local machine). It's here for potential future use
 *       if remote server support is added.
 * 
 * @param serverName - UNC path to server (e.g., L"\\\\SERVER01")
 * @return BOOL - TRUE if connection successful, FALSE otherwise
 */
BOOL TestServerConnection(WCHAR* serverName) {
     PFILE_INFO_3 pFileInfo = NULL;      // Buffer for file info (not used)
     DWORD dwEntriesRead = 0;             // Number of entries (not used)
     DWORD dwTotalEntries = 0;            // Total entries (not used)
     NET_API_STATUS nStatus;              // Return status
     
     // ============================================
     // Try to enumerate files (don't process results)
     // ============================================
     // WHY: If NetFileEnum succeeds, we can connect to the server
     //      We don't care about the actual files, just if the call works
     nStatus = NetFileEnum(
         serverName,                     // Server to test
         NULL,                           // No path filter
         NULL,                           // No user filter
         3,                              // Level 3 info (same as real query)
         (LPBYTE*)&pFileInfo,           // Output buffer (API allocates)
         MAX_PREFERRED_LENGTH,          // Request max data
         &dwEntriesRead,                // Entries read (not used)
         &dwTotalEntries,               // Total available (not used)
         NULL                           // No resume handle
     );
     
     // ============================================
     // Free buffer if allocated (we don't need the data)
     // ============================================
     if (pFileInfo != NULL) {
         NetApiBufferFree(pFileInfo);
     }
     
     // ============================================
     // Return TRUE if connection successful, FALSE otherwise
     // ============================================
     // NERR_Success (0) means we can connect and enumerate
     // Any other value means connection failed or access denied
     return (nStatus == NERR_Success);
 }