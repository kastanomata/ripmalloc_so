#include <buddy_allocator.h>

static struct Buddies BuddyAllocator_divide_block(BuddyAllocator* a, BuddyNode* parent) {
    struct Buddies buddies;
    if (!a || !parent) {
        #ifdef DEBUG
        printf(RED "ERROR: Invalid parameters in divide_block!\n" RESET);
        #endif
        buddies.left_buddy = NULL;
        buddies.right_buddy = NULL;
        return buddies;
    }
    buddies.left_buddy = SlabAllocator_malloc(&(a->node_allocator));
    buddies.right_buddy = SlabAllocator_malloc(&(a->node_allocator));

    parent->is_free = 0;

    buddies.left_buddy->size = parent->size / 2;
    buddies.left_buddy->data = parent->data;
    buddies.left_buddy->level = parent->level + 1;
    buddies.left_buddy->is_free = 0;
    buddies.left_buddy->parent = parent;
    buddies.left_buddy->buddy = buddies.right_buddy;

    buddies.right_buddy->size = parent->size / 2;
    buddies.right_buddy->data = parent->data + parent->size / 2;
    buddies.right_buddy->level = parent->level + 1;
    buddies.right_buddy->is_free = 1;
    buddies.right_buddy->parent = parent;
    buddies.right_buddy->buddy = buddies.left_buddy;

    return buddies;
}

static BuddyNode* BuddyAllocator_merge_blocks(BuddyAllocator* a, BuddyNode* left, BuddyNode* right) {
    if (!a || !left || !right) {
        #ifdef DEBUG
        printf(RED "ERROR: Invalid parameters in merge_blocks!\n" RESET);
        #endif
        return NULL;
    }

    // Check they are mutual buddies
    if (left->buddy != right || right->buddy != left || left->level != right->level) {
        #ifdef DEBUG
        printf(RED "ERROR: Nodes are not mutual buddies!\n" RESET);
        #endif
        return NULL;
    }

    BuddyNode* parent = left->parent;
    if (!parent || parent != right->parent) {
        #ifdef DEBUG
        printf(RED "ERROR: Parent mismatch!\n" RESET);
        #endif
        return NULL;
    }
    if (parent->is_free == true) {
        #ifdef DEBUG
        printf(RED "ERROR: Parent node is alreadyreserve free!\n" RESET);
        #endif
        return NULL;
    }

    // Release the child nodes
    SlabAllocator_free(&(a->node_allocator), left);
    SlabAllocator_free(&(a->node_allocator), right);

    list_detach(a->free_lists[left->level], (Node*)&left->node);
    list_detach(a->free_lists[right->level], (Node*)&right->node);

    parent->is_free = 1;
    return parent;
}

