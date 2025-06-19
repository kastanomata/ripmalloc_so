#pragma once
#include <allocator.h>
#include <data_structures/bitmap.h>
#include <math.h>

#define BITMAP_BUDDY_MAX_LEVELS 20
#define BITMAP_METADATA_SIZE (2 * sizeof(int))  // Metadata size for each allocation

typedef struct {
    Allocator base;          // Base allocator interface
    char* memory_start;      // Managed memory area
    int memory_size;         // Size of managed memory
    int num_levels;          // Number of levels in the hierarchy
    int min_bucket_size;     // Minimum allocation size
    Bitmap bitmap;           // Bitmap tracking block status
} BitmapBuddyAllocator;

// Core allocator interface
void* BitmapBuddyAllocator_init(Allocator* alloc, ...);
void* BitmapBuddyAllocator_cleanup(Allocator* alloc, ...);
void* BitmapBuddyAllocator_reserve(Allocator* alloc, ...);
void* BitmapBuddyAllocator_release(Allocator* alloc, ...);

// Helper function to create the allocator
inline BitmapBuddyAllocator* BitmapBuddyAllocator_create(BitmapBuddyAllocator* alloc, size_t total_size, int num_levels) {
    if(!alloc || total_size == 0 || num_levels <= 0) {
        #ifdef DEBUG
        printf(RED "Error: Invalid parameters for BitmapBuddyAllocator_create\n" RESET);
        #endif
        return NULL;
    }
    if (num_levels >= BITMAP_BUDDY_MAX_LEVELS) {
        #ifdef DEBUG
        printf(RED "Error: Number of levels exceeds maximum (%d)\n" RESET, BITMAP_BUDDY_MAX_LEVELS);
        #endif
        return NULL;
    }
    if (!BitmapBuddyAllocator_init((Allocator*)alloc, total_size, num_levels)) {
        #ifdef DEBUG
        printf(RED "Error: Failed to initialize BitmapBuddyAllocator\n" RESET);
        #endif
        return NULL;
    }
    return alloc;
}
inline int BitmapBuddyAllocator_destroy(BitmapBuddyAllocator* alloc) {
    if (!alloc) {
        #ifdef DEBUG
        printf(RED "Error: NULL allocator in BitmapBuddyAllocator_destroy\n" RESET);
        #endif
        return -1;
    }
    
    if (alloc->base.dest((Allocator*)alloc) != NULL) {
        #ifdef DEBUG
        printf(RED "Error: Failed to destroy BitmapBuddyAllocator\n" RESET);
        #endif
        return -1;
    }
    return 0;
}
inline void* BitmapBuddyAllocator_malloc(BitmapBuddyAllocator* alloc, size_t size) {
    if (!alloc || size <= 0) {
        #ifdef DEBUG
        printf(RED "Error: NULL allocator or invalid size\n" RESET);
        #endif
        return NULL;
    }
    
    void* memory_start = alloc->base.malloc((Allocator*)alloc, size);
    if( memory_start == NULL ) {
        #ifdef DEBUG
        printf(RED "Error: Failed to allocate memory\n" RESET);
        #endif
        return NULL;
    }
    return memory_start;
}
inline int BitmapBuddyAllocator_free(BitmapBuddyAllocator* alloc, void* ptr) {
    if (!alloc || !ptr) {
        #ifdef DEBUG
        printf(RED "Error: NULL allocator or pointer in BitmapBuddyAllocator_free\n" RESET);
        #endif
        return -1;
    }
    // Retrieve metadata
    int* metadata = (int*)((char*)ptr - 2 * sizeof(int));
    int bit = metadata[0];
    // Check if the pointer is already free
    if (bitmap_test(&alloc->bitmap, bit) == 0) {
        #ifdef DEBUG
        printf(RED "Error: Memory already free!\n" RESET);
        #endif
        return -1;
    }
    if (alloc->base.free((Allocator*)alloc, ptr) == (void*) -1) {
        #ifdef DEBUG
        printf(RED "Error: Failed to free memory in BitmapBuddyAllocator_free\n" RESET);
        #endif
        return -1;
    }
    return 0;
}

// Debug/Info functions
int BitmapBuddyAllocator_print_state(BitmapBuddyAllocator* alloc);