#include <buddy_allocator.h>

static struct Buddies BuddyAllocator_divide_block(BuddyAllocator* a, BuddyNode* parent) {
    struct Buddies buddies;
    if (!a || !parent) {
        printf("Error: Invalid parameters!\n");
        buddies.left_buddy = NULL;
        buddies.right_buddy = NULL;
        return buddies;
    }
    buddies.left_buddy = SlabAllocator_alloc(&(a->node_allocator));
    buddies.right_buddy = SlabAllocator_alloc(&(a->node_allocator));

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
        printf("Error: Invalid parameters!\n");
        return NULL;
    }

    // The parent node will represent the merged block
    BuddyNode* parent = left->parent;
    if (!parent) {
        printf("Error: Cannot merge, parent is NULL\n");
        return NULL;
    }

    // Free the left and right buddy nodes
    SlabAllocator_release(&(a->node_allocator), left);
    SlabAllocator_release(&(a->node_allocator), right);
    parent->is_free = 1;
    return parent;
}


void* BuddyAllocator_init(Allocator* alloc, ...) {
    va_list args;
    va_start(args, alloc);
    
    BuddyAllocator* buddy = (BuddyAllocator*)alloc;
    size_t total_size = va_arg(args, size_t);
    int num_levels = va_arg(args, int);
    
    va_end(args);

    // Calculate max number of nodes based on total size and num_levels
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
    // Print size of nodes at each level
    size_t level_size = total_size;
    printf("\nNode sizes at each level:\n");
    for (int i = 0; i < num_levels; i++) {
        printf("  Level %d: %zu bytes\n", i, level_size);
        level_size /= 2;
    }
    #endif

    // Initialize the allocator fields
    buddy->memory_start = mmap(NULL, total_size, PROT_READ | PROT_WRITE, 
                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (buddy->memory_start == MAP_FAILED) {
        printf("Failed to allocate memory!\n");
        return NULL;
    }

    buddy->total_size = total_size;
    buddy->num_levels = num_levels;
    buddy->min_block_size = total_size >> (num_levels - 1);

    // initialize list allocator
    SlabAllocator* list_allocator = SlabAllocator_create(&(buddy->list_allocator), sizeof(DoubleLinkedList), num_levels);
    if (!list_allocator) {
        printf("Failed to create list allocator!\n");
        return NULL;
    }
    buddy->list_allocator = *list_allocator;
    // Initialize free lists
    for (int i = 0; i < num_levels; i++) {
        buddy->free_lists[i] = (DoubleLinkedList*)SlabAllocator_alloc(&buddy->list_allocator);
        if (!buddy->free_lists[i]) {
            printf("Failed to allocate free list!\n");
            return NULL;
        }
        list_create(buddy->free_lists[i]);
    }

    // Initialize node allocator
    SlabAllocator* slab = SlabAllocator_create(&(buddy->node_allocator), sizeof(BuddyNode), max_nodes);
    if (!slab) {
        printf("Failed to create slab allocator!\n");
        return NULL;
    }
    buddy->node_allocator = *slab;

    // Add to first level free list the first BuddyNode in the memory 
    // allocate a BuddyNode from the slab allocator
    BuddyNode* first_node = (BuddyNode*)SlabAllocator_alloc(&buddy->node_allocator);
    if (!first_node) {
        printf("Failed to create first node!\n");
        return NULL;
    }

    // initialize the first node
    first_node->size = total_size;
    first_node->data = buddy->memory_start;
    first_node->level = 0;
    first_node->is_free = 1;
    first_node->buddy = NULL;
    first_node->parent = NULL;
    
    // add the first node to the first level free list
    list_push_front(buddy->free_lists[0], (Node*)&first_node->node);

    // Initialize the function pointers
    buddy->base.init = BuddyAllocator_init;
    buddy->base.dest = BuddyAllocator_destructor;
    buddy->base.malloc = BuddyAllocator_malloc;
    buddy->base.free = BuddyAllocator_free;
    return buddy;
}

BuddyAllocator* BuddyAllocator_create(BuddyAllocator* a, size_t total_size, int num_levels) {
    if (!a || total_size == 0 || num_levels == 0) {
        printf("Failed to create: invalid parameters!\n");
        return NULL;
    } 
       
    if (!BuddyAllocator_init((Allocator*)a, total_size, num_levels)) {
        printf("Failed to initialize BuddyAllocator!\n");
        return NULL;
    }
    
    return a;
}

void* BuddyAllocator_destructor(Allocator* alloc, ...) {
    if (!alloc) {
        printf("Error: NULL allocator passed to destructor\n");
        return (void*)-1;
    }

    BuddyAllocator* buddy = (BuddyAllocator*)alloc;
    if (!buddy->memory_start) {
        printf("Error: NULL memory_start in destructor\n");
        return (void*)-1;
    }

    if (munmap(buddy->memory_start, buddy->total_size) != 0) {
        printf("Error: Failed to unmap memory in destructor\n");
        return (void*)-1;
    }

    if (SlabAllocator_destroy(&buddy->list_allocator) != 0) {
        printf("Error: Failed to destroy list allocator\n");
        return (void*)-1;
    }

    if (SlabAllocator_destroy(&buddy->node_allocator) != 0) {
        printf("Error: Failed to destroy node allocator\n"); 
        return (void*)-1;
    }
    #ifdef VERBOSE
    printf("buddy allocator destroyed\n");
    #endif
    return (void*)0;
}

int BuddyAllocator_destroy(BuddyAllocator* a) {
    if (!a) return -1;
    if (BuddyAllocator_destructor((Allocator*)a) != 0) {
        printf("Error: Failed to destroy buddy allocator\n");
        return -1;
    }
    return 0;
}

void* BuddyAllocator_malloc(Allocator* alloc, ...) {
    va_list args;
    va_start(args, alloc);
    BuddyAllocator* buddy = (BuddyAllocator*)alloc;
    int level = va_arg(args, int);
    va_end(args);
    #ifdef VERBOSE
    printf("Allocating at level %d\n", level);
    #endif
    // Check if there's a free block at this level
    if (!buddy->free_lists[level]) {
        printf("No free list at level %d\n", level);
        return NULL;
    }

    BuddyNode* free_block = (BuddyNode*) list_pop_front(buddy->free_lists[level]);
    #ifdef VERBOSE
    if(!free_block) {
        printf("No block found at level %d\n", level);
    } else {
        printf("Found block at level %d: %p\n", level, (void*)free_block);
    }
    #endif
    
    int current_level = level;
    
    // If no block at requested level, look for larger blocks
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
            
            // Split block until we reach desired level
            while (current_level < level) {
                struct Buddies buddies = BuddyAllocator_divide_block(buddy, free_block);
                if (buddies.left_buddy == NULL || buddies.right_buddy == NULL) {
                    printf("Failed to split block!\n");
                    return NULL;
                }
                
                // Add buddy to free list
                list_push_back(buddy->free_lists[current_level + 1], (Node *) buddies.right_buddy);
                
                // Continue with the other half
                free_block = buddies.left_buddy;
                current_level++;
            }
            break;
        }
    }
    
    if (!free_block) {
        printf("No free blocks available at any level\n");
        return NULL;
    }

    #ifdef VERBOSE
    printf("Found free block at level %d: %p\n", level, (void*)free_block);
    #endif
    return free_block;

}

