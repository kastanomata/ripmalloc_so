#pragma once
#include <allocator.h>
#include <sys/mman.h>
#include <unistd.h>

typedef struct BuddyAllocator {
    Allocator base;
    size_t max_size;
    size_t block_size;
} BuddyAllocator;

// Constructor/Destructor
void *BuddyAllocator_init(Allocator* alloc, ...);
void *BuddyAllocator_destructor(Allocator* alloc, ...);

// Allocation/Free methods
void *BuddyAllocator_malloc(Allocator* alloc, ...);  
void *BuddyAllocator_free(Allocator* alloc, ...);    

BuddyAllocator* BuddyAllocator_create(BuddyAllocator* allocator, size_t block_size);
void BuddyAllocator_destroy(BuddyAllocator* allocator);
void* BuddyAllocator_alloc(BuddyAllocator* allocator, size_t size);
int BuddyAllocator_release(BuddyAllocator* allocator, void* ptr, size_t size);
void BuddyAllocator_info(BuddyAllocator* allocator);

