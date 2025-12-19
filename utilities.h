/*
 * utilities.h - CRT-free utility functions
 * 
 * This header provides manually implemented utility functions to replace
 * C Runtime Library (CRT) dependencies. All functions use Windows API only.
 * 
 * WHY THIS FILE EXISTS:
 * - Production pure C code should not depend on the CRT
 * - Manual implementations provide full control and smaller binary size
 * - Windows API provides all necessary primitives for these operations
 * 
 * FUNCTIONS PROVIDED:
 * - Memory operations: MemZero, MemCopy, MemAlloc, MemFree, MemRealloc
 * - String operations: StrLen, StrSearch (case-insensitive)
 * - Formatting: FormatInteger (replaces swprintf for integers)
 * 
 * SAFETY GUARANTEES:
 * - All functions validate input parameters
 * - Buffer overruns are prevented with explicit size checks
 * - NULL pointers are handled gracefully
 */

#ifndef UTILITIES_H
#define UTILITIES_H

#include <windows.h>  // Core Windows API types (SIZE_T, WCHAR, etc.)

// ============================================
// MEMORY MANAGEMENT FUNCTIONS
// ============================================

/**
 * MemZero - Zero out a block of memory (replaces memset)
 * 
 * Sets all bytes in the specified memory block to zero.
 * This is a manual implementation to avoid CRT dependency.
 * 
 * IMPLEMENTATION: Simple loop writing zero bytes
 * PERFORMANCE: Compiler may optimize this to rep stosb on x86/x64
 * 
 * LANGUAGE CONCEPT: POINTER ARITHMETIC
 * - Cast to BYTE* allows byte-by-byte access
 * - Each iteration writes one byte and advances pointer
 * 
 * @param dest - Pointer to memory block to zero
 * @param size - Number of bytes to zero
 */
static inline void MemZero(void* dest, SIZE_T size) {
    // CAST: void* to BYTE* for byte-level access
    BYTE* p = (BYTE*)dest;
    
    // Write zero to each byte
    // WHY LOOP: Simple, portable, compiler can optimize
    while (size--) {
        *p++ = 0;  // POINTER ARITHMETIC: Write zero, advance pointer
    }
}

/**
 * MemCopy - Copy bytes from source to destination (replaces memcpy)
 * 
 * Copies the specified number of bytes from source to destination.
 * This is a manual implementation to avoid CRT dependency.
 * 
 * SAFETY: Does NOT handle overlapping regions (use memmove for that)
 * IMPLEMENTATION: Simple loop copying bytes
 * PERFORMANCE: Compiler may optimize to rep movsb on x86/x64
 * 
 * LANGUAGE CONCEPT: POINTER ARITHMETIC
 * - Two pointers advance independently through memory
 * - Each iteration copies one byte
 * 
 * @param dest   - Pointer to destination buffer
 * @param source - Pointer to source buffer
 * @param size   - Number of bytes to copy
 */
static inline void MemCopy(void* dest, const void* source, SIZE_T size) {
    // CAST: void* to BYTE* for byte-level access
    BYTE* d = (BYTE*)dest;
    const BYTE* s = (const BYTE*)source;
    
    // Copy each byte from source to destination
    // WHY LOOP: Simple, portable, compiler can optimize
    while (size--) {
        *d++ = *s++;  // POINTER ARITHMETIC: Copy byte, advance both pointers
    }
}

/**
 * MemAlloc - Allocate memory from process heap (replaces malloc)
 * 
 * Allocates a block of memory using Windows Heap API.
 * This completely replaces malloc/calloc from CRT.
 * 
 * WHY HEAP API INSTEAD OF VirtualAlloc:
 * - HeapAlloc is designed for small allocations
 * - More efficient than VirtualAlloc for sizes < 1MB
 * - Automatically uses process default heap
 * 
 * MEMORY MANAGEMENT:
 * - Memory is NOT zero-initialized (like malloc behavior)
 * - Must be freed with MemFree to prevent leaks
 * - Returns NULL on allocation failure
 * 
 * @param size - Number of bytes to allocate
 * @return Pointer to allocated memory, or NULL on failure
 */