void* BuddyAllocator_alloc(BuddyAllocator* a, size_t size) {
    if (!a || size == 0) {
        printf("Error: NULL allocator or invalid size!\n");
        return NULL;
    }

    // Reserve space for BuddyNode* metadata
    size_t meta_size = sizeof(BuddyNode*);
    size_t adjusted_size = size + meta_size;
    // Align to 8 bytes
    if (adjusted_size % 8 != 0) adjusted_size += 8 - (adjusted_size % 8);

    // Calculate initial block size and level
    size_t block_size = a->total_size;
    int level = 0;

    // Find smallest block size that fits the request
    while (block_size / 2 >= adjusted_size && level < a->num_levels - 1) {
        block_size /= 2;
        level++;
    }
    #ifdef VERBOSE
    printf("Level required for size %zu: %d (block size: %zu)\n", size, level, block_size);
    #endif

    if (block_size < a->min_block_size) {
        printf("Error: Requested size too small for minimum block size\n");
        return NULL;
    }

    BuddyNode* node = a->base.malloc((Allocator*)a, level);
    if (!node) {
        printf("Failed to allocate node!\n");
        return NULL;
    }

    // Store the BuddyNode* just before the user data
    void* user_data = (char*)node->data + meta_size;
    *((BuddyNode**)((char*)user_data - meta_size)) = node;

    return user_data;
}

