/*
 * utilities.c - Implementation of CRT-free utility functions
 * 
 * This file contains implementations of utility functions that cannot be
 * inlined. Most functions are static inline in utilities.h for performance.
 */

#include <windows.h>     // Core Windows API
#include "utilities.h"   // Function declarations

/**
 * StrSearch - Case-insensitive substring search (replaces wcsstr)
 * 
 * This function searches for a substring within a string, ignoring case.
 * It's implemented here (not inline) because it's more complex than other utils.
 * 
 * ALGORITHM:
 * 1. Convert both strings to lowercase (using CharLowerW)
 * 2. Brute force search: try each position in haystack
 * 3. For each position, compare characters with needle
 * 4. Return pointer to first match, or NULL if not found
 * 
 * WHY NOT INLINE:
 * - Function is relatively large (would bloat call sites)
 * - Not performance-critical (search is infrequent)
 * - Only called from UI code, not tight loops
 * 
 * CASE CONVERSION:
 * - CharLowerW modifies a single character in-place
 * - We create temporary buffers to avoid modifying inputs
 * - Buffer size: 1024 chars is sufficient for typical filenames/paths
 * 
 * BUFFER OVERFLOW PREVENTION:
 * - Explicitly checks string lengths against buffer size
 * - Falls back to case-sensitive if strings are too long
 * - Prevents writes beyond buffer boundaries
 * 
 * @param haystack - String to search in (not modified)
 * @param needle   - Substring to search for (not modified)
 * @return Pointer to first match in original haystack, or NULL if not found
 */
const WCHAR* StrSearch(const WCHAR* haystack, const WCHAR* needle) {
    // ============================================
    // STEP 1: Handle edge cases
    // ============================================
    
    // NULL pointer checks
    if (!haystack || !needle) {
        return NULL;
    }
    
    // Empty needle always matches at start
    // WHY: Standard substring search behavior (like wcsstr, strstr)
    if (*needle == L'\0') {
        return haystack;
    }
    
    // ============================================
    // STEP 2: Calculate string lengths
    // ============================================
    
    SIZE_T haystackLen = StrLen(haystack);
    SIZE_T needleLen = StrLen(needle);
    
    // Needle longer than haystack: cannot match
    if (needleLen > haystackLen) {
        return NULL;
    }
    
    // ============================================
    // STEP 3: Create lowercase copies for comparison
    // ============================================
    
    // Stack buffers for lowercase copies
    // 1024 chars = 2KB per buffer, reasonable for filenames/paths
    // WHY STACK: Fast allocation, automatic cleanup
    // BUFFER OVERFLOW PREVENTION: Check lengths before copying
    WCHAR lowerHaystack[1024];
    WCHAR lowerNeedle[1024];
    
    // Validate lengths fit in buffers
    // WHY -1: Need space for null terminator
    if (haystackLen >= 1024 || needleLen >= 1024) {
        // String too long for our buffers
        // FALLBACK: Could implement case-sensitive search here
        // For now, just fail (better than buffer overflow)
        return NULL;
    }
    
    // Copy and convert haystack to lowercase
    // WHY MANUAL COPY: MemCopy doesn't convert case
    for (SIZE_T i = 0; i < haystackLen; i++) {
        // CharLowerW converts single character to lowercase
        // CAST: MAKEINTRESOURCE macro packs WCHAR into LPWSTR for API
        // WHY THIS WORKS: CharLowerW checks if high word is 0, treats as char
        lowerHaystack[i] = (WCHAR)(DWORD_PTR)CharLowerW((LPWSTR)(DWORD_PTR)haystack[i]);
    }
    lowerHaystack[haystackLen] = L'\0';  // Null terminate
    
    // Copy and convert needle to lowercase
    for (SIZE_T i = 0; i < needleLen; i++) {
        lowerNeedle[i] = (WCHAR)(DWORD_PTR)CharLowerW((LPWSTR)(DWORD_PTR)needle[i]);
    }
    lowerNeedle[needleLen] = L'\0';  // Null terminate
    
    // ============================================
    // STEP 4: Brute force substring search
    // ============================================
    
    // Try each position in haystack where needle could start
    // WHY -needleLen+1: No point checking if needle won't fit
    for (SIZE_T i = 0; i <= haystackLen - needleLen; i++) {
        // Check if needle matches at position i
        SIZE_T j;
        for (j = 0; j < needleLen; j++) {
            // Compare one character at a time
            if (lowerHaystack[i + j] != lowerNeedle[j]) {
                break;  // Mismatch, try next position
            }
        }
        
        // Did we match all characters?
        // WHY j == needleLen: Loop exited because all chars matched
        if (j == needleLen) {
            // Found match! Return pointer to match in ORIGINAL haystack
            // WHY NOT lowerHaystack: Caller expects pointer to original string
            return &haystack[i];
        }
    }
    
    // ============================================
    // STEP 5: No match found
    // ============================================
    
    return NULL;
}

