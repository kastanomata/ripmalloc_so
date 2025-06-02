#pragma once
#include <allocator.h>
#include <slab_allocator.h>

#include <math.h>
#include <sys/mman.h>
#include <string.h>
#include <stdio.h>

#define MAX_LEVELS 32
#define MIN_BLOCK_SIZE 256

typedef struct BuddyNode {
    Node node;
    char *data;
    size_t size;           // Size of this block (including header)
    int level;            // Level in the buddy system
    int is_free;          // Whether this block is free
    struct BuddyNode* buddy;  // Pointer to buddy block
    struct BuddyNode* parent;   // Pointer to parent block
} BuddyNode;

struct Buddies {
    BuddyNode* left_buddy;
    BuddyNode* right_buddy;
};

typedef struct BuddyAllocator {
    Allocator base;           // Base allocator interface
    void* memory_start;       // Start of managed memory
    size_t total_size;        // Total size of managed memory
    size_t min_block_size;    // Minimum block size (power of 2)
    int num_levels;          // Number of levels in the system
    SlabAllocator list_allocator;
    SlabAllocator node_allocator;
    DoubleLinkedList* free_lists[MAX_LEVELS];  // Array of free lists for each level
} BuddyAllocator;

// Core allocator interface
void* BuddyAllocator_init(Allocator* alloc, ...);
void* BuddyAllocator_cleanup(Allocator* alloc, ...);
void* BuddyAllocator_reserve(Allocator* alloc, ...);
void* BuddyAllocator_release(Allocator* alloc, ...);

// Helper functions
BuddyAllocator* BuddyAllocator_create(BuddyAllocator* a, size_t total_size, int num_levels);
int BuddyAllocator_destroy(BuddyAllocator* a);
void* BuddyAllocator_malloc(BuddyAllocator* a, size_t size);
void BuddyAllocator_free(BuddyAllocator* a, void* ptr);

// Debug/Info functions
int BuddyAllocator_print_state(BuddyAllocator* a);
void BuddyAllocator_validate(BuddyAllocator* a);


