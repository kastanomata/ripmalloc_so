#include <helpers/freeform.h>

static int freeform_slab_allocator() {
    printf("=== START FREEFORM SLAB ALLOCATOR ===\n");
    SlabAllocator slab_allocator;
    
    int slab_size, num_slabs;
    while (true) {
        printf("Enter slab size (in bytes): ");
        scanf("%d", &slab_size);
        if (slab_size > 0) {
            break;
        }
        printf("Invalid slab size. Please enter a positive value.\n");
    }
    while (true) {
        printf("Enter number of slabs: ");
        scanf("%d", &num_slabs);
        if (num_slabs > 0) {
            break;
        }
        printf("Invalid number of slabs. Please enter a positive value.\n");
    }

    void* allocations[num_slabs];
    int allocation_count = 0;
    memset(allocations, 0, sizeof(allocations));
    
    SlabAllocator_create(&slab_allocator, slab_size, num_slabs);
    printf("Slab Allocator created with slab size: %d bytes and number of slabs: %d\n", slab_size, num_slabs);

    while (true) {
        char command[256];
        printf("Enter command (alloc, free, print, exit): ");
        scanf("%s", command);

        if (strcmp(command, "exit") == 0 || strcmp(command, "quit") == 0 || strcmp(command, "q") == 0) {
            break;
        } else if (strcmp(command, "alloc") == 0) {
            if (allocation_count >= num_slabs) {
                printf("Maximum number of allocations reached (%d)\n", num_slabs);
                continue;
            }
            
            void* ptr = SlabAllocator_alloc(&slab_allocator);
            if (ptr) {
                allocations[allocation_count++] = ptr;
                printf("Allocated %zu bytes at %p (index %d)\n", slab_allocator.slab_size, ptr, allocation_count-1);
            } else {
                printf("Allocation failed\n");
            }
        } else if (strcmp(command, "free") == 0) {
            if (allocation_count == 0) {
                printf("No allocations to free\n");
                continue;
            }
            
            printf("Current allocations:\n");
            for (int i = 0; i < allocation_count; i++) {
                printf("[%d] %p\n", i, allocations[i]);
            }
            
            int index;
            printf("Enter index to free: ");
            scanf("%d", &index);
            
            if (index < 0 || index >= allocation_count) {
                printf("Invalid index\n");
                continue;
            }
            
            void* ptr = allocations[index];
            SlabAllocator_release(&slab_allocator, ptr);
            
            // Remove from array by shifting remaining elements
            for (int i = index; i < allocation_count - 1; i++) {
                allocations[i] = allocations[i + 1];
            }
            allocation_count--;
            
            printf("Freed memory at %p (index %d)\n", ptr, index);
        } else if (strcmp(command, "print") == 0) {
            SlabAllocator_print_state(&slab_allocator);
            
            printf("Current allocations:\n");
            for (int i = 0; i < allocation_count; i++) {
                printf("[%d] %p\n", i, allocations[i]);
            }
        } else {
            printf("Unknown command: %s\n", command);
        }
    }
    
    // // Free any remaining allocations
    // for (int i = 0; i < allocation_count; i++) {
    //     SlabAllocator_release(&slab_allocator, allocations[i]);
    // }
    
    SlabAllocator_destroy(&slab_allocator);
    printf("=== END FREEFORM SLAB ALLOCATOR ===\n");
    return 0;
}

