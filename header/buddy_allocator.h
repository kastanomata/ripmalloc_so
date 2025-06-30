#pragma once
#include <variable_block_allocator.h>
#include <slab_allocator.h>
#include <assert.h>

#include <math.h>
#include <sys/mman.h>
#include <string.h>
#include <stdio.h>

#define BUDDY_MAX_LEVELS 32
#define BUDDY_METADATA_SIZE sizeof(BuddyNode*)

typedef struct BuddyNode {
    Node node;
    char *data;
    size_t size; // Size of this block (including header)
    size_t requested_size; // Requested size (for logging)
    int level; // Level in the buddy system
    int is_free; // Whether this block is free
    struct BuddyNode* buddy; // Pointer to buddy block
    struct BuddyNode* parent; // Pointer to parent block
} BuddyNode;

struct Buddies {
    BuddyNode* left_buddy;
    BuddyNode* right_buddy;
};

typedef struct BuddyAllocator {
    VariableBlockAllocator base; // Base allocator interface
    void* memory_start; // Start of managed memory
    size_t memory_size; // Total size of managed memory
    size_t min_block_size; // Minimum block size (power of 2)
    int num_levels; // Number of levels in the system
    SlabAllocator list_allocator;
    SlabAllocator node_allocator;
    DoubleLinkedList** free_lists;  // Array of free lists for each level (in mmap)
} BuddyAllocator;

// Core allocator interface
void* BuddyAllocator_init(Allocator* alloc, ...);
void* BuddyAllocator_cleanup(Allocator* alloc, ...);
void* BuddyAllocator_reserve(Allocator* alloc, ...);
void* BuddyAllocator_release(Allocator* alloc, ...);

// Debug methods
int BuddyAllocator_print_state(BuddyAllocator* a);

// Callable methods

// Create a new BuddyAllocator
inline BuddyAllocator* BuddyAllocator_create(BuddyAllocator* a, size_t memory_size, int num_levels) {       
    if (!BuddyAllocator_init((Allocator*)a, memory_size, num_levels)) {
        #ifdef DEBUG
        printf(RED "ERROR: Failed to initialize BuddyAllocator!\n" RESET);
        #endif
        return NULL;
    }
    return a;
}

// Destroy BuddyAllocator
inline int BuddyAllocator_destroy(BuddyAllocator* a) {
    if (((Allocator*)a)->dest((Allocator*)a) != 0) {
        #ifdef DEBUG
        printf(RED "ERROR: Failed to destroy buddy allocator\n" RESET);
        #endif
        return -1;
    }
    return 0;
}

// Allocate memory from BuddyAllocator
inline void* BuddyAllocator_malloc(BuddyAllocator* a, size_t size) {    
    void* node = ((Allocator*)a)->malloc((Allocator*)a, size);
    if (!node) {
        #ifdef DEBUG
        printf(RED "ERROR: Failed to allocate node!\n" RESET);
        #endif
        return NULL;
    }
    return (BuddyNode*) node;
}

// Release memory back to BuddyAllocator
inline int BuddyAllocator_free(BuddyAllocator* a, void* ptr) {    
    uint64_t r = (uint64_t)(((Allocator*)a)->free((Allocator*)a, ptr));
    if (r != 0) {
        #ifdef DEBUG
        printf(RED "ERROR: Failed to release block\n" RESET);
        #endif
        return -1;
    }
    return 0;
}



