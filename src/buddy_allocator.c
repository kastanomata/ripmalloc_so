#include <buddy_allocator.h>

extern inline BuddyAllocator* BuddyAllocator_create(BuddyAllocator* alloc, size_t total_size, int num_levels);
extern inline int BuddyAllocator_destroy(BuddyAllocator* alloc);
extern inline void* BuddyAllocator_malloc(BuddyAllocator* alloc, size_t size);
extern inline int BuddyAllocator_free(BuddyAllocator* alloc, void* ptr);

static struct Buddies BuddyAllocator_divide_block(BuddyAllocator* a, BuddyNode* parent) {
    struct Buddies buddies = {NULL, NULL};
    if (!a || !parent) {
        #ifdef DEBUG
        printf(RED "ERROR: Invalid parameters in divide_block!\n" RESET);
        #endif
        return buddies;
    }

    buddies.left_buddy = SlabAllocator_malloc(&(a->node_allocator));
    buddies.right_buddy = SlabAllocator_malloc(&(a->node_allocator));
    
    // Clean up if one allocation fails
    if (!buddies.left_buddy || !buddies.right_buddy) {
        if (buddies.left_buddy) SlabAllocator_free(&(a->node_allocator), buddies.left_buddy);
        if (buddies.right_buddy) SlabAllocator_free(&(a->node_allocator), buddies.right_buddy);
        buddies.left_buddy = NULL;
        buddies.right_buddy = NULL;
        return buddies;
    }

    parent->is_free = false;

    size_t child_size = parent->size / 2;
    buddies.left_buddy->size = child_size;
    buddies.left_buddy->data = parent->data;
    buddies.left_buddy->is_free = false;
    buddies.left_buddy->parent = parent;
    buddies.left_buddy->buddy = buddies.right_buddy;

    buddies.right_buddy->size = child_size;
    buddies.right_buddy->data = parent->data + child_size;
    buddies.right_buddy->is_free = true;
    buddies.right_buddy->parent = parent;
    buddies.right_buddy->buddy = buddies.left_buddy;

    int child_level = parent->level + 1;
    buddies.left_buddy->level = child_level;
    buddies.right_buddy->level = child_level;
    
    assert(buddies.left_buddy->level == buddies.right_buddy->level);

    return buddies;
}


static BuddyNode* BuddyAllocator_merge_blocks(BuddyAllocator* a, BuddyNode* left, BuddyNode* right) {
    if (!a || !left || !right) {
        printf(RED "ERROR: Null parameters to merge_blocks\n" RESET);
        return NULL;
    }

    // Verify blocks are at same level
    if (left->level != right->level) {
        printf(RED "ERROR: Different levels in merge (%d vs %d)\n" RESET,
               left->level, right->level);
        return NULL;
    }

    BuddyNode* parent = left->parent;
    if (!parent || parent != right->parent) {
        printf(RED "ERROR: Blocks don't share a parent\n" RESET);
        return NULL;
    }
    
    // Verify parent level makes sense
    int expected_parent_level = left->level - 1;
    if (parent->level != expected_parent_level) {
        printf(RED "ERROR: Parent level mismatch! Is %d, should be %d\n" RESET,
               parent->level, expected_parent_level);
        return NULL;
    }

    parent->is_free = true;
    
    // Remove children from free lists
    list_detach(a->free_lists[left->level], (Node*)&left->node);
    list_detach(a->free_lists[right->level], (Node*)&right->node);

    // Free child nodes
    SlabAllocator_free(&(a->node_allocator), left);
    SlabAllocator_free(&(a->node_allocator), right);

    return parent;
}

