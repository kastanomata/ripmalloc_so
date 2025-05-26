#include "slab_allocator.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

// Initialize SlabAllocator
void *SlabAllocator_init(Allocator* alloc, ...) {
    #ifdef VERBOSE
    printf("Initializing SlabAllocator...\n");
    #endif
    if (!alloc) return NULL;

    SlabAllocator* slab = (SlabAllocator*)alloc;
    va_list args;
    va_start(args, alloc);

    // Parse variadic args (slab_size, initial_slabs)
    slab->slab_size = va_arg(args, size_t);
    size_t initial_slabs = va_arg(args, size_t);
    va_end(args);

    #ifdef VERBOSE
    printf("Slab size: %zu bytes\n", slab->slab_size);
    printf("Initial slabs: %zu\n", initial_slabs);
    #endif

    // Calculate total memory needed including space for DoubleLinkedList
    size_t list_size = sizeof(DoubleLinkedList);
    size_t total_size = slab->slab_size * initial_slabs + list_size;
    // Round up to page size
    size_t page_size = sysconf(_SC_PAGESIZE);
    total_size = (total_size + page_size - 1) & ~(page_size - 1);
    #ifdef VERBOSE
    printf("Total memory needed: %zu bytes\n", total_size);
    #endif

    // Allocate memory using mmap
    slab->managed_memory = mmap(NULL, total_size, PROT_READ | PROT_WRITE,
                               MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (slab->managed_memory == MAP_FAILED) {
        #ifdef VERBOSE
        printf("Failed to allocate managed memory!\n");
        #endif
        return NULL;
    }
    #ifdef VERBOSE
    printf("Successfully allocated %zu bytes of managed memory\n", total_size);
    #endif

    slab->buffer_size = total_size;
    
    // Initialize free list at start of managed memory
    slab->free_list = (DoubleLinkedList*)slab->managed_memory;
    if (!list_create(slab->free_list)) {
        #ifdef VERBOSE
        printf("Failed to create free list!\n");
        #endif
        munmap(slab->managed_memory, total_size);
        return NULL;
    }
    #ifdef VERBOSE
    printf("Successfully created free list\n");
    #endif

    // Initialize free list with slabs
    #ifdef VERBOSE
    printf("Initializing free list with slabs...\n");
    #endif
    char* current = slab->managed_memory + list_size;
    for (size_t i = 0; i < initial_slabs; i++) {
        SlabNode* slab_node = (SlabNode*)current;
        slab_node->data = current + sizeof(SlabNode);
        list_push_front(slab->free_list, &slab_node->node);
        current += slab->slab_size;
        #ifdef VERBOSE
        printf("Added slab %zu/%zu to free list\n", i+1, initial_slabs);
        #endif
    }
    slab->free_list_size = initial_slabs;
    #ifdef VERBOSE
    printf("Free list initialized with %u slabs\n", slab->free_list_size);
    #endif

    // Setup interface methods
    alloc->init = SlabAllocator_init;
    alloc->dest = SlabAllocator_destructor;
    alloc->malloc = SlabAllocator_malloc;
    alloc->free = SlabAllocator_free;
    #ifdef VERBOSE
    printf("Interface methods configured\n");
    printf("SlabAllocator initialization complete\n");
    #endif
    return (void*)1;
}

// Clean up SlabAllocator
void *SlabAllocator_destructor(Allocator* alloc, ...) {
    #ifdef VERBOSE
    printf("Destroying SlabAllocator...\n");
    #endif
    if (!alloc) return NULL;

    SlabAllocator* slab = (SlabAllocator*)alloc;
    if (slab->managed_memory) {
        #ifdef VERBOSE
        printf("Unmapping managed memory (%u bytes)\n", slab->buffer_size);
        #endif
        munmap(slab->managed_memory, slab->buffer_size);
    }
    memset(slab, 0, sizeof(SlabAllocator));
    #ifdef VERBOSE
    printf("SlabAllocator destruction complete\n");
    #endif
    return (void*)1;
}

// Allocate a slab
void *SlabAllocator_malloc(Allocator* alloc, ...) {
    #ifdef VERBOSE
    printf("Attempting to allocate a slab...\n");
    #endif
    if (!alloc) return NULL;

    SlabAllocator* slab = (SlabAllocator*)alloc;
    if (slab->free_list->size == 0) {
        #ifdef VERBOSE
        printf("Failed to allocate: out of memory!\n");
        #endif
        return NULL;  // Out of memory
    }

    Node* node = slab->free_list->head;
    if (node) {
        // Remove from free list
        if (node->next) {
            node->next->prev = NULL;
        }
        slab->free_list->head = node->next;
        if (!slab->free_list->head) {
            slab->free_list->tail = NULL;
        }
        slab->free_list->size--;
        slab->free_list_size--;
    }

    SlabNode* slab_node = (SlabNode*)node;
    #ifdef VERBOSE
    printf("Successfully allocated slab (free slabs remaining: %u)\n", slab->free_list_size);
    #endif
    return (void*)slab_node->data;
}

// Free a slab
void *SlabAllocator_free(Allocator* alloc, ...) {
    #ifdef VERBOSE
    printf("Attempting to free a slab...\n");
    #endif
    if (!alloc) return NULL;

    SlabAllocator* slab = (SlabAllocator*)alloc;
    va_list args;
    va_start(args, alloc);
    void* ptr = va_arg(args, void*);
    va_end(args);

    // Validate pointer is within managed memory
    if ((char*)ptr < (char*)slab->managed_memory || 
        (char*)ptr >= (char*)slab->managed_memory + slab->buffer_size) {
        #ifdef VERBOSE
        printf("Failed to free: pointer outside managed memory range!\n");
        #endif
        return NULL;
    }

    // Convert data pointer back to SlabNode pointer
    SlabNode* slab_node = (SlabNode*)((char*)ptr - sizeof(SlabNode));
    
    // Add to front of free list
    Node* node = &slab_node->node;
    node->next = slab->free_list->head;
    node->prev = NULL;
    if (slab->free_list->head) {
        slab->free_list->head->prev = node;
    } else {
        slab->free_list->tail = node;
    }
    slab->free_list->head = node;
    slab->free_list->size++;
    slab->free_list_size++;
    
    #ifdef VERBOSE
    printf("Successfully freed slab (free slabs: %u)\n", slab->free_list_size);
    #endif
    return (void*)1;
}

// Create a new SlabAllocator
SlabAllocator* SlabAllocator_create(SlabAllocator* slab, size_t slab_size, size_t initial_slabs) {
    #ifdef VERBOSE
    printf("Creating new SlabAllocator...\n");
    #endif
    if (!slab) return NULL;

    memset(slab, 0, sizeof(SlabAllocator));
    if (!SlabAllocator_init((Allocator*)slab, slab_size, initial_slabs)) {
        #ifdef VERBOSE
        printf("Failed to initialize SlabAllocator!\n");
        #endif
        return NULL;
    }
    #ifdef VERBOSE
    printf("Successfully created SlabAllocator\n");
    #endif
    return slab;
}

// Destroy a SlabAllocator
void SlabAllocator_destroy(SlabAllocator* slab) {
    #ifdef VERBOSE
    printf("Destroying SlabAllocator instance...\n");
    #endif
    if (!slab) return;
    SlabAllocator_destructor((Allocator*)slab);
    #ifdef VERBOSE
    printf("SlabAllocator instance destroyed\n");
    #endif
}