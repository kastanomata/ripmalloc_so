#include <benchmark.h>

// Function to count how many characters are remaining in the file after headers
long count_remaining_characters(FILE *file) {
    long offset = ftell(file); // Get current file position (after parsing headers)
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    long remaining = file_size - offset;
    fseek(file, offset, SEEK_SET); // Restore file position
    return (remaining < 0) ? -1 : remaining;
}

int Allocator_malloc_free(struct AllocatorBenchmarkConfig *config, void *instructions, long remaining, int n_pointers) {
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

        
        // Evaluate the instruction
        int ret = parse_allocator_request(instruction_str, config, pointers, 
            n_pointers, &allocation_counter);
        if (ret != 0 && result == 0) {
            result = ret;  // Capture first error
        }
        
        // Ensure we have space for: instruction + ",0\n" or ",1\n" + null terminator
        size_t required_space = line_len + 3 + 1; // instruction + ,0/1 + \n + \0
        if (config->log_offset + required_space >= config->max_log_size) {
            fprintf(stderr, "Log buffer overflow\n");
            result = -1;
            break;
        }
        
        int written;
        if (config->is_variable_size_allocation) {
            // put internal_fragmentation and sparse_free_space
            Allocator * vballocator = config->allocator;
            size_t internal_fragmentation = ((VariableBlockAllocator *) vballocator)->internal_fragmentation;
            size_t sparse_free_space = ((VariableBlockAllocator *) vballocator)->sparse_free_memory;
            written = snprintf((char *)config->log_data + config->log_offset, 
            config->max_log_size - config->log_offset, 
            "%s,%d,%zu,%zu\n", instruction_str, (ret == 0) ? 0 : 1, internal_fragmentation, sparse_free_space);
            // 
        } else {
            written = snprintf((char *)config->log_data + config->log_offset, 
            config->max_log_size - config->log_offset, 
            "%s,%d\n", instruction_str, (ret == 0) ? 0 : 1);
        }
        
        if (written < 0) {
            fprintf(stderr, "Log write error\n");
            result = -1;
            break;
        }
        
        // Verify we didn't exceed expected write size
        if ((size_t)written > (config->max_log_size - config->log_offset - 1)) {
            fprintf(stderr, "Log buffer overflow (unexpected)\n");
            result = -1;
            break;
        }
        
        config->log_offset += written;
        // Advance past newline if present
        if (instr < end && *instr == '\n') instr++;
    }
    
    // Check for memory leaks
    int leaks_found = 0;
    // Prepare to append leak info to the log file
    // size_t leak_log_start = config->log_offset;
    int leak_count = 0;
    for (int i = 0; i < n_pointers; ++i) {
        if (pointers[i] != NULL) {
            #ifdef DEBUG
            printf(RED "Pointer at index %d was not freed: %p\n" RESET, i, pointers[i]);
            #endif
            leaks_found = 1;
            // Write leak info to the log
            int written = snprintf((char *)config->log_data + config->log_offset,
                                   config->max_log_size - config->log_offset,
                                   "# leak: index=%d ptr=%p\n", i, pointers[i]);
            if (written > 0)
                config->log_offset += written;
            leak_count++;
        }
    }
    // Optionally, write a summary line if leaks were found
    if (leak_count > 0) {
        int written = snprintf((char *)config->log_data + config->log_offset,
                               config->max_log_size - config->log_offset,
                               "# total_leaks=%d\n", leak_count);
        if (written > 0)
            config->log_offset += written;
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
    struct AllocatorBenchmarkConfig config;
    // Parse allocator type
    enum AllocatorType type = parse_allocator_create(file);
    if (type < 0) {
        fclose(file);
        return -1;
    }

    // Create or overwrite the log file with .log extension
    char log_path[256];
    snprintf(log_path, sizeof(log_path), "%s/%s", BENCHMARK_FOLDER "/logs", file_name);
    char *dot = strrchr(log_path, '.');
    if (dot) {
        strcpy(dot, ".log");
    } else {
        strncat(log_path, ".log", sizeof(log_path) - strlen(log_path) - 1);
    }

    // Open log file with fopen
    FILE *log_fp = fopen(log_path, "w+");
    if (!log_fp) {
        perror("Failed to create log file");
        goto cleanup;
    }

    // get how many characters should the log be long
    size_t max_log_size = (size_t) count_remaining_characters(file) * 10;
    
    if (ftruncate(fileno(log_fp), max_log_size) != 0) {
        perror("Failed to set log file size");
        goto cleanup;
    }

    void *log_map = mmap(NULL, max_log_size, PROT_READ | PROT_WRITE, MAP_SHARED, fileno(log_fp), 0);
    if (log_map == MAP_FAILED) {
        perror("Failed to mmap log file");
        goto cleanup;
    }

    memset(log_map, EOF, max_log_size);

    // Log file is now memory-mapped at log_map

    config.type = type;
    config.allocator = NULL;
    config.log_data = log_map; // Pointer to mmaped log data
    config.max_log_size = max_log_size;
    config.log_offset = 0;
    config.is_variable_size_allocation = (type > VARIABLE_ALLOCATION_DELIMITER);

    union AllocatorParameterData params = parse_allocator_create_parameters(file, &config);

    void *map_base = NULL;
    long delta = 0;
    long remaining = 0;
    union GeneralAllocator allocator;

    // Create the appropriate allocator
    switch (type) {
        case SLAB_ALLOCATOR:
            printf("Running SLAB_ALLOCATOR benchmark...\n");
            config.log_offset += snprintf((char *)config.log_data + config.log_offset,
                                      config.max_log_size - config.log_offset,
                                      "# type=SLAB_ALLOCATOR\n");
            config.log_offset += snprintf((char *)config.log_data + config.log_offset,
                                      config.max_log_size - config.log_offset,
                                      "# slab_size=%zu,n_slabs=%zu\n",
                                      params.slab.slab_size, params.slab.n_slabs);
            config.allocator = (Allocator*) &allocator;
            SlabAllocator_create((SlabAllocator *) &allocator, params.slab.slab_size, params.slab.n_slabs);
            break;
        case BUDDY_ALLOCATOR:
            config.log_offset += snprintf((char *)config.log_data + config.log_offset,
                                config.max_log_size - config.log_offset,
                                "# type=BUDDY_ALLOCATOR\n");
            config.log_offset += snprintf((char *)config.log_data + config.log_offset,
                                        config.max_log_size - config.log_offset,
                                        "# total_size=%zu,max_levels=%zu\n",
                                        params.buddy.total_size, params.buddy.max_levels);
            printf("Running BUDDY_ALLOCATOR benchmark...\n");
            config.allocator = (Allocator*) &allocator;
            BuddyAllocator_create((BuddyAllocator *)&allocator, params.buddy.total_size, params.buddy.max_levels);
            break;
        case BITMAP_BUDDY_ALLOCATOR:
            config.log_offset += snprintf((char *)config.log_data + config.log_offset,
                        config.max_log_size - config.log_offset,
                        "# type=BITMAP_BUDDY_ALLOCATOR\n");
            config.log_offset += snprintf((char *)config.log_data + config.log_offset,
                                        config.max_log_size - config.log_offset,
                                        "# total_size=%zu,max_levels=%zu\n",
                                        params.buddy.total_size, params.buddy.max_levels);
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
            BuddyAllocator* buddy = (BuddyAllocator *) config.allocator;
            n_pointers = 1 << (buddy->num_levels - 1);
            break;
        case BITMAP_BUDDY_ALLOCATOR:
            BitmapBuddyAllocator* bitmap = (BitmapBuddyAllocator *) config.allocator;
            n_pointers = 1 << (bitmap->num_levels - 1);
            break;
        default:
            fprintf(stderr, "Unknown allocator type: %d\n", type);
            result = -1;
            goto cleanup;
    }

    // Use the function to get remaining characters
    long offset = ftell(file);
    remaining = count_remaining_characters(file);
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
    struct timespec start, end;
    struct rusage usage_start, usage_end;
    double elapsed_seconds = 0.0;
    double user_seconds = 0.0, sys_seconds = 0.0;

    if (clock_gettime(CLOCK_MONOTONIC, &start) != 0 || getrusage(RUSAGE_SELF, &usage_start) != 0) {
        perror("Failed to get start time");
        result = -1;
    } else {
        // Execute benchmark
        result = Allocator_malloc_free(&config, instructions, remaining, n_pointers);

        // End timing
        if (clock_gettime(CLOCK_MONOTONIC, &end) != 0 || getrusage(RUSAGE_SELF, &usage_end) != 0) {
            perror("Failed to get end time");
            result = -1;
        } else {
            // Calculate elapsed wall time in seconds
            elapsed_seconds = (end.tv_sec - start.tv_sec);
            elapsed_seconds += (end.tv_nsec - start.tv_nsec) / 1e9;

            // Calculate user and system CPU time in seconds
            user_seconds = (usage_end.ru_utime.tv_sec - usage_start.ru_utime.tv_sec)
                         + (usage_end.ru_utime.tv_usec - usage_start.ru_utime.tv_usec) / 1e6;
            sys_seconds = (usage_end.ru_stime.tv_sec - usage_start.ru_stime.tv_sec)
                        + (usage_end.ru_stime.tv_usec - usage_start.ru_stime.tv_usec) / 1e6;

        }
    }
    #else
    printf("N_pointers: %d\n", n_pointers);
    result = Allocator_malloc_free(&config, instructions, remaining, n_pointers);
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
    if (config.allocator && config.allocator->dest) {
        config.allocator->dest(config.allocator);
    }
    if (map_base != NULL && map_base != MAP_FAILED) {
        munmap(map_base, remaining + delta);
    }
    // Truncate log file to actual log_offset size BEFORE munmap and fclose
    if (log_map != NULL && log_map != MAP_FAILED) {
        printf("Log offset: %zu bytes\n", config.log_offset);
        ftruncate(fileno(log_fp), config.log_offset);

        printf("Do you want to save .log for graph generation? [y/N] ");
        fflush(stdout);

        char first_char = 0;
        if (scanf(" %c", &first_char) == 1 && tolower(first_char) == 'y') {
            char cmd[512];
            snprintf(cmd, sizeof(cmd), "mkdir -p ./thesis/graphs/benchmarks && cp '%s' ./thesis/graphs/benchmarks/", log_path);
            int ret = system(cmd);
            if (ret != 0) {
                fprintf(stderr, "Failed to copy log file to ./thesis/graphs/benchmarks\n");
            } else {
                printf("Log file copied to ./thesis/graphs/benchmarks\n");
            }
        }

        munmap(log_map, max_log_size);
    }
    if (log_fp) fclose(log_fp);
    if (file) fclose(file);
    return result;
}