static int freeform_buddy_allocator() {
    printf("=== START FREEFORM BUDDY ALLOCATOR ===\n");
    BuddyAllocator buddy_allocator;
    
    int total_size, max_levels;
    while (true) {
        printf("Enter total size for Buddy Allocator (in bytes): ");
        scanf("%d", &total_size);
        if (total_size > 0) {
            break;
        } else if (total_size == 0) {
           total_size = PAGESIZE; // Use system page size if 0 is entered
           break;
        } else {
        printf("Invalid total size. Please enter a positive value.\n");
        }
    }
    while (true) {
        printf("Enter max levels (1-32): ");
        scanf("%d", &max_levels);
        if (max_levels > 0 && max_levels <= MAX_LEVELS) {
            break;
        } else if (max_levels == 0) {
            max_levels = DEF_LEVELS_NUMBER; // Default to 4 levels if 0 is entered
            break;
        }
        printf("Invalid number of levels. \
                Please enter a value between 1 and %d, or 0 for default (%d).\n",
                MAX_LEVELS, DEF_LEVELS_NUMBER);
    }
    int max_allocations = (1 << max_levels) - 1;
    int allocation_count = 0;
    void* allocations[max_allocations];
    size_t allocation_sizes[max_allocations];
    memset(allocations, 0, sizeof(allocations));
    memset(allocation_sizes, 0, sizeof(allocation_sizes));
    BuddyAllocator_create(&buddy_allocator, total_size, max_levels);
    printf("Buddy Allocator created with total size: %d bytes and max levels: %d\n", total_size, max_levels);
    
    while (true) {
        char command[256];
        printf("Enter command (alloc, free, print, exit): ");
        scanf("%s", command);
        
        if (strcmp(command, "exit") == 0 || strcmp(command, "quit") == 0 || strcmp(command, "q") == 0) {
            break;
        } else if (strcmp(command, "alloc") == 0) {
            if (allocation_count >= max_allocations) {
                printf("Maximum number of allocations reached (%d)\n", max_allocations);
                continue;
            }
            
            size_t size;
            printf("Enter size to allocate: ");
            scanf("%zu", &size);
            
            void* ptr = BuddyAllocator_alloc(&buddy_allocator, size);
            if (ptr) {
                allocations[allocation_count] = ptr;
                allocation_sizes[allocation_count] = size;
                allocation_count++;
                printf("Allocated %zu bytes at %p (index %d)\n", size, ptr, allocation_count-1);
            } else {
                printf("Allocation failed for %zu bytes\n", size);
            }
            BuddyAllocator_print_state(&buddy_allocator);
        } else if (strcmp(command, "free") == 0) {
            if (allocation_count == 0) {
                printf("No allocations to free\n");
                continue;
            }
            
            printf("Current allocations:\n");
            for (int i = 0; i < allocation_count; i++) {
                printf("[%d] %p (%zu bytes)\n", i, allocations[i], allocation_sizes[i]);
            }
            
            int index;
            printf("Enter index to free: ");
            scanf("%d", &index);
            
            if (index < 0 || index >= allocation_count) {
                printf("Invalid index\n");
                continue;
            }
            
            void* ptr = allocations[index];
            BuddyAllocator_release(&buddy_allocator, ptr);
            
            // Remove from arrays by shifting remaining elements
            for (int i = index; i < allocation_count - 1; i++) {
                allocations[i] = allocations[i + 1];
                allocation_sizes[i] = allocation_sizes[i + 1];
            }
            allocation_count--;
            
            printf("Freed memory at %p (index %d)\n", ptr, index);
            BuddyAllocator_print_state(&buddy_allocator);
        } else if (strcmp(command, "print") == 0) {
            BuddyAllocator_print_state(&buddy_allocator);
            
            printf("Current allocations:\n");
            for (int i = 0; i < allocation_count; i++) {
                printf("[%d] %p (%zu bytes)\n", i, allocations[i], allocation_sizes[i]);
            }
        } else {
            printf("Unknown command: %s\n", command);
        }
    }
    
    // Free any remaining allocations
    for (int i = 0; i < allocation_count; i++) {
        BuddyAllocator_release(&buddy_allocator, allocations[i]);
    }
    
    BuddyAllocator_destroy(&buddy_allocator);
    printf("=== END FREEFORM BUDDY ALLOCATOR ===\n");
    return 0;
}
    

// static int freeform_bitmap_allocator() {
//     // TODO
// }

int freeform() {
    printf("=== START FREEFORM ===\n");
    // Freeform testing area
    while (true) {
        char input[256];
        printf("Enter a command (or 'exit' to quit): ");
        fgets(input, sizeof(input), stdin);
        int exit = strncmp(input, "exit", 4) == 0 || strncmp(input, "quit", 4) == 0 || strncmp(input, "q", 1) == 0;
        if (exit) {
            break;
        }
        // Check for specific commands
        int help = strncmp(input, "help", 4) == 0 || strncmp(input, "h", 1) == 0;
        if (help) {
            printf("Available commands:\n");
            printf("  help, h: Show this help message\n");
            printf("  exit, quit, q: Exit the freeform testing area\n");
            printf("  buddy: Start Buddy Allocator freeform testing\n");
            printf("  slab: Start Slab Allocator freeform testing\n");
            continue;
        }
        if (strncmp(input, "buddy", 5) == 0) {
            freeform_buddy_allocator();
            continue;
        }
        if (strncmp(input, "slab", 4) == 0) {
            freeform_slab_allocator();
            continue;
        }
        // Process the input command
        printf("You entered: %s", input);
    }

    printf("=== END FREEFORM ===\n");
    return 0;
}