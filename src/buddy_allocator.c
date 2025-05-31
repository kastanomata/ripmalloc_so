#include <buddy_allocator.h>


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
    printf("list allocator created\n");
    // Initialize free lists
    for (int i = 0; i < num_levels; i++) {
        buddy->free_lists[i] = (DoubleLinkedList*)SlabAllocator_alloc(&buddy->list_allocator);
        if (!buddy->free_lists[i]) {
            printf("Failed to allocate free list!\n");
            return NULL;
        }
        list_create(buddy->free_lists[i]);
        list_print(buddy->free_lists[i]);
    }
    printf("free lists initialized\n");

    // Initialize node allocator
    SlabAllocator* slab = SlabAllocator_create(&(buddy->node_allocator), sizeof(BuddyNode), max_nodes);
    if (!slab) {
        printf("Failed to create slab allocator!\n");
        return NULL;
    }
    buddy->node_allocator = *slab;
    printf("node allocator created\n");

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

    printf("first node: %p\n", first_node);
    printf("first node size: %zu\n", first_node->size);
    printf("first node level: %d\n", first_node->level);
    printf("first node is_free: %d\n", first_node->is_free);
    printf("first node buddy: %p\n", first_node->buddy);
    printf("first node parent: %p\n", first_node->parent);
    
    // add the first node to the first level free list
    list_push_front(buddy->free_lists[0], (Node*)&first_node->node);

    list_print(buddy->free_lists[0]);

    // Initialize the function pointers
    buddy->base.init = BuddyAllocator_init;
    buddy->base.dest = BuddyAllocator_destructor;
    // buddy->base.malloc = BuddyAllocator_malloc;
    // buddy->base.free = BuddyAllocator_free;
    printf("buddy allocator initialized\n");
    BuddyAllocator_print_state(buddy);
    return buddy;
}

BuddyAllocator* BuddyAllocator_create(BuddyAllocator* a, size_t total_size, int num_levels) {
    if (!a || total_size == 0 || num_levels == 0) {
        #ifdef VERBOSE
        printf("Failed to create: invalid parameters!\n");
        #endif
        return NULL;
    } 
       
    
    if (!BuddyAllocator_init((Allocator*)a, total_size, num_levels)) {
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

    printf("buddy allocator destroyed\n");
    return (void*)0;
}


int BuddyAllocator_print_state(BuddyAllocator* a) {
    printf("Buddy Allocator state:\n");
    printf("  Total size: %zu bytes\n", a->total_size);
    printf("  Number of levels: %d\n", a->num_levels);
    printf("  Min block size: %zu bytes\n", a->min_block_size);
    printf("  Memory start: %p\n", a->memory_start);
    printf("  Free lists:\n");
    for (int i = 0; i < a->num_levels; i++) {
        printf("    Level %d: ", i);
        list_print(a->free_lists[i]);
    }

    return 0;
}

int BuddyAllocator_destroy(BuddyAllocator* a) {
    if (!a) return -1;
    if (BuddyAllocator_destructor((Allocator*)a) != 0) {
        printf("Error: Failed to destroy buddy allocator\n");
        return -1;
    }
    return 0;
}


