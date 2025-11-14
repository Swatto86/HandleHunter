/*
 * netapi.h - Windows NetAPI wrapper function declarations
 * 
 * This header declares functions that wrap Windows Network API calls
 * for enumerating and closing file locks. Include this in any file
 * that needs to interact with file servers.
 */

#ifndef NETAPI_H
#define NETAPI_H

#include <windows.h>    // Windows API (BOOL, DWORD, WCHAR)
#include "lockinfo.h"     // LockArray structure

// ============================================
// File Lock Management Functions
// ============================================

/**
 * Enumerate all open files on a server
 * 
 * @param serverName - UNC path to server (e.g., L"\\\\SERVER01" or NULL for local)
 * @param outArray   - Pointer to LockArray to populate with results
 * @return TRUE if successful, FALSE on error (error message shown to user)
 * 
 * This function queries the server for all currently open files and populates
 * the output array with FileLockInfo structures.
 */
BOOL EnumerateOpenFiles(const WCHAR* serverName, LockArray* outArray);

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
BOOL CloseFileLock(const WCHAR* serverName, DWORD fileId);

/**
 * Test if we can successfully connect to a server
 * 
 * @param serverName - UNC path to server (e.g., L"\\\\SERVER01")
 * @return TRUE if connection successful, FALSE otherwise
 * 
 * This is a lightweight test that attempts to enumerate files.
 * Even if 0 files are returned, a successful return means we can connect.
 */
BOOL TestServerConnection(const WCHAR* serverName);

#endif // NETAPI_H

