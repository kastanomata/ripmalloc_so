#include "bitmap_buddy_allocator.h"
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <sys/mman.h>

extern BitmapBuddyAllocator* BitmapBuddyAllocator_create(BitmapBuddyAllocator* alloc, size_t total_size, int num_levels);
extern int BitmapBuddyAllocator_destroy(BitmapBuddyAllocator* alloc);
extern void* BitmapBuddyAllocator_malloc(BitmapBuddyAllocator* alloc, size_t size);
extern int BitmapBuddyAllocator_free(BitmapBuddyAllocator* alloc, void* ptr);

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

static int startIdx(int idx, int num_levels) {
    int level = levelIdx(idx);
    if (level >= num_levels) return -1;  // Invalid level
    return idx - firstIdx(level);
}

static void update_parent(Bitmap* bitmap, int bit) {
    if (bit == 0) return; 
    
    int parent = parentIdx(bit);
    int left = parent * 2 + 1;
    int right = parent * 2 + 2;
    
    // Parent is set only if both children are allocated
    if (bitmap_test(bitmap, left) && bitmap_test(bitmap, right)) {
        bitmap_set(bitmap, parent);
    } else {
        bitmap_clear(bitmap, parent);
    }
    
    // Recursively update ancestors
    update_parent(bitmap, parent);
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

static void merge_block(Bitmap* bitmap, int bit, int num_levels) {
    if (bit == 0) return;
    
    int buddy = buddyIdx(bit);
    if (buddy >= bitmap->num_bits) return;
    
    // Only merge if both are free and at same level
    if (!bitmap_test(bitmap, bit) && !bitmap_test(bitmap, buddy) && 
        levelIdx(bit) == levelIdx(buddy)) {
        int parent = parentIdx(bit);
        bitmap_clear(bitmap, parent);
        merge_block(bitmap, parent, num_levels);
    }
}

static int split_block(BitmapBuddyAllocator* alloc, int level) {
    int original_bit = -1;
    for (int upper = level - 1; upper >= 0; --upper) {
        int first = firstIdx(upper);
        int last = firstIdx(upper + 1);
        for (int i = first; i < last; ++i) {
            if (!bitmap_test(&alloc->bitmap, i)) {
                original_bit = i;
                bitmap_set(&alloc->bitmap, i);
                
                int cur = i;
                while (levelIdx(cur) < level) {
                    int left = cur * 2 + 1;
                    int right = cur * 2 + 2;
                    if (left >= alloc->bitmap.num_bits || right >= alloc->bitmap.num_bits) {
                        // Rollback
                        bitmap_clear(&alloc->bitmap, original_bit);
                        return -1;
                    }
                    bitmap_clear(&alloc->bitmap, left);
                    bitmap_clear(&alloc->bitmap, right);
                    cur = left;
                }
                return cur;
            }
        }
    }
    return -1;
}

static void* get_buddy(BitmapBuddyAllocator* alloc, int level, int size) {
    int bitmap_idx = -1;
    
    for (int i = firstIdx(level); i < firstIdx(level + 1); i++) {
        if (!bitmap_test(&alloc->bitmap, i)) {
            bitmap_idx = i;
            break;
        }
    }
    
    if (bitmap_idx == -1) {
        bitmap_idx = split_block(alloc, level);
        if (bitmap_idx == -1) return NULL;
    }
    
    // Mark the block as allocated
    bitmap_set(&alloc->bitmap, bitmap_idx);
    update_parent(&alloc->bitmap, bitmap_idx); // Ensure parents are updated
    
    int block_size = alloc->min_bucket_size << (alloc->num_levels - level);
    char* ret = alloc->memory_start + (startIdx(bitmap_idx, alloc->num_levels) * block_size);
    
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
    if (num_levels <= 0 || num_levels >= BITMAP_BUDDY_MAX_LEVELS) {
        #ifdef DEBUG
        printf(RED "ERROR: Invalid number of levels (%d)\n" RESET, num_levels);
        #endif
        return NULL;
    }
    
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
    alloc->memory_start = combined_memory;
    alloc->memory_size = total_size;
    void* bitmap_memory = combined_memory + total_size;
    
    // Initialize buddy allocator properties
    alloc->num_levels = num_levels;
    alloc->min_bucket_size = total_size >> num_levels;
    
    // Initialize bitmap
    if (!bitmap_create(&alloc->bitmap, num_bits, bitmap_memory)) {
        munmap(combined_memory, combined_size);
        #ifdef DEBUG
        printf(RED "ERROR: Failed to create bitmap\n" RESET);
        #endif
        return NULL;
    }
    
    // Clear the bitmap
    memset(bitmap_memory, 0, bitmap_size);
    
    // Set up function pointers
    alloc->base.init = BitmapBuddyAllocator_init;
    alloc->base.dest = BitmapBuddyAllocator_cleanup;
    alloc->base.malloc = BitmapBuddyAllocator_reserve;
    alloc->base.free = BitmapBuddyAllocator_release;
    
    return alloc;
}

void* BitmapBuddyAllocator_cleanup(Allocator* base_alloc, ...) {
    BitmapBuddyAllocator* alloc = (BitmapBuddyAllocator*)base_alloc;
    
    if (alloc->memory_start) {
        // Calculate total allocated size (buddy memory + bitmap)
        int num_bits = (1 << (alloc->num_levels + 1)) - 1;
        size_t bitmap_size = ((num_bits + 31) / 32) * sizeof(uint32_t);
        size_t total_size = alloc->memory_size + bitmap_size;
        
        munmap(alloc->memory_start, total_size);
        alloc->memory_start = NULL;
    }
    
    return NULL;
}

void* BitmapBuddyAllocator_reserve(Allocator* base_alloc, ...) {
    va_list args;
    va_start(args, base_alloc);
    
    BitmapBuddyAllocator* alloc = (BitmapBuddyAllocator*)base_alloc;
    size_t size = va_arg(args, size_t);
    va_end(args);
    
    if (size <= 0) {
        #ifdef DEBUG
        printf(RED "ERROR: Invalid size for reservation\n" RESET);
        #endif
        return NULL;
    } 
    
    // Add metadata overhead
    size_t real_size = size + BITMAP_METADATA_SIZE;
    real_size = (real_size + 7) & ~7;  // Align to 8 bytes

    if (real_size > (size_t) alloc->memory_size) {
        #ifdef DEBUG
        printf(RED "ERROR: Requested size too large\n" RESET);
        #endif
        return NULL;
    }
    if (real_size < (size_t)alloc->min_bucket_size) {
        real_size = alloc->min_bucket_size;
    }
    
    // Find appropriate level
    int level = 0;
    size_t block_size = alloc->memory_size;
    if (block_size < (size_t) alloc->min_bucket_size) {
        #ifdef DEBUG
        printf(RED "ERROR: Requested size too small\n" RESET);
        #endif
        return NULL;
    }
    while (block_size / 2 >= real_size && level < alloc->num_levels - 1) {
        block_size /= 2;
        level++;
    }
    
    return get_buddy(alloc, level, size);
}

void* BitmapBuddyAllocator_release(Allocator* base_alloc, ...) {
    va_list args;
    va_start(args, base_alloc);
    
    BitmapBuddyAllocator* alloc = (BitmapBuddyAllocator*)base_alloc;
    void* ptr = va_arg(args, void*);
    va_end(args);

    if (!ptr) return (void*)-1;

    // Convert to char* for pointer arithmetic
    char* char_ptr = (char*)ptr;
    char* memory_start = (char*)alloc->memory_start;
    char* memory_end = memory_start + alloc->memory_size;

    // Validate pointer range
    if (char_ptr < memory_start || char_ptr >= memory_end) {
        #ifdef DEBUG
        printf(RED "ERROR: Pointer %p outside allocator range [%p, %p)\n" RESET, 
               ptr, memory_start, memory_end);
        #endif
        return (void*)-1;
    }

    // Validate metadata location (must be at least BITMAP_METADATA_SIZE before end)
    if ((char_ptr - memory_start) < (long int) BITMAP_METADATA_SIZE) {
        #ifdef DEBUG
        printf(RED "ERROR: Pointer %p too close to start for metadata\n" RESET, ptr);
        #endif
        return (void*)-1;
    }

    // Retrieve metadata
    int* metadata = (int*)(char_ptr - BITMAP_METADATA_SIZE);
    int bit = metadata[0];
    
    // Validate bitmap index
    if (bit < 0 || bit >= alloc->bitmap.num_bits) {
        #ifdef DEBUG
        printf(RED "ERROR: Invalid metadata (bit index %d)\n" RESET, bit);
        #endif
        return (void*)-1;
    }

    // Update bitmap
    update_child(&alloc->bitmap, bit, 0);
    merge_block(&alloc->bitmap, bit, alloc->num_levels);

    return (void*)0;
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
