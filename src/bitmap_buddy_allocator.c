#include "bitmap_buddy_allocator.h"
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <sys/mman.h>

static int levelIdx(size_t idx) {
    return (int)floor(log2(idx+1));
}

static int buddyIdx(int idx) {
    if (idx == 0) return 0;
    return idx % 2 ? idx - 1 : idx + 1;
}

static int parentIdx(int idx) {
    return (idx - 1) / 2;
}

static int firstIdx(int level) {
    return (1 << level) - 1;
}

static int startIdx(int idx) {
    return idx - firstIdx(levelIdx(idx));
}

static void update_parent(Bitmap* bitmap, int bit, int value) {
    if (value) {
        bitmap_set(bitmap, bit);
    } else {
        bitmap_clear(bitmap, bit);
    }
    if (bit > 0) {
        update_parent(bitmap, parentIdx(bit), value);
    }
}

static void update_child(Bitmap* bitmap, int bit, int value) {
    if (bit >= bitmap->num_bits) return;
    
    if (value) {
        bitmap_set(bitmap, bit);
    } else {
        bitmap_clear(bitmap, bit);
    }
    update_child(bitmap, bit * 2 + 1, value);
    update_child(bitmap, bit * 2 + 2, value);
}

static void merge(Bitmap* bitmap, int bit) {
    if (bit == 0) return;
    
    if (bitmap_test(bitmap, bit)) {
        #ifdef DEBUG
        printf(RED "ERROR: Fatal Error in bitmap (merge on bit 1)\n" RESET);
        #endif
        return;
    }
    
    int buddy = buddyIdx(bit);
    if (bitmap_test(bitmap, buddy)) return;
    
    int parent = parentIdx(bit);
    bitmap_clear(bitmap, parent);
    merge(bitmap, parent);
}

static void* get_buddy(BitmapBuddyAllocator* alloc, int level, int size) {
    int bitmap_idx = -1;
    
    if (level == 0) {
        if (!bitmap_test(&alloc->bitmap, firstIdx(level))) {
            bitmap_idx = 0;
        }
    } else {
        for (int i = firstIdx(level); i < firstIdx(level + 1); i++) {
            if (!bitmap_test(&alloc->bitmap, i)) {
                bitmap_idx = i;
                break;
            }
        }
    }
    
    if (bitmap_idx == -1) return NULL;
    
    update_child(&alloc->bitmap, bitmap_idx, 1);
    update_parent(&alloc->bitmap, bitmap_idx, 1);
    
    int block_size = alloc->min_bucket_size << (alloc->num_levels - level);
    char* ret = alloc->memory + ((startIdx(bitmap_idx)) * block_size);
    
    // Store metadata
    ((int*)ret)[0] = bitmap_idx;
    ((int*)ret)[1] = size;
    return (void*)(ret + 2 * sizeof(int));
}