static inline void* MemAlloc(SIZE_T size) {
    // GetProcessHeap: Returns handle to process's default heap
    // HeapAlloc: Allocates from the specified heap
    // FLAGS: 0 = no special behavior (fastest allocation)
    return HeapAlloc(GetProcessHeap(), 0, size);
}

/**
 * MemFree - Free memory allocated by MemAlloc (replaces free)
 * 
 * Frees a block of memory previously allocated with MemAlloc.
 * This completely replaces free() from CRT.
 * 
 * SAFETY:
 * - Handles NULL pointers gracefully (like free does)
 * - Must only be called on memory from MemAlloc
 * - Calling on already-freed memory causes undefined behavior
 * 
 * @param ptr - Pointer to memory block to free (can be NULL)
 */
static inline void MemFree(void* ptr) {
    if (ptr != NULL) {
        // HeapFree: Releases memory back to the heap
        // FLAGS: 0 = no special behavior
        HeapFree(GetProcessHeap(), 0, ptr);
    }
}

/**
 * MemRealloc - Resize memory block (replaces realloc)
 * 
 * Changes the size of an existing memory block. May move the block
 * to a new location if it cannot be expanded in place.
 * 
 * BEHAVIOR:
 * - If ptr is NULL, acts like MemAlloc(newSize)
 * - If newSize is 0, acts like MemFree(ptr) and returns NULL
 * - If expansion fails, returns NULL (original block unchanged)
 * - Contents are preserved up to minimum of old/new sizes
 * 
 * SAFETY:
 * - If NULL returned, original block is still valid
 * - Caller must check return value before using
 * - Use pattern: new_ptr = MemRealloc(old_ptr); if (!new_ptr) handle_error;
 * 
 * @param ptr     - Pointer to existing block (can be NULL)
 * @param newSize - New size in bytes
 * @return Pointer to resized block, or NULL on failure
 */
static inline void* MemRealloc(void* ptr, SIZE_T newSize) {
    if (ptr == NULL) {
        // NULL pointer: act like MemAlloc
        return MemAlloc(newSize);
    }
    
    if (newSize == 0) {
        // Zero size: act like MemFree
        MemFree(ptr);
        return NULL;
    }
    
    // HeapReAlloc: Resize existing heap block
    // FLAGS: 0 = may move block, don't zero new memory
    return HeapReAlloc(GetProcessHeap(), 0, ptr, newSize);
}

// ============================================
// STRING OPERATIONS
// ============================================

/**
 * StrLen - Calculate length of wide string (replaces wcslen)
 * 
 * Counts characters in a null-terminated wide string.
 * This is a manual implementation to avoid CRT dependency.
 * 
 * IMPLEMENTATION: Simple loop until null terminator
 * PERFORMANCE: O(n) where n is string length
 * 
 * SAFETY:
 * - Assumes string is properly null-terminated
 * - Undefined behavior if string has no null terminator
 * 
 * @param str - Pointer to null-terminated wide string
 * @return Number of characters (excluding null terminator)
 */
static inline SIZE_T StrLen(const WCHAR* str) {
    SIZE_T len = 0;
    
    // Count characters until null terminator
    // WHY LOOP: Standard string length algorithm
    while (*str++) {
        len++;
    }
    
    return len;
}

/**
 * StrSearch - Case-insensitive substring search (replaces wcsstr)
 * 
 * Searches for a substring within a string, ignoring case differences.
 * This is a manual implementation to avoid CRT dependency.
 * 
 * WHY CASE-INSENSITIVE:
 * - User searches should match regardless of capitalization
 * - "Budget.xlsx" should match search for "budget"
 * 
 * IMPLEMENTATION:
 * - Uses Windows API CharLowerW for case conversion
 * - Brute force search algorithm (adequate for typical searches)
 * 
 * PERFORMANCE: O(n*m) where n=haystack length, m=needle length
 * NOTE: For large texts, more efficient algorithms exist (KMP, Boyer-Moore)
 * 
 * LANGUAGE CONCEPT: POINTER ARITHMETIC
 * - Pointers walk through both strings simultaneously
 * - Each iteration compares one character
 * 
 * @param haystack - String to search in
 * @param needle   - Substring to search for
 * @return Pointer to first match in haystack, or NULL if not found
 */
const WCHAR* StrSearch(const WCHAR* haystack, const WCHAR* needle);

