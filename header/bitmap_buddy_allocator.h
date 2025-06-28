#pragma once
#include <variable_block_allocator.h>
#include <data_structures/bitmap.h>
#include <math.h>

#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include <sys/mman.h>

#define BITMAP_BUDDY_MAX_LEVELS 32

#define RELEASED 0
#define RESERVED 1

typedef struct {
    int bitmap_idx;
    int size;
} BitmapBuddyMetadata;

#define BITMAP_METADATA_SIZE sizeof(BitmapBuddyMetadata)  // Metadata size for each allocation

typedef struct {
    VariableBlockAllocator base;          // Base allocator interface
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
    if (!BitmapBuddyAllocator_init((Allocator*)alloc, total_size, num_levels)) {
        #ifdef DEBUG
        printf(RED "Error: Failed to initialize BitmapBuddyAllocator\n" RESET);
        #endif
        return NULL;
    }
    return alloc;
}

inline int BitmapBuddyAllocator_destroy(BitmapBuddyAllocator* alloc) {   
    if (((Allocator*) alloc)->dest((Allocator*)alloc) != NULL) {
        #ifdef DEBUG
        printf(RED "Error: Failed to destroy BitmapBuddyAllocator\n" RESET);
        #endif
        return -1;
    }
    return 0;
}

inline void* BitmapBuddyAllocator_malloc(BitmapBuddyAllocator* alloc, size_t size) {
    void* ptr = ((Allocator*) alloc)->malloc((Allocator*)alloc, size);
    if( ptr == NULL ) {
        #ifdef DEBUG
        printf(RED "Error: Failed to allocate memory\n" RESET);
        #endif
        return NULL;
    }
    return ptr;
}
inline int BitmapBuddyAllocator_free(BitmapBuddyAllocator* alloc, void* ptr) {
    if (((Allocator*) alloc)->free((Allocator*)alloc, ptr) == (void*) -1) {
        #ifdef DEBUG
        printf(RED "Error: Failed to free memory in BitmapBuddyAllocator_free\n" RESET);
        #endif
        return -1;
    }
    return 0;
}

// Debug/Info functions
int BitmapBuddyAllocator_print_state(BitmapBuddyAllocator* alloc);
static void print_bitmap_status(BitmapBuddyAllocator* buddy) {
    printf("Bitmap Status:\n");
    for (int i = 0; i < buddy->bitmap.num_bits; i++) {
        printf("%c ", bitmap_test(&buddy->bitmap, i) ? 'A' : 'F');
        if ((i + 1) && !((i + 2) & (i + 1))) printf("| ");
    }
    printf("\n");
}