void* BuddyAllocator_init(Allocator* alloc, ...) {
    va_list args;
    va_start(args, alloc);
    BuddyAllocator* buddy = (BuddyAllocator*)alloc;
    size_t total_size = va_arg(args, size_t);
    int num_levels = va_arg(args, int) + 1; // +1 for the root level
    va_end(args);
    if (!alloc || total_size <= 0 || num_levels <= 1 || num_levels >= BUDDY_MAX_LEVELS) {
        #ifdef DEBUG
        printf(RED "ERROR: Invalid parameters in create!\n" RESET);
        #endif
        return NULL;
    } 
    // Calculate max number of nodes
    size_t max_nodes = 0;
    size_t level_nodes = 1;
    for (int i = 0; i < num_levels; i++) {
        max_nodes += level_nodes;
        level_nodes *= 2;
    }

    ((VariableBlockAllocator *) alloc)->internal_fragmentation = 0;
    ((VariableBlockAllocator *) alloc)->sparse_free_memory = total_size;

    // Initialize memory
    buddy->memory_start = mmap(NULL, total_size, PROT_READ | PROT_WRITE, 
                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (buddy->memory_start == MAP_FAILED) {
        #ifdef DEBUG
        printf(RED "ERROR: Failed to allocate memory in init!\n" RESET);
        perror("mmap failed");
        #endif
        return NULL;
    }

    buddy->total_size = total_size;
    // Initialize buddy allocator properties
    // Calculate minimum block size (min_bucket_size) and adjust num_levels if needed
    size_t min_block_size = total_size >> (num_levels - 1);
    while (min_block_size < (BUDDY_METADATA_SIZE + 1) && num_levels > 1) {
        num_levels--;
        min_block_size = total_size >> (num_levels - 1);
    }
    buddy->num_levels = num_levels;
    buddy->min_block_size = min_block_size;
    #ifdef DEBUG
    printf("num_levels: %d\n", num_levels);
    printf("min_block_size: %zu\n", min_block_size);
    #endif

    // Initialize list allocator
    SlabAllocator* list_allocator = SlabAllocator_create(&(buddy->list_allocator), sizeof(DoubleLinkedList), num_levels);
    if (!list_allocator) {
        #ifdef DEBUG
        printf(RED "ERROR: Failed to create list allocator!\n" RESET);
        #endif
        return NULL;
    }
    buddy->list_allocator = *list_allocator;
    
    // Initialize free lists
    for (int i = 0; i < num_levels; i++) {
        buddy->free_lists[i] = (DoubleLinkedList*)SlabAllocator_malloc(&buddy->list_allocator);
        if (!buddy->free_lists[i]) {
            #ifdef DEBUG
            printf(RED "ERROR: Failed to allocate free list!\n" RESET);
            #endif
            return NULL;
        }
        list_create(buddy->free_lists[i]);
    }

    // Initialize node allocator
    SlabAllocator* slab = SlabAllocator_create(&(buddy->node_allocator), sizeof(BuddyNode), max_nodes);
    if (!slab) {
        #ifdef DEBUG
        printf(RED "ERROR: Failed to create slab allocator!\n" RESET);
        #endif
        return NULL;
    }
    buddy->node_allocator = *slab;

    // Create first node
    BuddyNode* first_node = (BuddyNode*)SlabAllocator_malloc(&buddy->node_allocator);
    if (!first_node) {
        #ifdef DEBUG
        printf(RED "ERROR: Failed to create first node!\n" RESET);
        #endif
        return NULL;
    }

    // Initialize first node
    first_node->size = total_size;
    first_node->data = buddy->memory_start;
    first_node->level = 0;
    first_node->is_free = 1;
    first_node->buddy = NULL;
    first_node->parent = NULL;
    
    // Add to free list
    list_push_front(buddy->free_lists[0], (Node*)&first_node->node);

    // Initialize function pointers
    alloc->init = BuddyAllocator_init;
    alloc->dest = BuddyAllocator_cleanup;
    alloc->malloc = BuddyAllocator_reserve;
    alloc->free = BuddyAllocator_release;
    return buddy;
}

void* BuddyAllocator_cleanup(Allocator* alloc, ...) {
    if (!alloc) {
        #ifdef DEBUG
        printf(RED "ERROR: NULL allocator passed to destructor\n" RESET);
        #endif
        return (void*)-1;
    }

    BuddyAllocator* buddy = (BuddyAllocator*)alloc;
    if (!buddy->memory_start) {
        #ifdef DEBUG
        printf(RED "ERROR: NULL memory_start in destructor\n" RESET);
        #endif
        return (void*)-1;
    }

    if (munmap(buddy->memory_start, buddy->total_size) != 0) {
        #ifdef DEBUG
        printf(RED "ERROR: Failed to unmap memory in destructor\n" RESET);
        #endif
        return (void*)-1;
    }

    if (SlabAllocator_destroy(&buddy->list_allocator) != 0) {
        #ifdef DEBUG
        printf(RED "ERROR: Failed to destroy list allocator\n" RESET);
        #endif
        return (void*)-1;
    }

    if (SlabAllocator_destroy(&buddy->node_allocator) != 0) {
        #ifdef DEBUG
        printf(RED "ERROR: Failed to destroy node allocator\n" RESET); 
        #endif
        return (void*)-1;
    }
    
    return (void*)0;
}

void* BuddyAllocator_reserve(Allocator* alloc, ...) {
    va_list args;
    va_start(args, alloc);
    BuddyAllocator* buddy = (BuddyAllocator*)alloc;
    size_t size = va_arg(args, size_t);
    va_end(args);
    if (!alloc || size <= 0) {
        #ifdef DEBUG
        printf(RED "ERROR: NULL allocator or invalid size in alloc!\n" RESET);
        #endif
        return NULL;
    }

    size_t adjusted_size = size + BUDDY_METADATA_SIZE;
    // Align to 8 bytes
    adjusted_size = (adjusted_size + 7) & ~7;

    
    if (!buddy || adjusted_size == 0) {
        #ifdef DEBUG
        printf(RED "ERROR: NULL allocator or invalid adjusted_size in reserve!\n" RESET);
        #endif
        return NULL;
    }
    
    // Calculate the appropriate level for the requested adjusted_size
    size_t block_size = buddy->total_size;
    int level = 0;
    while (level < buddy->num_levels - 1 && block_size / 2 >= adjusted_size) {
        block_size /= 2;
        level++;
    }
    
    if (block_size < adjusted_size) {
        #ifdef DEBUG
        printf(RED "ERROR: Requested adjusted_size too large (req: %zu, max: %zu)\n" RESET, 
               adjusted_size, buddy->total_size);
        #endif
        return NULL;
    }

    
    if (level < 0 || level >= buddy->num_levels) {
        #ifdef DEBUG
        printf(RED "ERROR: Invalid level %d (max %d)\n" RESET, level, buddy->num_levels-1);
        #endif
        return NULL;
    }
    
    if (!buddy->free_lists[level]) {
        #ifdef DEBUG
        printf(RED "ERROR: No free list at level %d\n" RESET, level);
        #endif
        return NULL;
    }

    BuddyNode* free_block = (BuddyNode*) list_pop_front(buddy->free_lists[level]);    
    int current_level = level;
    
    // Search larger blocks if needed
    while (!free_block && current_level > 0) {
        current_level--;
        free_block = (BuddyNode*) list_pop_front(buddy->free_lists[current_level]);
        if (free_block) {            
            // Split block to desired level
            while (current_level < level) {
                struct Buddies buddies = BuddyAllocator_divide_block(buddy, free_block);
                if (buddies.left_buddy == NULL || buddies.right_buddy == NULL) {
                    #ifdef DEBUG
                    printf(RED "ERROR: Failed to split block!\n" RESET);
                    #endif
                    if (free_block) list_push_front(buddy->free_lists[current_level], (Node*)free_block);
                    return NULL;
                }
                
                list_push_back(buddy->free_lists[current_level + 1], (Node *) buddies.right_buddy);
                free_block = buddies.left_buddy;
                current_level++;
            }
            break;
        }
    }
    
    if (!free_block) {
        #ifdef DEBUG
        printf(RED "ERROR: No free blocks available at any level\n" RESET);
        #endif
        if(adjusted_size < ((VariableBlockAllocator *) alloc)->sparse_free_memory) {
            printf("\t EXTERNAL FRAGMENTATION: request of %zu bytes and %zu bytes of sparse free memory available.\n",
                   adjusted_size, ((VariableBlockAllocator *) alloc)->sparse_free_memory);
        }
        if(adjusted_size < (((VariableBlockAllocator *) alloc)->internal_fragmentation)) {
            printf("\t INTERNAL FRAGMENTATION: request of %zu bytes and %zu bytes of internal fragmentation.\n",
                   adjusted_size, ((VariableBlockAllocator *) alloc)->internal_fragmentation);
        }
        return NULL;
    }

    free_block->is_free = false;
    free_block->requested_size = adjusted_size;
    size_t internal_fragmentation = free_block->size - free_block->requested_size;
    ((VariableBlockAllocator *) alloc)->internal_fragmentation += internal_fragmentation;
    ((VariableBlockAllocator *) alloc)->sparse_free_memory -= free_block->size;

    // printf("ALLOCATION: Requested size: %zu bytes\n", adjusted_size);
    // printf("Level requested at: %d, size of blocks at that level: %zu\n", level, block_size);
    // printf("For this block, internal frag is %zu bytes\n", internal_fragmentation);
    // printf("Total internal frag is %zu bytes\n", 
    //        ((VariableBlockAllocator *) alloc)->internal_fragmentation);

    // Store metadata at start of block
    void* raw_block = free_block->data;
    *((BuddyNode**)raw_block) = free_block;
    void* user_ptr = (char*)raw_block + BUDDY_METADATA_SIZE;
    return user_ptr;
}

void *BuddyAllocator_release(Allocator* alloc, ...) {
    va_list args;
    va_start(args, alloc);
    BuddyAllocator* a = (BuddyAllocator*)alloc;
    void* ptr = va_arg(args, void*);
    va_end(args);
    if(!a || !ptr) {
        #ifdef DEBUG
        printf(RED "ERROR: NULL allocator or pointer in release\n" RESET);
        #endif
        return (void*)-1;
    }
    // Verify pointer is within allocator's memory range
    if ((char*)ptr < (char*)a->memory_start || 
        (char*)ptr >= (char*)a->memory_start + a->total_size) {
        #ifdef DEBUG
        printf(RED "ERROR: Pointer outside allocator memory range!\n" RESET);
        #endif
        return (void*)-1;
    }

    BuddyNode* node = *((BuddyNode**)((char*)ptr - BUDDY_METADATA_SIZE));
    if(node->is_free) {
        #ifdef DEBUG
        printf(RED "ERROR: Attempting to release an already free block\n" RESET);
        #endif
        return (void*)-1;
    }
    if (!node || !node->data) {
        if (node) printf("Node data: %p\n", (void*)node->data);
        return (void*)-1;
    }

    if ((char*)node->data < (char*)a->memory_start || 
        (char*)node->data >= (char*)a->memory_start + a->total_size) {
        printf(RED "ERROR: Node outside allocator memory range!\n" RESET);
        return (void*)-1;
    }
    
    if (node->level < 0 || node->level >= a->num_levels) {
        printf(RED "ERROR: Invalid level %d (max %d)\n" RESET, 
               node->level, a->num_levels-1);
        return (void*)-1;
    }
    if (!a->free_lists[node->level]) {
        printf(RED "ERROR: Free list at level %d is NULL!\n" RESET, node->level);
        return (void*)-1;
    }    
    node->is_free = 1;
    
    size_t internal_fragmentation = node->size - node->requested_size;
    ((VariableBlockAllocator *) alloc)->internal_fragmentation -= internal_fragmentation;
    ((VariableBlockAllocator *) alloc)->sparse_free_memory += node->size;

    // printf("FREE: freeing block of size %zu bytes, was a request for %zu\n", node->size, node->requested_size);
    // printf("Level freed at: %d, size of blocks at that level: %zu\n", 
    //        node->level, a->total_size / (1 << node->level));
    // printf("Total internal frag is %zu bytes\n", 
    //        ((VariableBlockAllocator *) alloc)->internal_fragmentation);

    list_push_front(a->free_lists[node->level], (Node*)&node->node);

    // Try to merge with buddy if possible
    while (node->parent && node->buddy && node->buddy->is_free) {
        BuddyNode* buddy = node->buddy;
        
        // Determine left and right buddies (lower address first)
        BuddyNode* left = node;
        BuddyNode* right = buddy;
        
        node = BuddyAllocator_merge_blocks(a, left, right);
        if (!node) {
            printf("Merge failed or not possible\n");
            break;
        }
        
        
        list_push_front(a->free_lists[node->level], (Node*)&node->node);
    }
    
    return (void*)0;
}

int BuddyAllocator_print_state(BuddyAllocator* a) {
    printf("Buddy Allocator state:\n");
    printf("\tTotal size: %zu bytes\n", a->total_size);
    printf("\tNumber of levels: %d\n", a->num_levels);
    printf("\tMin block size: %zu bytes\n", a->min_block_size);
    printf("\tMemory start: %p\n", a->memory_start);
    SlabAllocator_print_state(&a->node_allocator);
    printf("\tFree lists:\n");
    for (int i = 0; i < a->num_levels; i++) {
        printf("Level %d (block size %zu):\n", i, a->total_size / (1 << i));
        
        Node* current = a->free_lists[i]->head;
        int blocks_in_level = 1 << i;
        for (int b = 0; b < blocks_in_level; b++) {
            if (current) {
                printf("■ "); // Free block
                current = current->next;
            } else {
                printf("□ "); // Used/split block
            }
        }
        printf("\n");
    }
    return 0;
}