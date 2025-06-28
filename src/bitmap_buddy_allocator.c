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
    
    // Parent is set if one of the children is allocated
    if (bitmap_test(bitmap, left) || bitmap_test(bitmap, right)) {
        bitmap_set(bitmap, parent);
    } else {
        bitmap_clear(bitmap, parent);
    }
    
    // Recursively update ancestors
    update_parent(bitmap, parent);
}

static bool block_is_free(Bitmap* bm, int idx) {
    if (bitmap_test(bm, idx)) 
        return false;           // this block itself is already marked used
    // walk up the tree: if any ancestor is used, this child is off‑limits
    while (idx > 0) {
        idx = parentIdx(idx);
        if (bitmap_test(bm, idx)) 
            return false;
    }
    return true;
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

static int split_block(BitmapBuddyAllocator* buddy, int level) {
    int original_bit = -1;
    for (int upper = level - 1; upper >= 0; --upper) {
        int first = firstIdx(upper);
        int last = firstIdx(upper + 1);
        for (int i = first; i < last; ++i) {
            if (!bitmap_test(&buddy->bitmap, i)) {
                original_bit = i;
                bitmap_set(&buddy->bitmap, i);
                
                int cur = i;
                while (levelIdx(cur) < level) {
                    int left = cur * 2 + 1;
                    int right = cur * 2 + 2;
                    if (left >= buddy->bitmap.num_bits || right >= buddy->bitmap.num_bits) {
                        // Rollback
                        bitmap_clear(&buddy->bitmap, original_bit);
                        return -1;
                    }
                    bitmap_clear(&buddy->bitmap, left);
                    bitmap_clear(&buddy->bitmap, right);
                    cur = left;
                }
                return cur;
            }
        }
    }
    return -1;
}

static void* get_buddy(BitmapBuddyAllocator* buddy, int level, int size) {
    int bitmap_idx = -1;
    
    for (int i = firstIdx(level); i < firstIdx(level + 1); i++) {
        if (!bitmap_test(&buddy->bitmap, i)) {
            bitmap_idx = i;
            break;
        }
    }
    
    if (bitmap_idx == -1) {
        bitmap_idx = split_block(buddy, level);
        if (bitmap_idx == -1) {
            #ifdef DEBUG
            printf(RED "ERROR: Block of appropriate size not found! %d\n" RESET, level);
            #endif
            if((size_t) size < ((VariableBlockAllocator *) buddy)->sparse_free_memory) {
                printf("\t EXTERNAL FRAGMENTATION: request of %d bytes and %zu bytes of sparse free memory available.\n",
                       size, ((VariableBlockAllocator *) buddy)->sparse_free_memory);
            }
            if((size_t) size < ((VariableBlockAllocator *) buddy)->internal_fragmentation) {
                printf("\t INTERNAL FRAGMENTATION: request of %d bytes and %zu bytes of internal fragmentation.\n",
                       size, ((VariableBlockAllocator *) buddy)->internal_fragmentation);
            }
            return NULL;
        }
    }
    print_user_pointer(bitmap_idx,buddy->num_levels, buddy);
    
    // Mark the block as allocated
    bitmap_set(&buddy->bitmap, bitmap_idx);
    update_parent(&buddy->bitmap, bitmap_idx); // Ensure parents are updated

    level = levelIdx(bitmap_idx);
    int user_idx = bitmap_idx - firstIdx(level);
    size_t usable_block_size = buddy->min_bucket_size << (buddy->num_levels - level);
    size_t full_block_size   = usable_block_size + BITMAP_METADATA_SIZE;
    char*  block_start       = buddy->memory_start
                            + user_idx * full_block_size;
    // write metadata at the very top of each full_block
    BitmapBuddyMetadata* meta = (BitmapBuddyMetadata*)block_start;
    meta->level = level;
    meta->size  = size;
    size_t internal_fragmentation = usable_block_size - size;
    ((VariableBlockAllocator *) buddy)->internal_fragmentation += internal_fragmentation;
    ((VariableBlockAllocator *) buddy)->sparse_free_memory -= usable_block_size;
    // and return the payload area (just past metadata)
    return (void*)(block_start + BITMAP_METADATA_SIZE);
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
    
    BitmapBuddyAllocator* buddy = (BitmapBuddyAllocator*)alloc;
    size_t size = va_arg(args, size_t);
    va_end(args);
    
    if (!buddy || size <= 0) {
        #ifdef DEBUG
        printf(RED "Error: NULL allocator or invalid size\n" RESET);
        #endif
        return NULL;
    }
    
    // Add metadata overhead
    size_t real_size = size + BITMAP_METADATA_SIZE;
    real_size = (real_size + 7) & ~7;  // Align to 8 bytes

    if (real_size > (size_t) buddy->memory_size) {
        #ifdef DEBUG
        printf(RED "ERROR: Requested size too large\n" RESET);
        #endif
        return NULL;
    }
    if (real_size < (size_t)buddy->min_bucket_size) {
        real_size = buddy->min_bucket_size;
    }
    
    // Find appropriate level
    int level = 0;
    size_t block_size = buddy->memory_size;
    if (block_size < (size_t) buddy->min_bucket_size) {
        #ifdef DEBUG
        printf(RED "ERROR: Requested size too small\n" RESET);
        #endif
        return NULL;
    }
    while (block_size / 2 >= real_size && level < buddy->num_levels - 1) {
        block_size /= 2;
        level++;
    }
    
    return get_buddy(buddy, level, size);
}

void* BitmapBuddyAllocator_release(Allocator* alloc, ...) {
    va_list args;
    va_start(args, alloc);
    
    BitmapBuddyAllocator* buddy = (BitmapBuddyAllocator*)alloc;
    void* ptr = va_arg(args, void*);
    va_end(args);

    if (!buddy || !ptr) {
        #ifdef DEBUG
        printf(RED "Error: NULL allocator or pointer in BitmapBuddyAllocator_free\n" RESET);
        #endif
        return (void*)-1;
    }

    // Convert to char* for pointer arithmetic
    char* char_ptr = (char*)ptr;
    char* memory_start = (char*)buddy->memory_start;
    char* memory_end = memory_start + buddy->memory_size;

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

    // Retrieve metadata using BitmapBuddyMetadata struct
    BitmapBuddyMetadata* metadata = (BitmapBuddyMetadata*)(char_ptr - BITMAP_METADATA_SIZE);
    int bit = metadata->level; 
    int size = metadata->size;

    // Calculate bitmap index from level and offset
    int usable_block_size = buddy->min_bucket_size 
                        << (buddy->num_levels - metadata->level);
    int full_block_size   = usable_block_size + BITMAP_METADATA_SIZE;
    int offset = (char_ptr - BITMAP_METADATA_SIZE - memory_start)
                / full_block_size;
    bit = firstIdx(metadata->level) + offset;
    print_user_pointer(bit,buddy->num_levels,buddy);

    // Validate bitmap index
    if (bit < 0 || bit >= buddy->bitmap.num_bits) {
        #ifdef DEBUG
        printf(RED "ERROR: Invalid metadata (calculated bit index %d)\n" RESET, bit);
        #endif
        return (void*)-1;
    }

    // Check if the pointer is already free
    if (bitmap_test(&buddy->bitmap, bit) == 0) {
        #ifdef DEBUG
        printf(RED "Error: Memory already free!\n" RESET);
        #endif
        return (void*)-1;
    }

    size_t internal_fragmentation = usable_block_size - size;
    ((VariableBlockAllocator *) alloc)->internal_fragmentation -= internal_fragmentation;
    ((VariableBlockAllocator *) alloc)->sparse_free_memory += usable_block_size;
    // Update bitmap
    update_child(&buddy->bitmap, bit, 0);
    merge_block(&buddy->bitmap, bit, buddy->num_levels);

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
    
    printf("Bitmap Status:\n");
    for (int i = 0; i < buddy->bitmap.num_bits; i++) {
        printf("%s ", bitmap_test(&buddy->bitmap, i) ? "■" : "□");
        if ((i + 1) && !((i + 2) & (i + 1))) printf("| ");
    }
    printf("\n");
    
    return 0;
}