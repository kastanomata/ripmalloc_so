#include <benchmark.h>

static int get_benchmark_files(char *files[], int max_files) {
    DIR *dir = opendir(BENCHMARK_FOLDER);
    if (!dir) {
        perror("Failed to open benchmark directory");
        return -1;
    }
    struct dirent *entry;
    int count = 0;
    while ((entry = readdir(dir)) != NULL && count < max_files) {
        const char *ext = strrchr(entry->d_name, '.');
        if (ext && strcmp(ext, ".alloc") == 0) {
            files[count] = strdup(entry->d_name);
            if (!files[count]) {
                perror("Failed to allocate memory for file name");
                for (int i = 0; i < count; i++) free(files[i]);
                closedir(dir);
                return -1;
            }
            count++;
        }
    }
    closedir(dir);
    return count;
}

static int free_benchmark_files(char *files[], int file_count) {
	// Free the allocated memory for file names
	for (int i = 0; i < file_count; i++) {
		free(files[i]);
	}
	return 0;
}

static int select_benchmark_file(char *files[], int file_count) {
	return file_count;  // Return 0 to indicate only first file should be run
	
	// print all the files in a 1-indexed list and prompt the user to select one
	// if the user enters 0, run them all
	printf("Select a benchmark file (0 to run all):\n");
	for (int i = 0; i < file_count; i++) {
		printf("%d: %s\n", i + 1, files[i]);
	}
	int choice;
	scanf("%d", &choice);
	
	if (choice == 0) {
		return file_count;  // Indicate that all files should be run
	} else if (choice > 0 && choice <= file_count) {
		return choice - 1;  // Return the index of the selected file
	} else {
		printf("Invalid choice.\n");
		return -1;  // Indicate an invalid choice
	}
}

int run_benchmark(const char *file_name) {
	printf("Parsing benchmark: %s\n", file_name);
	// Open the benchmark file in the BENCHMARK_FOLDER
	char full_path[256];
	snprintf(full_path, sizeof(full_path), "%s/%s", BENCHMARK_FOLDER, file_name);
	FILE *file = fopen(full_path, "r");
	if (!file) {
		perror("Failed to open benchmark file");
		return -1;  // Error opening file
	}
	enum AllocatorType type = parse_allocator_create(file);
	printf("Parsed allocator type: %d\n", type);
	if (type < 0) {
		fclose(file);
		return -1;  // Error parsing allocator type
	}
	struct AllocatorConfig config;
	union AllocatorConfigData config_data;
	switch (type) {
		case SLAB_ALLOCATOR:
			printf("Running SLAB_ALLOCATOR benchmark...\n");
			SlabAllocator slab;
			
			config.type = SLAB_ALLOCATOR;
			config.allocator = (Allocator*) &slab;  

			config_data = parse_allocator_create_parameters(file, &config);
			if (!config_data.slab.slab_size || !config_data.slab.n_slabs) {
				fprintf(stderr, "Invalid slab parameters\n");
				fclose(file);
				return -1;  // Error parsing parameters
			}
			if (!SlabAllocator_create(&slab, config_data.slab.slab_size, config_data.slab.n_slabs)) {
				fprintf(stderr, "Failed to create SlabAllocator\n");
				fclose(file);
				return -1;  // Error creating allocator
			}

			// print information about the allocator
			SlabAllocator_print_state(&slab);
			break;
		case BUDDY_ALLOCATOR:
			printf("Running BUDDY_ALLOCATOR benchmark...\n");
			BuddyAllocator buddy;
			
			config.type = BUDDY_ALLOCATOR;
			config.allocator = (Allocator*) &buddy;

			config_data = parse_allocator_create_parameters(file, &config);
			if (!config_data.buddy.total_size || !config_data.buddy.max_levels) {
				fprintf(stderr, "Invalid buddy parameters\n");
				fclose(file);
				return -1;  // Error parsing parameters
			}
			if (!BuddyAllocator_create(&buddy, config_data.buddy.total_size, config_data.buddy.max_levels)) {
				fprintf(stderr, "Failed to create BuddyAllocator\n");
				fclose(file);
				return -1;  // Error creating allocator
			}
			// print information about the allocator
			BuddyAllocator_print_state(&buddy);
			break;
		case BITMAP_BUDDY_ALLOCATOR:
			printf("Running BITMAP_BUDDY_ALLOCATOR benchmark...\n");
			BitmapBuddyAllocator bitmap;
			
			config.type = BITMAP_BUDDY_ALLOCATOR;
			config.allocator = (Allocator*) &bitmap;

			config_data = parse_allocator_create_parameters(file, &config);
			if (!config_data.buddy.total_size || !config_data.buddy.max_levels) {
				fprintf(stderr, "Invalid bitmap buddy parameters\n");
				fclose(file);
				return -1;  // Error parsing parameters
			}
			if (!BitmapBuddyAllocator_create(&bitmap, config_data.buddy.total_size, config_data.buddy.max_levels)) {
				fprintf(stderr, "Failed to create BitmapBuddyAllocator\n");
				fclose(file);
				return -1;  // Error creating allocator
			}
			// print information about the allocator
			BitmapBuddyAllocator_print_state(&bitmap);
			break;
		default:
			fprintf(stderr, "Unknown allocator type: %d\n", type);
			fclose(file);
			return -1;  // Unknown allocator type
	}
	return 0;
}

int benchmark() {
	// this function should print all the benchmark files in the benchmark folder
	char *files[100];  // Adjust size as needed
	int file_count = get_benchmark_files(files, 100);
	if (file_count <= 0) {
		return -1;  // Error occurred
	}
	
	int run = select_benchmark_file(files, file_count);
	if(run == file_count) {
		// Run all files
		for (int i = 0; i < file_count; i++) {
			printf("Running benchmark: %s\n", files[i]);
			run_benchmark(files[i]);  
		}
	} else if (run >= 0) {
		// Run the selected file
		printf("Running benchmark: %s\n", files[run]);
		run_benchmark(files[run]);  
	} else {
		printf("No valid benchmark selected.\n");
		free_benchmark_files(files, file_count);
		return -1;  // Invalid selection
	}
	
	
	// Free the allocated memory for file names
	free_benchmark_files(files, file_count);
	return 0;  // Success
}

