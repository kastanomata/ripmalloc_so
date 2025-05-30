#pragma once
#include <allocator.h>  
#include <data_structures/double_linked_list.h>
#include <data_structures/slab_allocator.h>

#define MAX_LEVELS 4

// Forward declaration
typedef struct BuddyNode BuddyNode;

// Block header structure to track allocations
struct BuddyNode {
    Node node;        // Node for the double linked list
    char *data;
    size_t size;      // Size of the block including header
    size_t user_size; // Size of the block excluding header
    int level;        // Level in the buddy system
    int is_free;      // Whether the block is free
    BuddyNode* buddy; // Pointer to the buddy block
    BuddyNode* next; // Pointer to the next block
};

typedef struct BuddyAllocator {
    Allocator base;           // Base allocator interface
    int num_levels;          // Number of levels in the buddy system
    char* managed_memory;    // Start of the managed memory region
    size_t min_block_size;   // Size of smallest block (including header)
    size_t total_size;       // Total size of managed memory
    SlabAllocator slabs[MAX_LEVELS];  // Array of slab allocators for each level
} BuddyAllocator;

// Allocator interface functions
void* BuddyAllocator_init(Allocator* alloc, ...);
void* BuddyAllocator_destructor(Allocator* alloc, ...);
void* BuddyAllocator_malloc(Allocator* alloc, ...);
void* BuddyAllocator_free(Allocator* alloc, ...);

// Helper functions
BuddyAllocator* BuddyAllocator_create(BuddyAllocator* a, size_t num_levels, int buffer_size);
void BuddyAllocator_destroy(BuddyAllocator* a);
void* BuddyAllocator_alloc(BuddyAllocator* a, size_t size);
int BuddyAllocator_release(BuddyAllocator* a, void* ptr);
void BuddyAllocator_info(BuddyAllocator* a);