void* BuddyAllocator_init(Allocator* alloc, ...) {
    va_list args;
    va_start(args, alloc);
    
    BuddyAllocator* buddy = (BuddyAllocator*)alloc;
    size_t total_size = va_arg(args, size_t);
    int num_levels = va_arg(args, int) + 1; // +1 for the root level
    
    va_end(args);

    // Calculate max number of nodes
    size_t max_nodes = 0;
    size_t level_nodes = 1;
    for (int i = 0; i < num_levels; i++) {
        max_nodes += level_nodes;
        level_nodes *= 2;
    }
    
    #ifdef VERBOSE
    printf("Buddy Allocator initialized with:\n");
    printf("  Total size: %zu bytes\n", total_size);
    printf("  Number of levels: %d\n", num_levels);
    printf("  Max nodes: %zu\n", max_nodes);
    size_t level_size = total_size;
    printf("\nNode sizes at each level:\n");
    for (int i = 0; i < num_levels; i++) {
        printf("  Level %d: %zu bytes\n", i, level_size);
        level_size /= 2;
    }
    #endif

    // Initialize memory
    buddy->memory_start = mmap(NULL, total_size, PROT_READ | PROT_WRITE, 
                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (buddy->memory_start == MAP_FAILED) {
        #ifdef DEBUG
        printf(RED "ERROR: Failed to allocate memory in init!\n" RESET);
        #endif
        return NULL;
    }

    buddy->total_size = total_size;
    buddy->num_levels = num_levels;
    buddy->min_block_size = total_size >> (num_levels - 1);

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
    buddy->base.init = BuddyAllocator_init;
    buddy->base.dest = BuddyAllocator_cleanup;
    buddy->base.malloc = BuddyAllocator_reserve;
    buddy->base.free = BuddyAllocator_release;
    return buddy;
}

BuddyAllocator* BuddyAllocator_create(BuddyAllocator* a, size_t total_size, int num_levels) {
    if (!a || total_size == 0 || num_levels == 0) {
        #ifdef DEBUG
        printf(RED "ERROR: Invalid parameters in create!\n" RESET);
        #endif
        return NULL;
    } 
       
    if (!BuddyAllocator_init((Allocator*)a, total_size, num_levels)) {
        #ifdef DEBUG
        printf(RED "ERROR: Failed to initialize BuddyAllocator!\n" RESET);
        #endif
        return NULL;
    }
    
    return a;
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
    
    #ifdef VERBOSE
    printf("Buddy allocator destroyed\n");
    #endif
    return (void*)0;
}

int BuddyAllocator_destroy(BuddyAllocator* a) {
    if (!a) {
        #ifdef DEBUG
        printf(RED "ERROR: NULL allocator in destroy\n" RESET);
        #endif
        return -1;
    }
    if (BuddyAllocator_cleanup((Allocator*)a) != 0) {
        #ifdef DEBUG
        printf(RED "ERROR: Failed to destroy buddy allocator\n" RESET);
        #endif
        return -1;
    }
    return 0;
}

void* BuddyAllocator_reserve(Allocator* alloc, ...) {
    va_list args;
    va_start(args, alloc);
    BuddyAllocator* buddy = (BuddyAllocator*)alloc;
    int level = va_arg(args, int);
    va_end(args);
    
    #ifdef VERBOSE
    printf("Allocating at level %d\n", level);
    #endif
    
    if (!buddy->free_lists[level]) {
        #ifdef DEBUG
        printf(RED "ERROR: No free list at level %d\n" RESET, level);
        #endif
        return NULL;
    }

    BuddyNode* free_block = (BuddyNode*) list_pop_front(buddy->free_lists[level]);
    #ifdef VERBOSE
    if(!free_block) {
        printf("No block found at level %d\n",  level);
    }
    #endif
    
    int current_level = level;
    
    // Search larger blocks if needed
    while (!free_block && current_level > 0) {
        current_level--;
        #ifdef VERBOSE
        printf("Looking for block at level %d\n", current_level);
        #endif
        free_block = (BuddyNode*) list_pop_front(buddy->free_lists[current_level]);
        if (free_block) {
            #ifdef VERBOSE
            printf("Found block at level %d, splitting...\n", current_level);
            #endif
            
            // Split block to desired level
            while (current_level < level) {
                struct Buddies buddies = BuddyAllocator_divide_block(buddy, free_block);
                if (buddies.left_buddy == NULL || buddies.right_buddy == NULL) {
                    #ifdef DEBUG
                    printf(RED "ERROR: Failed to split block!\n" RESET);
                    #endif
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
        return NULL;
    }

    #ifdef VERBOSE
    printf("Found free block at level %d: %p\n", level, (void*)free_block);
    #endif
    return free_block;
}

void* BuddyAllocator_malloc(BuddyAllocator* a, size_t size) {
    if (!a || size == 0) {
        #ifdef DEBUG
        printf(RED "ERROR: NULL allocator or invalid size in alloc!\n" RESET);
        #endif
        return NULL;
    }

    // Reserve space for metadata
    size_t meta_size = sizeof(BuddyNode*);
    size_t adjusted_size = size + meta_size;
    if (adjusted_size % 8 != 0) adjusted_size += 8 - (adjusted_size % 8);
    
    // Find appropriate block size
    size_t block_size = a->total_size;
    int level = 0;
    while (block_size / 2 >= adjusted_size && level < a->num_levels - 1) {
        block_size /= 2;
        level++;
    }
    
    #ifdef VERBOSE
    printf("Requested size: %zu, adjusted size with metadata: %zu\n", size, adjusted_size);
    printf("Level required for size %zu: %d (block size: %zu)\n", size, level, block_size);
    #endif    

    if (block_size < a->min_block_size) {
        #ifdef DEBUG
        printf(RED "ERROR: Requested size too small for minimum block size\n" RESET);
        #endif
        return NULL;
    }

    BuddyNode* node = a->base.malloc((Allocator*)a, level);
    if (!node) {
        #ifdef DEBUG
        printf(RED "ERROR: Failed to allocate node!\n" RESET);
        #endif
        return NULL;
    }
    node->is_free = 0; // Mark as used

    // Store metadata
    void* user_data = (char*)node->data + meta_size;
    *((BuddyNode**)((char*)user_data - meta_size)) = node;

    return user_data;
}

void *BuddyAllocator_release(Allocator* alloc, ...) {
    va_list args;
    va_start(args, alloc);
    BuddyAllocator* a = (BuddyAllocator*)alloc;
    BuddyNode* node = va_arg(args, BuddyNode*);
    va_end(args);

    if (!node) {
        #ifdef DEBUG
        printf(RED "ERROR: NULL node in free!\n" RESET);
        #endif
        return (void*)-1;
    }

    #ifdef VERBOSE
    printf("Freeing block at %p, size %zu, level %d\n", 
           node->data, node->size, node->level);
    #endif

    node->is_free = 1;
    list_push_front(a->free_lists[node->level], (Node*)&node->node);

    // Try to merge with buddy if possible
    while (node->parent && node->buddy && node->buddy->is_free) {
        BuddyNode* buddy = node->buddy;

        #ifdef VERBOSE
        printf("Merging blocks: [%p] and [%p] at level %d\n", 
               node->data, buddy->data, node->level);
        #endif

        // Determine left and right buddies (lower address first)
        BuddyNode* left = node;
        BuddyNode* right = buddy;

        node = BuddyAllocator_merge_blocks(a, left, right);
        if (!node) {
            #ifdef DEBUG
            printf(RED "ERROR: Failed to merge buddies\n" RESET);
            #endif
            break;
        }
        
        // After merging, add parent to its free list
        list_push_front(a->free_lists[node->level], (Node*)&node->node);
    }
    
    return (void*)0;
}

void BuddyAllocator_free(BuddyAllocator* a, void* ptr) {
    if(!a || !ptr) {
        #ifdef DEBUG
        printf(RED "ERROR: NULL allocator or pointer in release\n" RESET);
        #endif
        return;
    }
    
    BuddyNode* node = *((BuddyNode**)((char*)ptr - sizeof(BuddyNode*)));
    if (!node) {
        #ifdef DEBUG
        printf(RED "ERROR: NULL node in release\n" RESET);
        #endif
        return;
    }
    
    if(node->is_free) {
        #ifdef DEBUG
        printf(RED "ERROR: Attempting to release an already free block\n" RESET);
        #endif
        return;
    }
    
    #ifdef VERBOSE
    printf("Releasing block at %p, size %zu, level %d\n", 
           (void*)node->data, node->size, node->level);
    #endif
    
    uint64_t r = (uint64_t)((Allocator*)a->base.free((Allocator*)a, node));
    if (r != 0) {
        #ifdef DEBUG
        printf(RED "ERROR: Failed to release block\n" RESET);
        #endif
    }
    
    #ifdef VERBOSE
    printf("Block released and merged if possible\n");
    #endif
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