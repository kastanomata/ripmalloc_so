#include <buddy_allocator.h>

void *BuddyAllocator_init(Allocator* alloc, ...) {
    va_list args;
    va_start(args, alloc);
    BuddyAllocator* allocator = va_arg(args, BuddyAllocator*);
    size_t block_size = va_arg(args, size_t);
    va_end(args);
    BuddyAllocator* buddy = (BuddyAllocator *)alloc;
    if (!allocator) return NULL;
    #ifdef VERBOSE
    printf("\tInitializing buddy allocator with block size %zu\n", block_size);
    #endif

    // Initialize base allocator fields
    buddy->base.init = BuddyAllocator_init;
    buddy->base.dest =  BuddyAllocator_destructor;
    buddy->base.malloc = BuddyAllocator_malloc;
    buddy->base.free = BuddyAllocator_free;

    // Initialize buddy allocator specific fields
    buddy->block_size = block_size;
    buddy->max_size = sysconf(_SC_PAGESIZE) / 4;

    #ifdef VERBOSE
    printf("\tBuddy allocator initialized with max size %zu\n", buddy->max_size);
    #endif

    return (void*) allocator;

}
void *BuddyAllocator_destructor(Allocator* alloc, ...) {
    // BuddyAllocator* buddy = (BuddyAllocator *)alloc;
    (void)alloc;  // Suppress unused parameter warning
    #ifdef VERBOSE
    printf("\tDestroying buddy allocator\n");
    #endif
    // TODO: Implement actual buddy deallocation logic
    return (void*) 0;

}

void *BuddyAllocator_malloc(Allocator* alloc, ...) {
    va_list args;
    va_start(args, alloc);
    size_t size = va_arg(args, size_t);
    va_end(args);
    // BuddyAllocator* buddy = (BuddyAllocator *)alloc;
    #ifdef VERBOSE
    printf("\tAllocating memory of size %zu\n", size);
    #endif
    // Allocate memory using mmap
    void* ptr = mmap(NULL, size, 
                    PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS,
                    -1, 0);
    
    if (ptr == MAP_FAILED) {
        #ifdef VERBOSE
        printf("\tMemory allocation failed\n");
        #endif
        return NULL;
    }
    
    #ifdef VERBOSE
    printf("\tMemory allocated at address %p\n", ptr);
    #endif
    return ptr;

}

void *BuddyAllocator_free(Allocator* alloc, ...) {
    va_list args;
    va_start(args, alloc);
    void* ptr = va_arg(args, void*);
    size_t size = va_arg(args, size_t);
    va_end(args);
    // BuddyAllocator* buddy = (BuddyAllocator *)alloc;
    #ifdef VERBOSE
    printf("\tFreeing memory of size %zu at address %p\n", size, ptr);
    #endif
    
    int result = munmap(ptr, size);
    if (result == -1) {
        #ifdef VERBOSE
        printf("\tFailed to free memory\n");
        #endif
        return (void*) -1;
    }
    #ifdef VERBOSE
    printf("\tMemory freed successfully\n");
    #endif
    return (void*) 0;
} 


BuddyAllocator* BuddyAllocator_create(BuddyAllocator* allocator, size_t block_size) {  
    #ifdef VERBOSE
    printf("\tCreating buddy allocator with block size %zu\n", block_size);
    #endif

    if (!allocator || !block_size) {
        #ifdef VERBOSE
        printf("\tInvalid parameters for buddy allocator creation\n");
        #endif
        return NULL;
    }
    BuddyAllocator* buddy_allocator = (BuddyAllocator*) BuddyAllocator_init((Allocator*)allocator, block_size);
    if (!buddy_allocator) {
        #ifdef VERBOSE
        printf("\tFailed to initialize buddy allocator\n");
        #endif
        return NULL;
    }

    #ifdef VERBOSE
    printf("\tBuddy allocator created successfully\n");
    #endif
    return buddy_allocator;

}

void BuddyAllocator_destroy(BuddyAllocator* allocator) {
    #ifdef VERBOSE
    printf("\tDestroying buddy allocator at address %p\n", (void*)allocator);
    #endif
    if (!allocator) {
        #ifdef VERBOSE
        printf("\tNull allocator provided, nothing to destroy\n");
        #endif
        return;
    }
    BuddyAllocator_destructor((Allocator*)allocator);
}

void* BuddyAllocator_alloc(BuddyAllocator* allocator, size_t size) {
    #ifdef VERBOSE
    printf("\tRequesting allocation of size %zu\n", size);
    #endif
    if (!allocator || size == 0 ||size <= allocator->max_size) {
        #ifdef VERBOSE
        printf("\tInvalid allocation parameters\n");
        #endif
        return NULL;
    }
    void* ptr = BuddyAllocator_malloc((Allocator*)allocator, size);
    if (!ptr) {
        #ifdef VERBOSE
        printf("\tAllocation failed\n");
        #endif
        return NULL;
    }
    #ifdef VERBOSE
    printf("\tAllocation successful at address %p\n", ptr);
    #endif
    return ptr;
}

int BuddyAllocator_release(BuddyAllocator* allocator, void* ptr, size_t size) {
    #ifdef VERBOSE
    printf("\tAttempting to release memory at address %p\n", ptr);
    #endif
    if (!allocator || !ptr) {
        #ifdef VERBOSE
        printf("\tInvalid release parameters\n");
        #endif
        return -1;
    }
    void* result = BuddyAllocator_free((Allocator*)allocator, ptr, size);
    if (result == (void*) -1) {
        #ifdef VERBOSE
        printf("\tFailed to free memory\n");
        #endif
        return -1;
    }
    #ifdef VERBOSE
    printf("\tMemory released successfully\n");
    #endif
    return 0;
}

void BuddyAllocator_info(BuddyAllocator* allocator) {
    if (!allocator) {
        printf("Error: Null allocator provided\n");
        return;
    }
    
    // Validate allocator fields before accessing
    if (allocator->block_size == 0 || allocator->max_size == 0) {
        printf("Error: Invalid allocator state\n");
        return;
    }

    // Print info with additional error checking
    printf("Buddy Allocator Info:\n");
    printf("Block Size: %zu bytes\n", allocator->block_size);
    printf("Max Size: %zu bytes\n", allocator->max_size);
}