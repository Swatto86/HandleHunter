/*
 * lockinfo.c - Dynamic array management for file lock information
 * 
 * This file implements a dynamic array (vector) data structure for storing
 * FileLockInfo structures. The array automatically grows as needed.
 * 
 * MEMORY MANAGEMENT:
 * - Uses Windows Heap API (HeapAlloc/HeapFree) instead of CRT malloc/free
 * - No CRT dependencies - pure Windows API implementation
 * - Manual memory management with explicit allocation/deallocation
 */

#include <windows.h>      // Core Windows API
#include "lockinfo.h"     // FileLockInfo, LockArray definitions
#include "utilities.h"    // CRT-free memory functions (MemAlloc, MemCopy, etc.)

// Initial capacity for the array (will grow as needed)
// Chosen to balance memory usage vs. reallocation frequency
#define INITIAL_CAPACITY 100

/**
 * Initialize a new LockArray
 * 
 * This function allocates initial heap memory for the dynamic array.
 * Uses Windows Heap API instead of CRT malloc.
 * 
 * MEMORY MANAGEMENT:
 * - Allocates from process default heap using HeapAlloc
 * - WHY HEAP API: No CRT dependency, efficient for small allocations
 * - Must call LockArray_Free() to prevent memory leaks
 * 
 * ERROR HANDLING:
 * - If allocation fails, sets array to safe defaults
 * - Safe defaults: NULL pointer, zero count/capacity
 * - Array functions will handle NULL gracefully
 * 
 * @param array - Pointer to LockArray structure to initialize
 */
void LockArray_Init(LockArray* array) {
    // Allocate initial block of memory from process heap
    // FUNCTION LINKAGE: MemAlloc uses HeapAlloc from kernel32.lib
    // CAST: HeapAlloc returns void*, cast to our structure type
    array->items = (FileLockInfo*)MemAlloc(INITIAL_CAPACITY * sizeof(FileLockInfo));
    
    // Check if allocation succeeded
    // WHY CHECK: Heap can be full, system low on memory
    if (!array->items) {
        // Allocation failed - set to safe defaults
        // SAFETY: All other functions check for NULL before use
        array->items = NULL;
        array->count = 0;
        array->capacity = 0;
        return;
    }
    
    // Initialize array state
    array->count = 0;                    // No items yet
    array->capacity = INITIAL_CAPACITY;  // Can hold INITIAL_CAPACITY items
}

/**
 * Add a FileLockInfo to the array
 * 
 * This function adds a new lock to the array, automatically growing
 * the array if needed. Uses Windows Heap API for memory management.
 * 
 * MEMORY MANAGEMENT:
 * - Array doubles in size when full (amortized O(1) complexity)
 * - Uses HeapReAlloc to resize existing allocation
 * - WHY DOUBLING: Balances memory usage vs. reallocation frequency
 * 
 * REALLOCATION STRATEGY:
 * - HeapReAlloc may move the block to a new address
 * - If realloc fails, original block remains valid
 * - We check return value before updating array pointer
 * 
 * ERROR HANDLING:
 * - If reallocation fails, item is not added
 * - Function returns silently (caller can check if count increased)
 * - Original array data remains intact on failure
 * 
 * SAFETY:
 * - MemCopy checks are not needed (FileLockInfo has no pointers)
 * - Structure contains only POD types (Plain Old Data)
 * - Simple byte-wise copy is safe
 * 
 * @param array - Pointer to LockArray to add to
 * @param lock  - Pointer to FileLockInfo to copy into array
 */
void LockArray_Add(LockArray* array, const FileLockInfo* lock) {
    // Check if we need to resize the array
    // WHY >= INSTEAD OF ==: Handles edge case where capacity is 0
    if (array->count >= array->capacity) {
        // Array is full - need to grow
        // Double the capacity (amortized O(1) insertion cost)
        // WHY CAST: Ensure size_t arithmetic (avoid overflow on DWORD)
        SIZE_T newCapacity = (SIZE_T)array->capacity * 2;
        
        // Attempt to reallocate with new size
        // FUNCTION LINKAGE: MemRealloc uses HeapReAlloc from kernel32.lib
        // CAST: HeapReAlloc returns void*, cast to our structure type
        FileLockInfo* newItems = (FileLockInfo*)MemRealloc(
            array->items, 
            newCapacity * sizeof(FileLockInfo)
        );
        
        // Check if reallocation succeeded
        // WHY CHECK: Heap may be fragmented, system low on memory
        if (!newItems) {
            // Reallocation failed - cannot add item
            // SAFETY: Original array->items pointer is still valid
            // NOTE: In production, consider logging this error
            return;
        }
        
        // Update array with new memory block and capacity
        // SAFETY: Only update pointer after successful reallocation
        array->items = newItems;
        array->capacity = (DWORD)newCapacity;
    }
    
    // Copy the lock data into the array at the current count position
    // FUNCTION LINKAGE: MemCopy is our CRT-free memcpy replacement
    // WHY SAFE: FileLockInfo contains no pointers, byte copy is valid
    MemCopy(&array->items[array->count], lock, sizeof(FileLockInfo));
    
    // Increment count to reflect the new item
    // SAFETY: We ensured capacity > count before reaching here
    array->count++;
}

/**
 * Clear all items from the array (but keep memory allocated)
 * 
 * This sets count to 0 but doesn't free memory. Useful for reusing
 * the array without reallocating. Memory is still allocated and can
 * be reused immediately.
 * 
 * WHY THIS IS USEFUL:
 * - Refresh operation clears and refills array repeatedly
 * - Keeping memory allocated avoids reallocation overhead
 * - Much faster than Free() + Init()
 * 
 * @param array - Pointer to LockArray to clear
 */
void LockArray_Clear(LockArray* array) {
    // Simply reset count to 0
    // Memory remains allocated, ready for reuse
    // WHY: Memory can be reused, avoiding reallocation overhead
    array->count = 0;
}

/**
 * Free all memory associated with the array
 * 
 * This function releases heap memory back to the system.
 * Uses Windows Heap API instead of CRT free.
 * 
 * MEMORY MANAGEMENT:
 * - Uses HeapFree to return memory to process heap
 * - WHY HEAP API: Matches allocation method (HeapAlloc)
 * - Must be called to prevent memory leaks
 * 
 * SAFETY:
 * - Handles NULL pointer gracefully (MemFree checks this)
 * - Sets pointer to NULL after freeing (prevents use-after-free)
 * - Resets count/capacity to safe defaults
 * 
 * USAGE:
 * - Call when done with array (before program exit, window close)
 * - After calling, must call LockArray_Init() before using again
 * - Safe to call multiple times (NULL pointer check prevents double-free)
 * 
 * @param array - Pointer to LockArray to free
 */
void LockArray_Free(LockArray* array) {
    // Free the memory block if it was allocated
    // FUNCTION LINKAGE: MemFree uses HeapFree from kernel32.lib
    if (array->items != NULL) {
        MemFree(array->items);
        // SAFETY: Set to NULL to prevent use-after-free bugs
        // WHY IMPORTANT: If code accidentally accesses after free,
        //                NULL dereference is easier to debug than dangling pointer
        array->items = NULL;
    }
    
    // Reset all fields to safe defaults
    // WHY: Makes array safe to inspect even after freeing
    array->count = 0;
    array->capacity = 0;
}

