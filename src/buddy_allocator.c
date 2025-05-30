#include <buddy_allocator.h>
#include <math.h>
#include <sys/mman.h>
#include <string.h>

void* BuddyAllocator_init(Allocator* alloc, ...) {
    va_list args;
    va_start(args, alloc);
    
    BuddyAllocator* buddy = (BuddyAllocator*)alloc;
    int num_levels = va_arg(args, int);
    int buffer_size = va_arg(args, int);
    
    va_end(args);
    
    buddy->num_levels = num_levels;
    buddy->total_size = buffer_size;
    buddy->min_block_size = buffer_size / (1 << (num_levels - 1));

    // Initialize slab allocators for each level
    for (int i = 0; i < num_levels; i++) {
        size_t slab_size = buddy->min_block_size * (1 << i);
        size_t num_slabs = buddy->total_size / slab_size;
        
        if (!SlabAllocator_create(&buddy->slabs[i], slab_size, num_slabs)) {
            // Cleanup previously created slab allocators
            for (int j = 0; j < i; j++) {
                SlabAllocator_destroy(&buddy->slabs[j]);
            }
            return NULL;
        }
    }

    // Setup interface methods
    alloc->init = BuddyAllocator_init;
    alloc->dest = BuddyAllocator_destructor;
    alloc->malloc = BuddyAllocator_malloc;
    alloc->free = BuddyAllocator_free;

    return buddy;
}

void* BuddyAllocator_destructor(Allocator* alloc, ...) {
    if (!alloc) return NULL;

    BuddyAllocator* buddy = (BuddyAllocator*)alloc;
    
    // Destroy all slab allocators
    for (int i = 0; i < buddy->num_levels; i++) {
        SlabAllocator_destroy(&buddy->slabs[i]);
    }

    return (void*)1;
}

void* BuddyAllocator_malloc(Allocator* alloc, ...) {
    if (!alloc) return NULL;

    va_list args;
    va_start(args, alloc);
    size_t size = va_arg(args, size_t);
    va_end(args);

    BuddyAllocator* buddy = (BuddyAllocator*)alloc;

    // Find the appropriate level for this size
    int level = 0;
    size_t block_size = buddy->min_block_size;
    
    while (level < buddy->num_levels && block_size < size) {
        block_size *= 2;
        level++;
    }

    if (level >= buddy->num_levels) {
        return NULL; // Request too large
    }

    // Try to allocate from the appropriate level
    void* ptr = SlabAllocator_alloc(&buddy->slabs[level]);
    if (ptr) return ptr;

    // If allocation failed, try to split a block from a higher level
    for (int i = level + 1; i < buddy->num_levels; i++) {
        ptr = SlabAllocator_alloc(&buddy->slabs[i]);
        if (!ptr) continue;

        // Split the block and add to appropriate levels
        char* block = (char*)ptr;
        size_t curr_size = buddy->min_block_size * (1 << i);
        
        while (i > level) {
            curr_size /= 2;
            i--;
            
            // Add the buddy block to the free list
            char* buddy_block = block + curr_size;
            SlabAllocator_release(&buddy->slabs[i], buddy_block);
        }

        return block;
    }

    return NULL; // No suitable block found
}

void* BuddyAllocator_free(Allocator* alloc, ...) {
    if (!alloc) return NULL;

    va_list args;
    va_start(args, alloc);
    void* ptr = va_arg(args, void*);
    va_end(args);

    if (!ptr) return NULL;

    BuddyAllocator* buddy = (BuddyAllocator*)alloc;

    // Find which level this block belongs to
    char* block = (char*)ptr;
    size_t offset = block - buddy->managed_memory;
    
    // Find the level based on block alignment
    int level = 0;
    while (level < buddy->num_levels) {
        size_t block_size = buddy->min_block_size * (1 << level);
        if (offset % block_size == 0) break;
        level++;
    }

    if (level >= buddy->num_levels) return NULL; // Invalid pointer

    // Free the block at this level
    SlabAllocator_release(&buddy->slabs[level], ptr);

    // Try to merge with buddy blocks
    while (level < buddy->num_levels - 1) {
        size_t block_size = buddy->min_block_size * (1 << level);
        char* buddy_block;
        
        // Calculate buddy block address
        if (offset % (block_size * 2) == 0) {
            buddy_block = block + block_size;
        } else {
            buddy_block = block - block_size;
        }

        // Check if buddy is free
        void* buddy_ptr = SlabAllocator_alloc(&buddy->slabs[level]);
        if (!buddy_ptr || buddy_ptr != buddy_block) {
            if (buddy_ptr) {
                SlabAllocator_release(&buddy->slabs[level], buddy_ptr);
            }
            break;
        }

        // Merge blocks
        SlabAllocator_release(&buddy->slabs[level], block);
        level++;
        block = (block < buddy_block) ? block : buddy_block;
        offset = block - buddy->managed_memory;
    }

    return (void*)1;
}

BuddyAllocator* BuddyAllocator_create(BuddyAllocator* a, size_t num_levels, int buffer_size) {
    if (!a || buffer_size == 0 || num_levels == 0 || num_levels > MAX_LEVELS) {
        return NULL;
    }

    if (!BuddyAllocator_init((Allocator*)a, num_levels, buffer_size)) {
        return NULL;
    }

    return a;
}