void *BuddyAllocator_free(Allocator* alloc, ...) {
    va_list args;
    va_start(args, alloc);
    BuddyAllocator* a = (BuddyAllocator*)alloc;
    BuddyNode* node = va_arg(args, BuddyNode*);
    va_end(args);

    // Mark the node as free
    node->is_free = 1;
    // Try to merge with buddy if possible
    while (node->parent && node->buddy && node->buddy->is_free) {
        // Remove buddy from free list
        list_detach(a->free_lists[node->level], (Node*)&node->buddy->node);
        // Remove this node from free list if present
        list_detach(a->free_lists[node->level], (Node*)&node->node);
        
        #ifdef VERBOSE
        printf("\t Reunifying nodes! \n");
        #endif

        // Merge nodes: parent becomes the merged free block
        node = BuddyAllocator_merge_blocks(a, node < node->buddy ? node : node->buddy,
                                              node > node->buddy ? node : node->buddy);
        if (!node) {
            printf("Error: Failed to merge buddies\n");
            return (void*) -1;
        }
        node->is_free = 1;
    }
    // Add the (possibly merged) node to the free list at its level
    list_push_front(a->free_lists[node->level], (Node*)&node->node);
    return (void*) 0;

}

void BuddyAllocator_release(BuddyAllocator* a, void* ptr) {
    if(!a || !ptr) {
        printf("Error: NULL allocator or pointer passed to release\n");
        return;
    }
    // Retrieve BuddyNode* from metadata before the user pointer
    BuddyNode* node = *((BuddyNode**)((char*)ptr - sizeof(BuddyNode*)));
    if (!node) {
        printf("Error: NULL node in release\n");
        return;
    }
    if(node->is_free) {
        printf("Error: Attempting to release an already free block\n");
        return;
    }
    #ifdef VERBOSE
    printf("Releasing block at %p, size %zu, level %d\n", (void*)node->data, node->size, node->level);
    #endif
    uint64_t r = (uint64_t)((Allocator*)a->base.free((Allocator*)a, node));
    if (r != 0) {
        printf("Error: Failed to release block\n");
    }
    #ifdef VERBOSE
    printf("Block released and merged if possible\n");
    #endif
}

int BuddyAllocator_print_state(BuddyAllocator* a) {
    printf("Buddy Allocator state:\n");
    printf("  Total size: %zu bytes\n", a->total_size);
    printf("  Number of levels: %d\n", a->num_levels);
    printf("  Min block size: %zu bytes\n", a->min_block_size);
    printf("  Memory start: %p\n", a->memory_start);
    printf("  Free lists:\n");
    for (int i = 0; i < a->num_levels; i++) {
        printf("Level %d (block size %zu):\n", i, a->total_size / (1 << i));
        
        // Print indentation based on level
        for (int indent = 0; indent < i; indent++) {
            printf("  ");
        }
        
        // Print blocks at this level
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
