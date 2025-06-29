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
    int in_free_list;
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
    // memset(a, 0, sizeof(SlabAllocator));
    if (!SlabAllocator_init((Allocator*)a, slab_size, n_slabs)) {
        #ifdef DEBUG
        printf(RED "ERROR: Failed to initialize SlabAllocator!\n" RESET);
        #endif
        return NULL;
    }
    return a;
}
// Destroy a SlabAllocator
inline int SlabAllocator_destroy(SlabAllocator* a) {    
    void* result = ((Allocator*)a)->dest((Allocator*)a);
    if (!result) return -1;
    return 0;
}
// Allocate a slab
inline void* SlabAllocator_malloc(SlabAllocator* a) {
    return ((Allocator*)a)->malloc((Allocator*)a);
}
// Free a slab
inline int SlabAllocator_free(SlabAllocator* a, void* ptr) {
    void * r = ((Allocator*)a)->free((Allocator*)a, ptr);
    if(r == NULL)
        return -1;
    return 0;
}