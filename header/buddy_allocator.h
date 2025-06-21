#pragma once
#include <allocator.h>
#include <slab_allocator.h>

#include <math.h>
#include <sys/mman.h>
#include <string.h>
#include <stdio.h>

#define MAX_LEVELS 32
#define MIN_BLOCK_SIZE 256
#define BUDDY_METADATA_SIZE sizeof(BuddyNode*)

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

// Debug methods
int BuddyAllocator_print_state(BuddyAllocator* a);

// Callable methods

// Create a new BuddyAllocator
inline BuddyAllocator* BuddyAllocator_create(BuddyAllocator* a, size_t total_size, int num_levels) {
    if (!a || total_size == 0 || num_levels == 0) {
        #ifdef DEBUG
        printf(RED "ERROR: Invalid parameters in create!\n" RESET);
        #endif
        return NULL;
    } 
       
    if (!BuddyAllocator_init((Allocator*)a, total_size, num_levels)) {
        #ifdef DEBUG
        printf(RED "ERROR: Failed to initialize BuddyAllocator!\n" RESET);
        #endif
        return NULL;
    }
    
    return a;
}

// Destroy BuddyAllocator
inline int BuddyAllocator_destroy(BuddyAllocator* a) {
    if (!a) {
        #ifdef DEBUG
        printf(RED "ERROR: NULL allocator in destroy\n" RESET);
        #endif
        return -1;
    }
    if (a->base.dest((Allocator*)a) != 0) {
        #ifdef DEBUG
        printf(RED "ERROR: Failed to destroy buddy allocator\n" RESET);
        #endif
        return -1;
    }
    return 0;
}

// Allocate memory from BuddyAllocator
inline void* BuddyAllocator_malloc(BuddyAllocator* a, size_t size) {
    if (!a || size == 0) {
        #ifdef DEBUG
        printf(RED "ERROR: NULL allocator or invalid size in alloc!\n" RESET);
        #endif
        return NULL;
    }

    size_t adjusted_size = size + BUDDY_METADATA_SIZE;
    // Align to 8 bytes
    adjusted_size = (adjusted_size + 7) & ~7;
    
    // Find appropriate block size
    size_t block_size = a->total_size;
    int level = 0;
    while (level < a->num_levels - 1 && block_size / 2 >= adjusted_size) {
        block_size /= 2;
        level++;
    }
    
    if (block_size < adjusted_size) {
        #ifdef DEBUG
        printf(RED "ERROR: Requested size too large (req: %zu, max: %zu)\n" RESET, 
               adjusted_size, a->total_size);
        #endif
        return NULL;
    }

    BuddyNode* node = a->base.malloc((Allocator*)a, level);
    if (!node) {
        #ifdef DEBUG
        printf(RED "ERROR: Failed to allocate node!\n" RESET);
        #endif
        return NULL;
    }
    node->is_free = false;

    // Store metadata at start of block
    void* user_data = (char*)node->data + BUDDY_METADATA_SIZE;
    *((BuddyNode**)((char*)user_data - BUDDY_METADATA_SIZE)) = node;

    return user_data;
}

// Release memory back to BuddyAllocator
inline int BuddyAllocator_free(BuddyAllocator* a, void* ptr) {
    if(!a || !ptr) {
        #ifdef DEBUG
        printf(RED "ERROR: NULL allocator or pointer in release\n" RESET);
        #endif
        return -1;
    }
    
    // Verify pointer is within allocator's memory range
    if ((char*)ptr < (char*)a->memory_start || 
        (char*)ptr >= (char*)a->memory_start + a->total_size) {
        #ifdef DEBUG
        printf(RED "ERROR: Pointer outside allocator memory range!\n" RESET);
        #endif
        return -1;
    }

    BuddyNode* node = *((BuddyNode**)((char*)ptr - BUDDY_METADATA_SIZE));
    if (!node || !node->data) {
        #ifdef DEBUG
        printf(RED "ERROR: Invalid metadata in release\n" RESET);
        #endif
        return -1;
    }
    
    if(node->is_free) {
        #ifdef DEBUG
        printf(RED "ERROR: Attempting to release an already free block\n" RESET);
        #endif
        return -1;
    }
    
    #ifdef VERBOSE
    printf("Releasing block at %p, size %zu, level %d\n", 
           (void*)node->data, node->size, node->level);
    #endif
    
    uint64_t r = (uint64_t)((Allocator*)a->base.free((Allocator*)a, node));
    if (r != 0) {
        #ifdef DEBUG
        printf(RED "ERROR: Failed to release block\n" RESET);
        #endif
        return -1;
    }
    
    #ifdef VERBOSE
    printf("Block released and merged if possible\n");
    #endif
    return 0;
}



