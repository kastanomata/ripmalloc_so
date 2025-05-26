#pragma once
#include <allocator.h>
#include <data_structures/double_linked_list.h>

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
    size_t slab_size;    // Size of each slab
    DoubleLinkedList* free_list;    // Linked list of free blocks
    unsigned int free_list_size;
    unsigned int free_list_size_max;
};

// Constructor/Destructor
void *SlabAllocator_init(Allocator* alloc, ...);
void *SlabAllocator_destructor(Allocator* alloc, ...);

// Allocation/Free methods
void *SlabAllocator_malloc(Allocator* alloc, ...);  
void *SlabAllocator_free(Allocator* alloc, ...);    

// Helper to initialize a SlabAllocator
SlabAllocator* SlabAllocator_create(SlabAllocator* slab, size_t slab_size, size_t initial_slabs);
void SlabAllocator_destroy(SlabAllocator* slab);
void* SlabAllocator_alloc(SlabAllocator* slab);
void SlabAllocator_release(SlabAllocator* slab, void* ptr);