void* BitmapBuddyAllocator_init(Allocator* base_alloc, ...) {
    va_list args;
    va_start(args, base_alloc);
    
    BitmapBuddyAllocator* alloc = (BitmapBuddyAllocator*)base_alloc;
    size_t total_size = va_arg(args, size_t);
    int num_levels = va_arg(args, int);
    va_end(args);
    
    // Validate parameters
    if (num_levels >= BITMAP_BUDDY_MAX_LEVELS) {
        #ifdef DEBUG
        printf(RED "ERROR: Number of levels exceeds maximum (%d)\n" RESET, BITMAP_BUDDY_MAX_LEVELS);
        #endif
        return NULL;
    }
    
    // Allocate memory
    alloc->memory = mmap(NULL, total_size, PROT_READ | PROT_WRITE, 
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (alloc->memory == MAP_FAILED) {
        #ifdef DEBUG
        printf(RED "ERROR: Failed to allocate memory\n" RESET);
        #endif
        return NULL;
    }
    
    alloc->memory_size = total_size;
    alloc->num_levels = num_levels;
    alloc->min_bucket_size = total_size >> num_levels;
    
    // Initialize bitmap
    int num_bits = (1 << (num_levels + 1)) - 1;
    if (!bitmap_create(&alloc->bitmap, num_bits)) {
        munmap(alloc->memory, total_size);
        #ifdef DEBUG
        printf(RED "ERROR: Failed to create bitmap\n" RESET);
        #endif
        return NULL;
    }
    
    // Set up function pointers
    alloc->base.init = BitmapBuddyAllocator_init;
    alloc->base.dest = BitmapBuddyAllocator_cleanup;
    alloc->base.malloc = BitmapBuddyAllocator_reserve;
    alloc->base.free = BitmapBuddyAllocator_release;
    
    return alloc;
}

BitmapBuddyAllocator* BitmapBuddyAllocator_create(BitmapBuddyAllocator* alloc, 
                                                 size_t total_size, 
                                                 int num_levels) {
    if(!alloc || total_size == 0 || num_levels <= 0) {
        #ifdef DEBUG
        printf(RED "Error: Invalid parameters for BitmapBuddyAllocator_create\n" RESET);
        #endif
        return NULL;
    }
    if (num_levels >= BITMAP_BUDDY_MAX_LEVELS) {
        #ifdef DEBUG
        printf(RED "Error: Number of levels exceeds maximum (%d)\n" RESET, BITMAP_BUDDY_MAX_LEVELS);
        #endif
        return NULL;
    }
    if (!BitmapBuddyAllocator_init((Allocator*)alloc, total_size, num_levels)) {
        #ifdef DEBUG
        printf(RED "Error: Failed to initialize BitmapBuddyAllocator\n" RESET);
        #endif
        return NULL;
    }
    return alloc;
}

void* BitmapBuddyAllocator_cleanup(Allocator* base_alloc, ...) {
    BitmapBuddyAllocator* alloc = (BitmapBuddyAllocator*)base_alloc;
    
    if (alloc->memory) {
        munmap(alloc->memory, alloc->memory_size);
    }
    
    if (alloc->bitmap.bits) {
        bitmap_destroy(&alloc->bitmap);
    }
    
    return NULL;
}

int BitmapBuddyAllocator_destroy(BitmapBuddyAllocator* alloc) {
    if (!alloc) {
        #ifdef DEBUG
        printf(RED "Error: NULL allocator in BitmapBuddyAllocator_destroy\n" RESET);
        #endif
        return -1;
    }
    
    if (alloc->base.dest((Allocator*)alloc) != NULL) {
        #ifdef DEBUG
        printf(RED "Error: Failed to destroy BitmapBuddyAllocator\n" RESET);
        #endif
        return -1;
    }
    return 0;
}

void* BitmapBuddyAllocator_reserve(Allocator* base_alloc, ...) {
    va_list args;
    va_start(args, base_alloc);
    
    BitmapBuddyAllocator* alloc = (BitmapBuddyAllocator*)base_alloc;
    size_t size = va_arg(args, size_t);
    va_end(args);
    
    if (size <= 0) return NULL;
    
    // Add metadata overhead
    size_t real_size = size + 2 * sizeof(int);
    
    // Find appropriate level
    int level = 0;
    size_t block_size = alloc->memory_size;
    while (block_size / 2 >= real_size && level < alloc->num_levels) {
        block_size /= 2;
        level++;
    }
    
    return get_buddy(alloc, level, size);
}

void* BitmapBuddyAllocator_malloc(BitmapBuddyAllocator* alloc, size_t size) {
    if (!alloc || size <= 0) {
        #ifdef DEBUG
        printf(RED "Error: NULL allocator or invalid size\n" RESET);
        #endif
        return NULL;
    }

    void* memory = alloc->base.malloc((Allocator*)alloc, size);
    if( memory == NULL ) {
        #ifdef DEBUG
        printf(RED "Error: Failed to allocate memory\n" RESET);
        #endif
        return NULL;
    }
    return memory;
}

void* BitmapBuddyAllocator_release(Allocator* base_alloc, ...) {
    va_list args;
    va_start(args, base_alloc);
    
    BitmapBuddyAllocator* alloc = (BitmapBuddyAllocator*)base_alloc;
    void* ptr = va_arg(args, void*);
    va_end(args);

    if (!ptr) return (void*) -1;

    // Retrieve metadata
    int* metadata = (int*)((char*)ptr - 2 * sizeof(int));
    int bit = metadata[0];
    
    // Update bitmap
    update_child(&alloc->bitmap, bit, 0);
    merge(&alloc->bitmap, bit);

    return (void*)0;
}

int BitmapBuddyAllocator_free(BitmapBuddyAllocator* alloc, void* ptr) {
    if (!alloc || !ptr) {
        #ifdef DEBUG
        printf(RED "Error: NULL allocator or pointer in BitmapBuddyAllocator_free\n" RESET);
        #endif
        return -1;
    }
    // Retrieve metadata
    int* metadata = (int*)((char*)ptr - 2 * sizeof(int));
    int bit = metadata[0];
    // Check if the pointer is already free
    if (bitmap_test(&alloc->bitmap, bit) == 0) {
        #ifdef DEBUG
        printf(RED "Error: Memory already free!\n" RESET);
        #endif
        return -1;
    }
    if (alloc->base.free((Allocator*)alloc, ptr) == (void*) -1) {
        #ifdef DEBUG
        printf(RED "Error: Failed to free memory in BitmapBuddyAllocator_free\n" RESET);
        #endif
        return -1;
    }
    return 0;
}

int BitmapBuddyAllocator_print_state(BitmapBuddyAllocator* alloc) {
    if (!alloc) {
        #ifdef DEBUG
        printf(RED "Error: NULL allocator in BitmapBuddyAllocator_print_state\n" RESET);
        #endif
        return -1;
    }
    
    printf("Bitmap Buddy Allocator State:\n");
    printf("  Memory Size: %d bytes\n", alloc->memory_size);
    printf("  Number of Levels: %d\n", alloc->num_levels);
    printf("  Minimum Bucket Size: %d bytes\n", alloc->min_bucket_size);
    
    printf("Bitmap Status:\n");
    for (int i = 0; i < alloc->bitmap.num_bits; i++) {
        printf("%s ", bitmap_test(&alloc->bitmap, i) ? "■" : "□");
        if ((i + 1) && !((i + 2) & (i + 1))) printf("| ");
    }
    printf("\n");
    
    return 0;
}
