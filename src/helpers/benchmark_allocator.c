#include <benchmark.h>

int Allocator_malloc_free(struct AllocatorConfig config, void *instructions, long remaining, int n_pointers) {
    int result = 0;
    char *pointers[n_pointers];
    long allocation_counter = 0;
    
    // Initialize pointers array safely
    memset(pointers, 0, sizeof(char*) * n_pointers);
    
    const unsigned char *instr = (unsigned char *)instructions;
    const unsigned char *end = instr + remaining;
    char instruction_str[256];
    
    // Process instructions in single pass
    while (instr < end) {
        const unsigned char *line_start = instr;
        // Find next newline or end of buffer
        while (instr < end && *instr != '\n') instr++;
        size_t line_len = instr - line_start;
        // Skip empty lines and comments
        if (line_len == 0 || *line_start == '%') {
            instr++;
            continue;
        }
        // Handle line overflow safely
        if (line_len >= sizeof(instruction_str)) {
            fprintf(stderr, "Line too long (%zu bytes), max %zu allowed\n",
                    line_len, sizeof(instruction_str)-1);
            result = -1;
            instr++;
            continue;
        }
        // Process valid instruction
        memcpy(instruction_str, line_start, line_len);
        instruction_str[line_len] = '\0';
				#ifdef VERBOSE
        printf("Instruction: %s -- ", instruction_str);
				#endif
        int ret = parse_allocator_request(instruction_str, &config, pointers, 
                                        n_pointers, &allocation_counter);
        if (ret != 0 && result == 0) {
            result = ret;  // Capture first error
        }
        // Advance past newline if present
        if (instr < end && *instr == '\n') instr++;
    }
    
    // Check for memory leaks
    int leaks_found = 0;
    for (int i = 0; i < n_pointers; ++i) {
        if (pointers[i] != NULL) {
						#ifdef DEBUG
            printf(RED "Pointer at index %d was not freed: %p\n" RESET, i, pointers[i]);
						#endif
            leaks_found = 1;
        }
    }
    
    return leaks_found ? -1 : result;
}

