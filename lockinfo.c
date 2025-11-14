/*
 * lockinfo.c - Dynamic array management for file lock information
 * 
 * This file implements a dynamic array (vector) data structure for storing
 * FileLockInfo structures. The array automatically grows as needed.
 */

#include <stdlib.h>    // malloc, realloc, free
#include <string.h>    // memcpy
#include "lockinfo.h"     // FileLockInfo, LockArray definitions

// Initial capacity for the array (will grow as needed)
// Chosen to balance memory usage vs. reallocation frequency
#define INITIAL_CAPACITY 100

/**
 * Initialize a new LockArray
 * 
 * @param array - Pointer to LockArray structure to initialize
 * 
 * Allocates initial memory for the array and sets count to 0.
 * Call LockArray_Free() when done to prevent memory leaks.
 */
void LockArray_Init(LockArray* array) {
    // Allocate initial block of memory
    // Multiply capacity by size of one FileLockInfo structure
    array->items = (FileLockInfo*)malloc(INITIAL_CAPACITY * sizeof(FileLockInfo));
    
    // Check if allocation succeeded
    if (!array->items) {
        // Allocation failed - set to safe defaults
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
 * @param array - Pointer to LockArray to add to
 * @param lock  - Pointer to FileLockInfo to copy into array
 * 
 * The array will automatically grow (double in size) if it's full.
 * If reallocation fails, the function returns without adding the item.
 */
void LockArray_Add(LockArray* array, const FileLockInfo* lock) {
    // Check if we need to resize the array
    if (array->count >= array->capacity) {
        // Array is full - need to grow
        // Double the capacity (amortized O(1) insertion cost)
        size_t newCapacity = array->capacity * 2;
        
        // Attempt to reallocate with new size
        FileLockInfo* newItems = (FileLockInfo*)realloc(
            array->items, 
            newCapacity * sizeof(FileLockInfo)
        );
        
        // Check if reallocation succeeded
        if (!newItems) {
            // Reallocation failed - cannot add item
            // In production, you might want to log this or show an error
            return;
        }
        
        // Update array with new memory block and capacity
        array->items = newItems;
        array->capacity = newCapacity;
    }
    
    // Copy the lock data into the array at the current count position
    // Using memcpy is safe here because FileLockInfo contains no pointers
    memcpy(&array->items[array->count], lock, sizeof(FileLockInfo));
    
    // Increment count to reflect the new item
    array->count++;
}

/**
 * Clear all items from the array (but keep memory allocated)
 * 
 * @param array - Pointer to LockArray to clear
 * 
 * This sets count to 0 but doesn't free memory. Useful for reusing
 * the array without reallocating. Memory is still allocated and can
 * be reused immediately.
 */
void LockArray_Clear(LockArray* array) {
    // Simply reset count to 0
    // Memory remains allocated, ready for reuse
    array->count = 0;
}

/**
 * Free all memory associated with the array
 * 
 * @param array - Pointer to LockArray to free
 * 
 * This must be called when done with the array to prevent memory leaks.
 * After calling, the array should not be used unless LockArray_Init()
 * is called again.
 */
void LockArray_Free(LockArray* array) {
    // Free the memory block if it was allocated
    if (array->items != NULL) {
        free(array->items);
        array->items = NULL;  // Prevent use-after-free bugs
    }
    
    // Reset all fields to safe defaults
    array->count = 0;
    array->capacity = 0;
}

