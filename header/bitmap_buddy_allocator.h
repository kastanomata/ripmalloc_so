#pragma once
#include <allocator.h>
#include <data_structures/bitmap.h>
#include <math.h>

#define BITMAP_BUDDY_MAX_LEVELS 20

typedef struct {
    Allocator base;          // Base allocator interface
    char* memory;            // Managed memory area
    int memory_size;         // Size of managed memory
    int num_levels;          // Number of levels in the hierarchy
    int min_bucket_size;     // Minimum allocation size
    Bitmap bitmap;           // Bitmap tracking block status
} BitmapBuddyAllocator;

// Core allocator interface
void* BitmapBuddyAllocator_init(Allocator* alloc, ...);
void* BitmapBuddyAllocator_destructor(Allocator* alloc, ...);
void* BitmapBuddyAllocator_malloc(Allocator* alloc, ...);
void* BitmapBuddyAllocator_free(Allocator* alloc, ...);

// Helper function to create the allocator
BitmapBuddyAllocator* BitmapBuddyAllocator_create(BitmapBuddyAllocator* alloc, 
                                                 size_t total_size, 
                                                 int num_levels);