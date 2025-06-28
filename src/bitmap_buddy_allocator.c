#include "bitmap_buddy_allocator.h"
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <sys/mman.h>

extern BitmapBuddyAllocator* BitmapBuddyAllocator_create(BitmapBuddyAllocator* buddy, size_t total_size, int num_levels);
extern int BitmapBuddyAllocator_destroy(BitmapBuddyAllocator* buddy);
extern void* BitmapBuddyAllocator_malloc(BitmapBuddyAllocator* buddy, size_t size);
extern int BitmapBuddyAllocator_free(BitmapBuddyAllocator* buddy, void* ptr);

void print_user_pointer(int bitmap_idx, int num_levels, BitmapBuddyAllocator* buddy) {
    int level = (int)floor(log2(bitmap_idx + 1));
    int user_idx = bitmap_idx - ((1 << level) - 1);
    size_t usable_block_size = buddy->min_bucket_size << (buddy->num_levels - level);
    size_t full_block_size   = usable_block_size + BITMAP_METADATA_SIZE;
    char*  user_ptr = buddy->memory_start
                    + user_idx * full_block_size
                    + BITMAP_METADATA_SIZE;
    printf("memory_start %p + user_idx %d*full_block_size %d + METADATA_SIZE %ld (%d) = %p\n",
           (void*)buddy->memory_start, user_idx, (int)full_block_size, BITMAP_METADATA_SIZE, 
           (int)(user_idx * full_block_size + BITMAP_METADATA_SIZE), 
           (void*)(buddy->memory_start + user_idx * full_block_size + BITMAP_METADATA_SIZE));
    printf("User pointer for bitmap index %d (level %d of %d): %p\n", bitmap_idx, level, num_levels, (void*)user_ptr);
}

static int levelIdx(size_t idx) {
    return (int)floor(log2(idx+1));
}

static int buddyIdx(int idx) {
    if (idx == 0) return -1;
    // formula: even goes to idx - 1, odd goes to idx + 1
    if (idx % 2 == 0) {
        return idx - 1; // even index, buddy is left
    } else {
        return idx + 1; // odd index, buddy is right
    }
}

static int parentIdx(int idx) {
    if(idx == 0 ) return -1;
    return (idx - 1) / 2;
}

static int firstIdx(int level) {
    return (1 << level) - 1; // 2^(level-1)
}


static void update_parents(Bitmap* bitmap, int bit, int value) {
    
    if (value) {
        bitmap_set(bitmap, bit);
    } else {
        bitmap_clear(bitmap, bit);
    }
    int parent = parentIdx(bit);
    if(parent == -1) return;
    
    // Recursively update ancestors
    update_parents(bitmap, parent,value);
}

static void update_children(Bitmap* bitmap, int bit, int value) {
    if (bit >= bitmap->num_bits) return;

    if (value) {
        bitmap_set(bitmap, bit);
    } else {
        bitmap_clear(bitmap, bit);
    }
    update_children(bitmap, bit * 2 + 1, value);
    update_children(bitmap, bit * 2 + 2, value);
}