int Allocator_benchmark_initialize(const char *file_name) {
    int result = 0;
    
    // Open the benchmark file
    char full_path[256];
    snprintf(full_path, sizeof(full_path), "%s/%s", BENCHMARK_FOLDER, file_name);
    FILE *file = fopen(full_path, "r");
    if (!file) {
        perror("Failed to open benchmark file");
        return -1;
    }
    
    // Parse allocator type
    enum AllocatorType type = parse_allocator_create(file);
    if (type < 0) {
        fclose(file);
        return -1;
    }

    struct AllocatorConfig config;
    config.type = type;
    config.allocator = NULL;
    config.is_variable_size_allocation = (type > VARIABLE_ALLOCATION_DELIMITER);

    union AllocatorConfigData params = parse_allocator_create_parameters(file, &config);
    void *map_base = NULL;
    long delta = 0;
    long remaining = 0;
    union GeneralAllocator allocator;

    // Create the appropriate allocator
    switch (type) {
        case SLAB_ALLOCATOR:
            printf("Running SLAB_ALLOCATOR benchmark...\n");
            config.allocator = (Allocator*) &allocator;
            SlabAllocator_create((SlabAllocator *) &allocator, params.slab.slab_size, params.slab.n_slabs);
            break;
        case BUDDY_ALLOCATOR:
            printf("Running BUDDY_ALLOCATOR benchmark...\n");
            config.allocator = (Allocator*) &allocator;
            BuddyAllocator_create((BuddyAllocator *)&allocator, params.buddy.total_size, params.buddy.max_levels);
            break;
        case BITMAP_BUDDY_ALLOCATOR:
            printf("Running BITMAP_BUDDY_ALLOCATOR benchmark...\n");
            config.allocator = (Allocator*) &allocator;
            BitmapBuddyAllocator_create((BitmapBuddyAllocator *)&allocator, params.buddy.total_size, params.buddy.max_levels);
            break;
        default:
            fprintf(stderr, "Unknown allocator type: %d\n", type);
            fclose(file);
            return -1;
    }

    // Validate allocator was created
    if (!config.allocator) {
        fprintf(stderr, "Failed to create allocator\n");
        fclose(file);
        return -1;
    }

    // Determine number of pointers based on allocator type
    int n_pointers = 0;
    switch(config.type) {
        case SLAB_ALLOCATOR:
            n_pointers = ((SlabAllocator *)config.allocator)->free_list_size_max;
            break;
        case BUDDY_ALLOCATOR:
            n_pointers = 1 << (((BuddyAllocator *)config.allocator)->num_levels - 1);
            break;
        case BITMAP_BUDDY_ALLOCATOR:
            n_pointers = ((BitmapBuddyAllocator *)config.allocator)->bitmap.num_bits;
            break;
        default:
            fprintf(stderr, "Unknown allocator type: %d\n", type);
            result = -1;
            goto cleanup;
    }

    // Memory map the file for reading instructions
    long offset = ftell(file); // Get current file position (after parsing headers)
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    remaining = file_size - offset;
    if (remaining < 0) {
        result = -1;
        goto cleanup;
    }

    long page_size = sysconf(_SC_PAGE_SIZE);
    long page_offset = offset & ~(page_size - 1);
    delta = offset - page_offset;
    map_base = mmap(NULL, remaining + delta, PROT_READ, MAP_PRIVATE, fileno(file), page_offset);
    if (map_base == MAP_FAILED) {
        perror("mmap failed");
        result = -1;
        goto cleanup;
    }

    void *instructions = (unsigned char *)map_base + delta;

    // Validate input parameters
    if (!config.allocator || !instructions || remaining <= 0) {
        fprintf(stderr, "Invalid parameters for Allocator benchmark\n");
        result = -1;
        goto cleanup;
    }

    // Execute the benchmark
#ifdef TIME
    // Setup timing variables
    struct timespec start, end;
    double elapsed_seconds = 0.0;
    
    // Start timing
    if (clock_gettime(CLOCK_MONOTONIC, &start) != 0) {
        perror("Failed to get start time");
        result = -1;
    } else {
        // Execute benchmark
        result = Allocator_malloc_free(config, instructions, remaining, n_pointers);
        
        // End timing
        if (clock_gettime(CLOCK_MONOTONIC, &end) != 0) {
            perror("Failed to get end time");
            result = -1;
        } else {
            // Calculate elapsed time in seconds
            elapsed_seconds = (end.tv_sec - start.tv_sec);
            elapsed_seconds += (end.tv_nsec - start.tv_nsec) / 1e9;
            
            printf("\nBenchmark timing results:\n");
            printf("  Total time: %.6f seconds\n", elapsed_seconds);
            printf("  Operations: %ld\n", remaining);
        }
    }
    #else
    result = Allocator_malloc_free(config, instructions, remaining, n_pointers);
    #endif

    if (result < 0) {
        fprintf(stderr, RED "Failed to parse some allocator requests\n" RESET);
        result = -1;
    } else if (result == 0) {
        printf(GREEN "Allocator benchmark completed successfully\n" RESET);
    }

    #ifdef VERBOSE
    switch (type) {
        case SLAB_ALLOCATOR:
            SlabAllocator_print_state((SlabAllocator *)config.allocator);
            break;
        case BUDDY_ALLOCATOR:
            BuddyAllocator_print_state((BuddyAllocator *)config.allocator);
            break;
        case BITMAP_BUDDY_ALLOCATOR:
            BitmapBuddyAllocator_print_state((BitmapBuddyAllocator *)config.allocator);
            break;
        default:
            fprintf(stderr, RED "Unknown allocator type: %d\n" RESET, type);
    }
    #endif

cleanup:
    // Cleanup resources
    if (config.allocator && config.allocator->dest) {
        config.allocator->dest(config.allocator);
    }
    if (map_base != NULL && map_base != MAP_FAILED) {
        munmap(map_base, remaining + delta);
    }
    fclose(file);
    return result;
}