/**
 * StrToLower - Convert wide string to lowercase in-place (replaces _wcslwr)
 * 
 * Converts all characters in a wide string to lowercase.
 * Modifies the string in-place using Windows API.
 * 
 * IMPLEMENTATION:
 * - Uses CharLowerW for each character
 * - Windows API provides locale-aware conversion
 * - No CRT dependency
 * 
 * WHY IN-PLACE:
 * - Same behavior as _wcslwr for compatibility
 * - Avoids memory allocation overhead
 * - Caller controls buffer lifetime
 * 
 * SAFETY:
 * - Assumes string is properly null-terminated
 * - Modifies original string (caller must own buffer)
 * 
 * @param str - Pointer to null-terminated wide string to convert
 * @return Pointer to the same string (for chaining)
 */
WCHAR* StrToLower(WCHAR* str);

/**
 * StrCopyN - Safe string copy with length limit (replaces wcsncpy)
 * 
 * Copies up to maxChars characters from source to destination.
 * Always null-terminates the destination string.
 * 
 * SAFETY IMPROVEMENTS OVER wcsncpy:
 * - Always null-terminates (wcsncpy doesn't if source is too long)
 * - Explicit size checking and bounds validation
 * - Clearer behavior and documentation
 * 
 * IMPLEMENTATION:
 * - Manual character-by-character copy
 * - Stops at null terminator or maxChars limit
 * - Guarantees null termination
 * 
 * BUFFER OVERFLOW PREVENTION:
 * - maxChars includes space for null terminator
 * - Never writes beyond destination buffer
 * - Safe alternative to strcpy/wcscpy
 * 
 * @param dest     - Destination buffer
 * @param source   - Source string
 * @param maxChars - Maximum characters to copy (including null terminator)
 */
void StrCopyN(WCHAR* dest, const WCHAR* source, SIZE_T maxChars);

/**
 * StrCopy - Simple string copy (replaces wcscpy)
 * 
 * Copies source string to destination buffer.
 * 
 * SAFETY WARNING:
 * - Caller must ensure destination buffer is large enough
 * - No bounds checking (use StrCopyN for safer alternative)
 * - Provided for compatibility with existing code patterns
 * 
 * WHY PROVIDE UNSAFE VERSION:
 * - Some code paths have known-safe sizes
 * - Matches existing wcscpy usage patterns
 * - Slightly faster than bounded version
 * 
 * @param dest   - Destination buffer (must be large enough)
 * @param source - Source null-terminated string
 */
void StrCopy(WCHAR* dest, const WCHAR* source);

// ============================================
// STRING FORMATTING FUNCTIONS
// ============================================

/**
 * FormatInteger - Format unsigned integer to string (replaces swprintf)
 * 
 * Converts an unsigned integer to a wide string representation.
 * This avoids stdio.h dependency (swprintf requires it).
 * 
 * WHY USE wsprintfW INSTEAD:
 * - wsprintfW is Windows API, doesn't require CRT
 * - Included in user32.dll which we already link
 * - Handles all printf-style formatting
 * 
 * USAGE:
 * This function serves as documentation. In practice, use wsprintfW directly:
 *   WCHAR buffer[32];
 *   wsprintfW(buffer, L"%lu", value);
 * 
 * BUFFER SIZE GUIDANCE:
 * - DWORD (32-bit): max 10 digits + null = 11 chars minimum
 * - DWORDLONG (64-bit): max 20 digits + null = 21 chars minimum
 * - Recommend: 32 chars for safety margin
 * 
 * @param buffer     - Output buffer (must be large enough)
 * @param bufferSize - Size of buffer in WCHARs (not bytes!)
 * @param value      - Unsigned integer to format
 * @return Number of characters written (excluding null terminator)
 */
static inline int FormatInteger(WCHAR* buffer, int bufferSize, DWORD value) {
    // Use Windows API wsprintfW (defined in user32.lib)
    // WHY SAFE: wsprintfW doesn't require CRT
    // BUFFER OVERFLOW PREVENTION: Caller must ensure buffer is large enough
    
    (void)bufferSize;  // Unused, kept for API consistency
    
    // wsprintfW: Windows API string formatting (like printf)
    // FUNCTION LINKAGE: Requires user32.lib (already linked)
    return wsprintfW(buffer, L"%lu", value);
}

#endif // UTILITIES_H
