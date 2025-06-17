#pragma once
#include <allocator.h>
#include <data_structures/double_linked_list.h>

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

typedef struct {
    Node node;
    char* data;
} SlabNode;

// Forward declaration
typedef struct SlabAllocator SlabAllocator;

// SlabAllocator structure (extends Allocator)
struct SlabAllocator {
    Allocator base;
    char* managed_memory;
    unsigned int buffer_size;
    size_t slab_size;   
    size_t user_size;
    DoubleLinkedList* free_list;   
    unsigned int free_list_size;
    unsigned int free_list_size_max;
};

// Core allocator interface
void *SlabAllocator_init(Allocator* alloc, ...);
void *SlabAllocator_cleanup(Allocator* alloc, ...);
void *SlabAllocator_reserve(Allocator* alloc, ...);  
void *SlabAllocator_release(Allocator* alloc, ...);    

// Debug methods
void SlabAllocator_print_state(SlabAllocator* a);
void SlabAllocator_print_memory_map(SlabAllocator* a);
void print_slab_info(SlabAllocator* a, unsigned int slab_index);

// Callable methods

// Create a new SlabAllocator
inline SlabAllocator* SlabAllocator_create(SlabAllocator* a, size_t slab_size, size_t n_slabs) {
    #ifdef VERBOSE
    printf("\tCreating new SlabAllocator...\n");
    #endif
    
    // Validate input parameters
    if (!a || slab_size == 0 || n_slabs == 0) {
        #ifdef DEBUG
        printf(RED "ERROR: Failed to create: invalid parameters!\n" RESET);
        #endif
        return NULL;
    }

    // memset(a, 0, sizeof(SlabAllocator));
    if (!SlabAllocator_init((Allocator*)a, slab_size, n_slabs)) {
        #ifdef DEBUG
        printf(RED "ERROR: Failed to initialize SlabAllocator!\n" RESET);
        #endif
        return NULL;
    }
    #ifdef VERBOSE
    printf("\tSuccessfully created SlabAllocator\n");
    #endif
    return a;
}
// Destroy a SlabAllocator
inline int SlabAllocator_destroy(SlabAllocator* a) {
    #ifdef VERBOSE
    printf("\tDestroying SlabAllocator instance...\n");
    #endif
    if (!a) {
        #ifdef DEBUG
        printf(RED "ERROR: NULL allocator passed to SlabAllocator_destroy\n" RESET);
        #endif
        return -1;
    }
    
    void* result = ((Allocator*)a)->dest((Allocator*)a);
    if (!result) return -1;
    
    #ifdef VERBOSE
    printf("\tSlabAllocator instance destroyed\n");
    #endif
    return 0;
}
// Allocate a slab
inline void* SlabAllocator_malloc(SlabAllocator* a) {
    return ((Allocator*)a)->malloc((Allocator*)a);
}
// Free a slab
inline void SlabAllocator_free(SlabAllocator* a, void* ptr) {
    ((Allocator*)a)->free((Allocator*)a, ptr);
}