void* BitmapBuddyAllocator_init(Allocator* alloc, ...) {
    va_list args;
    va_start(args, alloc);
    
    BitmapBuddyAllocator* buddy = (BitmapBuddyAllocator*)alloc;
    size_t total_size = va_arg(args, size_t);
    int num_levels = va_arg(args, int);
    va_end(args);
    
    // Validate parameters
    if (!buddy || total_size <= 0 || num_levels <= 0 || num_levels >= BITMAP_BUDDY_MAX_LEVELS) {
        #ifdef DEBUG
        printf(RED "ERROR: Invalid allocator or invalid number of levels (%d)\n" RESET, num_levels);
        #endif
        return NULL;
    }

    // Round down total_size to the largest power of two
    size_t pow2 = 1UL << ((size_t)floor(log2((double)total_size)));
    total_size = pow2;
    
    // Calculate bitmap memory requirements
    int num_bits = (1 << (num_levels + 1)) - 1;
    size_t bitmap_size = ((num_bits + 31) / 32) * sizeof(uint32_t);
    
    // Allocate memory for both buddy system and bitmap
    size_t combined_size = total_size + bitmap_size;
    char* combined_memory = mmap(NULL, combined_size, PROT_READ | PROT_WRITE, 
                               MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (combined_memory == MAP_FAILED) {
        #ifdef DEBUG
        printf(RED "ERROR: Failed to allocate memory\n" RESET);
        #endif
        return NULL;
    }
    
    // Split the allocated memory into buddy memory and bitmap memory
    buddy->memory_start = combined_memory;
    buddy->memory_size = total_size;
    void* bitmap_memory = combined_memory + total_size;

    // Initialize fields of VariableBlockAllocator
    ((VariableBlockAllocator *) alloc)->internal_fragmentation = 0;
    ((VariableBlockAllocator *) alloc)->sparse_free_memory = total_size;
    
    // Initialize buddy allocator properties
    size_t min_bucket_size = total_size >> num_levels;
    while (min_bucket_size < (BITMAP_METADATA_SIZE + 1) && num_levels > 0) {
        num_levels--;
        min_bucket_size = total_size >> num_levels;
    }
    buddy->num_levels = num_levels;
    buddy->min_bucket_size = min_bucket_size;
    #ifdef DEBUG
    printf("num_levels: %d\n", num_levels);
    printf("min_bucket_size: %zu\n", min_bucket_size);
    #endif
    
    // Initialize bitmap
    if (!bitmap_create(&buddy->bitmap, num_bits, bitmap_memory)) {
        munmap(combined_memory, combined_size);
        #ifdef DEBUG
        printf(RED "ERROR: Failed to create bitmap\n" RESET);
        #endif
        return NULL;
    }
    
    // Clear the bitmap
    memset(bitmap_memory, 0, bitmap_size);

    /* Set the metadata for the biggest block to -1 (mark as free) */
    BitmapBuddyMetadata* meta = (BitmapBuddyMetadata*)buddy->memory_start;
    meta->bitmap_idx = -1;
    meta->size = -1;
    
    // Set up function pointers
    alloc->init = BitmapBuddyAllocator_init;
    alloc->dest = BitmapBuddyAllocator_cleanup;
    alloc->malloc = BitmapBuddyAllocator_reserve;
    alloc->free = BitmapBuddyAllocator_release;
    
    return buddy;
}

void* BitmapBuddyAllocator_cleanup(Allocator* alloc, ...) {
    BitmapBuddyAllocator* buddy = (BitmapBuddyAllocator*)alloc;
    if (!buddy) {
        #ifdef DEBUG
        printf(RED "Error: NULL allocator in BitmapBuddyAllocator_destroy\n" RESET);
        #endif
        return (void*)-1;
    }
    
    if (buddy->memory_start) {
        // Calculate total allocated size (buddy memory + bitmap)
        int num_bits = (1 << (buddy->num_levels + 1)) - 1;
        size_t bitmap_size = ((num_bits + 31) / 32) * sizeof(uint32_t);
        size_t total_size = buddy->memory_size + bitmap_size;
        
        munmap(buddy->memory_start, total_size);
        buddy->memory_start = NULL;
    }
    
    return NULL;
}

void* BitmapBuddyAllocator_reserve(Allocator* alloc, ...) {
    va_list args;
    va_start(args, alloc);
    size_t size = va_arg(args, size_t);
    va_end(args);

    BitmapBuddyAllocator* buddy = (BitmapBuddyAllocator*)alloc;
    size_t total_size = size + BITMAP_METADATA_SIZE;
    if (!buddy || size == 0 || total_size > (size_t)buddy->memory_size) {
        #ifdef DEBUG
        printf(RED "ERROR: NULL allocator or invalid allocation size !\n");
        #endif
        return NULL;
    }

    // Find the smallest block whose usable size fits the request
    int level_new_block = buddy->num_levels;
    size_t block_size = buddy->min_bucket_size;
    for (int i = 0; i <= buddy->num_levels; i++) {
        if (block_size >= size + BITMAP_METADATA_SIZE) {
            break;
        }
        if (level_new_block > 0) {
            block_size *= 2;
            level_new_block--;
        }
    }

    // Cerca un blocco libero al livello scelto
    int freeidx = -1;
    for (int j = firstIdx(level_new_block); j < firstIdx(level_new_block + 1); j++) {
        if (!bitmap_test(&buddy->bitmap, j)) {
            freeidx = j;
            break;
        }
    }

    if (freeidx == -1) {
        #ifdef DEBUG
        printf(RED "ERROR: No free blocks at any level\n");
        #endif
        return NULL;
    }
    // When printing/debugging, show both block and usable size
    size_t usable_block_size = block_size - BITMAP_METADATA_SIZE;
    #ifdef DEBUG
    printf("Found free block at bitmap index %d (level %d) of size %zu (usable %zu)\n",
           freeidx, level_new_block, block_size, usable_block_size);
    #endif

    // Setta il blocco e i suoi antenati/discendenti come allocati
    update_parents(&buddy->bitmap, freeidx, RESERVED);
    update_children(&buddy->bitmap, freeidx, RESERVED);

    // Calcola l'indirizzo e salva i metadati
    int user_idx = freeidx - firstIdx(level_new_block);
    size_t full_block_size = usable_block_size + BITMAP_METADATA_SIZE;
    char* block_start = buddy->memory_start + user_idx * full_block_size;
    BitmapBuddyMetadata* meta = (BitmapBuddyMetadata*)block_start;
    meta->bitmap_idx = freeidx;
    meta->size = size;

    size_t internal_fragmentation = full_block_size - size;
    ((VariableBlockAllocator *) buddy)->internal_fragmentation += internal_fragmentation;
    ((VariableBlockAllocator *) buddy)->sparse_free_memory -= full_block_size;

    // Restituisce il puntatore all'area payload (dopo i metadati)
    #ifdef DEBUG
    printf("After alloc:\n");
    // print_bitmap_status(buddy);
    printf("Allocating block at bitmap index %d (level %d) with size %zu: %p\n", meta->bitmap_idx, level_new_block, size, (void*)(block_start + BITMAP_METADATA_SIZE));
    #endif
    return (void*)(block_start + BITMAP_METADATA_SIZE);
}

static void merge(Bitmap* bitmap, int idx) {
    if (idx == 0) return; // root, nothing to merge up
    int buddy_idx = buddyIdx(idx);
    int parent = parentIdx(idx);
    #ifdef DEBUG
    printf("Merging block at bitmap index %d\n", idx);
    printf("Buddy  at bitmap index %d\n", buddy_idx);
    printf("Parent at bitmap index %d\n", parent);
    printf("Status: idx is %s, buddy is %s, parent is %s\n",
           bitmap_test(bitmap, idx) ? "allocated" : "free",
           bitmap_test(bitmap, buddy_idx) ? "allocated" : "free",
           bitmap_test(bitmap, parent) ? "allocated" : "free");
    #endif
    // Only merge if both buddies are free
    if (!bitmap_test(bitmap, idx) && !bitmap_test(bitmap, buddy_idx)) {
        #ifdef DEBUG
        printf("Both buddies are free, merging...\n");
        #endif
        bitmap_clear(bitmap, parent);
        merge(bitmap, parent);
    } else {
        #ifdef DEBUG
        printf("Cannot merge: at least one buddy is allocated.\n");
        #endif
    }
}

void* BitmapBuddyAllocator_release(Allocator* alloc, ...) {
    va_list args;
    va_start(args, alloc);
    void* ptr = va_arg(args, void*);
    va_end(args);

    BitmapBuddyAllocator* buddy = (BitmapBuddyAllocator*)alloc;
    if (!buddy || !ptr) {
        #ifdef DEBUG
        printf(RED "ERROR: NULL allocator or pointer to free!\n" RESET);
        #endif
        return (void*)-1;
    }

    char* char_ptr = (char*)ptr;
    BitmapBuddyMetadata* meta = (BitmapBuddyMetadata*)(char_ptr - BITMAP_METADATA_SIZE);
    int idx_to_free = meta->bitmap_idx;
    #ifdef DEBUG
    printf("Freeing block at metadata bitmap index %d\n", idx_to_free);
    #endif
    int size = meta->size;
    int level = levelIdx(idx_to_free);
    int full_block_size = buddy->min_bucket_size << (buddy->num_levels - level);

    // Controlla se già libero (double free)
    if (idx_to_free == -1) {
        #ifdef DEBUG
        printf(RED "ERROR: Double free!\n" RESET);
        printf("\t tried to free block number %d at level %d with size %d\n", idx_to_free, level, size);
        #endif
        return (void*)-1;
    }
    
    ((VariableBlockAllocator *) buddy)->internal_fragmentation -= (full_block_size - size);
    ((VariableBlockAllocator *) buddy)->sparse_free_memory += full_block_size;
    // Libera i discendenti e tenta il merge
    update_children(&buddy->bitmap, idx_to_free, RELEASED);
    merge(&buddy->bitmap, idx_to_free);
    
    // put metadata to 0
    meta->bitmap_idx = -1; // Clear metadata
    meta->size = -1;       // Clear size
    #ifdef DEBUG
    printf("After free:\n");
    // print_bitmap_status(buddy);
    printf("Freed block at bitmap index %d (level %d) with size %d\n", idx_to_free, level, size);
    #endif

    return (void*)0;
}

int BitmapBuddyAllocator_print_state(BitmapBuddyAllocator* buddy) {
    if (!buddy) {
        #ifdef DEBUG
        printf(RED "Error: NULL allocator in BitmapBuddyAllocator_print_state\n" RESET);
        #endif
        return -1;
    }
    
    printf("Bitmap Buddy Allocator State:\n");
    printf("  Memory Size: %d bytes\n", buddy->memory_size);
    printf("  Number of Levels: %d\n", buddy->num_levels);
    printf("  Minimum Bucket Size: %d bytes\n", buddy->min_bucket_size);
    
    print_bitmap_status(buddy);
    
    return 0;
}

