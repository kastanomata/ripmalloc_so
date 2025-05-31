#include "slab_allocator.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

// Calculate actual size needed for a slab including metadata
static inline size_t get_slab_total_size(size_t requested_size) {
    return sizeof(SlabNode) + requested_size;
}

// Initialize SlabAllocator
void *SlabAllocator_init(Allocator* alloc, ...) {
    #ifdef VERBOSE
    printf("\tInitializing SlabAllocator...\n");
    #endif
    if (!alloc) return NULL;

    SlabAllocator* slab = (SlabAllocator*)alloc;
    va_list args;
    va_start(args, alloc);

    // Parse variadic args (slab_size, n_slabs)
    size_t requested_size = va_arg(args, size_t);
    size_t n_slabs = va_arg(args, size_t);
    va_end(args);

    // Calculate actual slab size including metadata
    slab->slab_size = get_slab_total_size(requested_size);
    
    #ifdef VERBOSE
    printf("\tRequested slab size: %zu bytes\n", requested_size);
    printf("\tActual slab size with metadata: %zu bytes\n", slab->slab_size);
    printf("\tInitial slabs: %zu\n", n_slabs);
    #endif

    // Calculate total memory needed including space for DoubleLinkedList
    size_t list_size = sizeof(DoubleLinkedList);
    size_t total_size = slab->slab_size * n_slabs + list_size;
    // Round up to page size
    size_t page_size = sysconf(_SC_PAGESIZE);
    total_size = (total_size + page_size - 1) & ~(page_size - 1);
    #ifdef VERBOSE
    printf("\tTotal memory needed: %zu bytes\n", total_size);
    #endif

    // Allocate memory using mmap
    slab->managed_memory = mmap(NULL, total_size, PROT_READ | PROT_WRITE,
                               MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (slab->managed_memory == MAP_FAILED) {
        #ifdef VERBOSE
        printf("\tFailed to allocate managed memory!\n");
        #endif
        return NULL;
    }
    #ifdef VERBOSE
    printf("\tSuccessfully allocated %zu bytes of managed memory\n", total_size);
    #endif

    slab->buffer_size = total_size;
    
    // Initialize free list at start of managed memory
    slab->free_list = (DoubleLinkedList*)slab->managed_memory;
    if (!list_create(slab->free_list)) {
        #ifdef VERBOSE
        printf("\tFailed to create free list!\n");
        #endif
        munmap(slab->managed_memory, total_size);
        return NULL;
    }
    #ifdef VERBOSE
    printf("\tSuccessfully created free list\n");
    #endif

    // Initialize free list with slabs
    #ifdef VERBOSE
    printf("\tInitializing free list with slabs...\n");
    #endif
    char* current = slab->managed_memory + list_size;
    for (size_t i = 0; i < n_slabs; i++) {
        SlabNode* slab_node = (SlabNode*)current;
        // Clear the entire slab area
        // memset(current, 0, slab->slab_size);
        // Set data pointer to just after the SlabNode structure
        slab_node->data = current + sizeof(SlabNode);
        list_push_front(slab->free_list, &slab_node->node);
        current += slab->slab_size;
        #ifdef VERBOSE
        printf("\tAdded slab %zu/%zu to free list (node: %p, data: %p)\n", 
               i+1, n_slabs, (void*)slab_node, (void*)slab_node->data);
        #endif
    }
    slab->free_list_size = n_slabs;
    slab->free_list_size_max = n_slabs;
    #ifdef VERBOSE
    printf("\tFree list initialized with %u slabs\n", slab->free_list_size);
    #endif

    // Store the requested size for user allocations
    slab->user_size = requested_size;

    // Setup interface methods
    alloc->init = SlabAllocator_init;
    alloc->dest = SlabAllocator_destructor;
    alloc->malloc = SlabAllocator_malloc;
    alloc->free = SlabAllocator_free;
    #ifdef VERBOSE
    printf("\tInterface methods configured\n");
    printf("\tSlabAllocator initialization complete\n");
    #endif
    return (void*)1;
}

// Create a new SlabAllocator
SlabAllocator* SlabAllocator_create(SlabAllocator* a, size_t slab_size, size_t n_slabs) {
    #ifdef VERBOSE
    printf("\tCreating new SlabAllocator...\n");
    #endif
    
    // Validate input parameters
    if (!a || slab_size == 0 || n_slabs == 0) {
        #ifdef VERBOSE
        printf("\tFailed to create: invalid parameters!\n"); 
        #endif
        return NULL;
    }

    // memset(a, 0, sizeof(SlabAllocator));
    if (!SlabAllocator_init((Allocator*)a, slab_size, n_slabs)) {
        #ifdef VERBOSE
        printf("\tFailed to initialize SlabAllocator!\n");
        #endif
        return NULL;
    }
    #ifdef VERBOSE
    printf("\tSuccessfully created SlabAllocator\n");
    #endif
    return a;
}

// Clean up SlabAllocator
void *SlabAllocator_destructor(Allocator* alloc, ...) {
    #ifdef VERBOSE
    printf("\tDestroying SlabAllocator...\n");
    #endif
    if (!alloc) return NULL;

    SlabAllocator* slab = (SlabAllocator*)alloc;
    if (slab->managed_memory) {
        #ifdef VERBOSE
        printf("\tUnmapping managed memory (%u bytes)\n", slab->buffer_size);
        #endif
        munmap(slab->managed_memory, slab->buffer_size);
    }
    // memset(slab, 0, sizeof(SlabAllocator));
    #ifdef VERBOSE
    printf("\tSlabAllocator destruction complete\n");
    #endif
    return (void*)1;
}

// Destroy a SlabAllocator
int SlabAllocator_destroy(SlabAllocator* a) {
    #ifdef VERBOSE
    printf("\tDestroying SlabAllocator instance...\n");
    #endif
    if (!a) return -1;
    
    void* result = SlabAllocator_destructor((Allocator*)a);
    if (!result) return -1;
    
    #ifdef VERBOSE
    printf("\tSlabAllocator instance destroyed\n");
    #endif
    return 0;
}

// Allocate a slab
void *SlabAllocator_malloc(Allocator* alloc, ...) {
    #ifdef VERBOSE
    printf("\tAttempting to allocate a slab...\n");
    #endif
    if (!alloc) return NULL;

    SlabAllocator* slab = (SlabAllocator*)alloc;
    if (slab->free_list->size == 0) {
        #ifdef VERBOSE
        printf("\tFailed to allocate: out of memory!\n");
        #endif
        return NULL;  // Out of memory
    }

    Node* node = list_pop_front(slab->free_list);
    if (!node) {
        return NULL;
    }
    
    SlabNode* slab_node = (SlabNode*)node;
    slab->free_list_size--;

    // Clear the user data area
    // memset(slab_node->data, 0, slab->user_size);

    #ifdef VERBOSE
    printf("\tSuccessfully allocated slab (free slabs remaining: %u)\n", slab->free_list_size);
    printf("\tAllocated node: %p, data: %p\n", (void*)slab_node, (void*)slab_node->data);
    #endif
    return slab_node->data;
}

void* SlabAllocator_alloc(SlabAllocator* a) {
    return ((Allocator*)a)->malloc((Allocator*)a);
}

// Free a slab
void *SlabAllocator_free(Allocator* alloc, ...) {
    #ifdef VERBOSE
    printf("\tAttempting to free a slab...\n");
    #endif
    if (!alloc) return NULL;

    SlabAllocator* slab = (SlabAllocator*)alloc;
    va_list args;
    va_start(args, alloc);
    void* ptr = va_arg(args, void*);
    va_end(args);

    if (!ptr) {
        #ifdef VERBOSE
        printf("\tSkipping free of NULL pointer\n");
        #endif
        return NULL;
    }

    // Validate pointer is within managed memory
    if ((char*)ptr < (char*)slab->managed_memory || 
        (char*)ptr >= (char*)slab->managed_memory + slab->buffer_size) {
        #ifdef VERBOSE
        printf("\tFailed to free: pointer outside managed memory range!\n");
        printf("\tPointer: %p, Memory range: %p - %p\n", 
               ptr, slab->managed_memory, 
               (char*)slab->managed_memory + slab->buffer_size);
        #endif
        return NULL;
    }

    // Calculate the slab node pointer from the data pointer
    SlabNode* slab_node = (SlabNode*)((char*)ptr - sizeof(SlabNode));
    
    // Validate the slab node
    if (slab_node->data != ptr) {
        #ifdef VERBOSE
        printf("\tFailed to free: invalid slab pointer (data mismatch)!\n");
        printf("\tExpected data ptr: %p, Got: %p\n", 
               (void*)slab_node->data, ptr);
        #endif
        return NULL;
    }

    // Check if node is already in free list
    if (list_find(slab->free_list, &slab_node->node) != NULL) {
        #ifdef VERBOSE
        printf("\tSlab already in free list, skipping...\n");
        #endif
        return NULL;
    }
    
    // Clear the entire slab area before adding to free list
    // memset(slab_node, 0, slab->slab_size);
    slab_node->data = (char*)slab_node + sizeof(SlabNode);  // Restore data pointer
    
    // Add to free list
    list_push_front(slab->free_list, &slab_node->node);
    slab->free_list_size++;
    
    #ifdef VERBOSE
    printf("\tSuccessfully freed slab (free slabs: %u)\n", slab->free_list_size);
    printf("\tFreed node: %p, data: %p\n", (void*)slab_node, (void*)slab_node->data);
    #endif
    return (void*)1;
}

void SlabAllocator_release(SlabAllocator* a, void* ptr) {
    ((Allocator*)a)->free((Allocator*)a, ptr);
}

void SlabAllocator_info(SlabAllocator* a) {
    printf("\tSlabAllocator Info:\n");
    printf("\tSlab Size: %zu\n", a->slab_size);
    printf("\tSlots: %u/%u used\n", 
           a->free_list_size_max - a->free_list_size,
           a->free_list_size_max);
    printf("\tStatus: ");
    // Print visual representation of used/free slots
    for (unsigned int i = 0; i < a->free_list_size_max; i++) {
        if (i < a->free_list_size)
            printf("□"); // Free slot
        else
            printf("■"); // Used slot
    }
    printf("\n");
}

void SlabAllocator_print_memory_map(SlabAllocator* a) {
    printf("\tSlabAllocator Visualization:\n");
    if (!a || !a->managed_memory) {
        printf("\tNo memory to visualize\n");
        return;
    }

    printf("\tMemory range: %p - %p\n", (void*)a->managed_memory, 
           (void*)(a->managed_memory + a->buffer_size));
    
    for (unsigned int i = 0; i < a->free_list_size_max; i++) {
        print_slab_info(a, i);
    }

}

// Print each slab's status
void print_slab_info(SlabAllocator* a, unsigned int slab_index) {
    char* slab_start = a->managed_memory + (slab_index * a->slab_size);
    printf("\nSlab %u [%p]:\n", slab_index, (void*)slab_start);
    
    // Check if slab is in free list
    int is_free = 0;
    Node* current = a->free_list->head;
    while (current) {
        SlabNode* slab_node = (SlabNode*)current;
        if (slab_node->data == slab_start) {
            is_free = 1;
            break;
        }
        current = current->next;
    }
    
    printf("\tStatus: %s\n", is_free ? "\tFREE" : "\tUSED");
    
    // Print all slab content in hex
    printf("\tContent: \n");
    for (size_t j = 0; j < a->slab_size; j++) {
        if (j % 16 == 0) printf("\t%04zx: ", j);
        printf("%02x ", (unsigned char)slab_start[j]);
        if ((j + 1) % 8 == 0) printf(" ");
        if ((j + 1) % 16 == 0) printf("\n");
    }
    // Print newline if we didn't end on a 16-byte boundary
    if (a->slab_size % 16 != 0) printf("\n");
}