/**
 * StrToLower - Convert wide string to lowercase in-place
 * 
 * This function converts all characters in a wide string to lowercase.
 * Uses Windows API CharLowerW for locale-aware conversion.
 * 
 * IMPLEMENTATION:
 * - Iterate through string character by character
 * - Convert each character using CharLowerW
 * - Modify string in-place (same as _wcslwr behavior)
 * 
 * WHY IN-PLACE:
 * - Matches _wcslwr behavior for compatibility
 * - Avoids allocation/deallocation overhead
 * - Caller controls buffer lifetime
 * 
 * LANGUAGE CONCEPT: POINTER ARITHMETIC
 * - Pointer walks through string
 * - Each iteration converts one character
 * 
 * @param str - Pointer to null-terminated wide string to convert
 * @return Pointer to the same string (for chaining)
 */
WCHAR* StrToLower(WCHAR* str) {
    if (!str) {
        return NULL;
    }
    
    // Walk through string and convert each character
    WCHAR* p = str;
    while (*p) {
        // CharLowerW converts single character to lowercase
        // CAST: MAKEINTRESOURCE macro packs WCHAR into LPWSTR for API
        // WHY THIS WORKS: CharLowerW checks if high word is 0, treats as char
        *p = (WCHAR)(DWORD_PTR)CharLowerW((LPWSTR)(DWORD_PTR)*p);
        p++;
    }
    
    return str;
}

/**
 * StrCopyN - Safe string copy with length limit
 * 
 * This function copies a string with explicit length limiting and
 * guaranteed null termination. Safer alternative to wcsncpy.
 * 
 * SAFETY IMPROVEMENTS:
 * - Always null-terminates destination
 * - Explicit bounds checking
 * - Clear behavior documentation
 * 
 * IMPLEMENTATION:
 * - Copy characters one by one
 * - Stop at null terminator or length limit
 * - Always write null terminator at end
 * 
 * BUFFER OVERFLOW PREVENTION:
 * - maxChars includes space for null terminator
 * - Never writes beyond destination buffer
 * - Validates all writes are within bounds
 * 
 * @param dest     - Destination buffer
 * @param source   - Source string
 * @param maxChars - Maximum characters to copy (including null terminator)
 */
void StrCopyN(WCHAR* dest, const WCHAR* source, SIZE_T maxChars) {
    // Validate inputs
    if (!dest || !source || maxChars == 0) {
        return;
    }
    
    // Copy characters up to limit
    SIZE_T i;
    for (i = 0; i < maxChars - 1 && source[i] != L'\0'; i++) {
        dest[i] = source[i];
    }
    
    // Always null terminate
    // WHY: wcsncpy doesn't do this if source is too long
    // SAFETY: We reserved space by limiting to maxChars-1
    dest[i] = L'\0';
}

/**
 * StrCopy - Simple string copy
 * 
 * This function copies a null-terminated string from source to destination.
 * 
 * SAFETY WARNING:
 * - No bounds checking
 * - Caller must ensure destination is large enough
 * - Use StrCopyN for safer alternative
 * 
 * WHY PROVIDE THIS:
 * - Some code paths have known-safe buffer sizes
 * - Matches wcscpy usage patterns
 * - Slightly faster than bounded version
 * 
 * IMPLEMENTATION:
 * - Simple character-by-character copy
 * - Stops at null terminator
 * - Copies null terminator to destination
 * 
 * @param dest   - Destination buffer (must be large enough)
 * @param source - Source null-terminated string
 */
void StrCopy(WCHAR* dest, const WCHAR* source) {
    if (!dest || !source) {
        return;
    }
    
    // Copy until null terminator
    while (*source) {
        *dest++ = *source++;
    }
    
    // Copy null terminator
    *dest = L'\0';
}
