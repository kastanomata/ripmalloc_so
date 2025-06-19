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
	if (file_count == 0) {
		printf("No benchmark files found.\n");
		return -1;
	}
	
	printf("Select a benchmark file (0 to run all):\n");
	for (int i = 0; i < file_count; i++) {
		printf("%d: %s\n", i + 1, files[i]);
	}
	
	int choice;
	while (1) {
		printf("Enter choice [0-%d]: ", file_count);
		if (scanf("%d", &choice) != 1) {
			printf("Invalid input. Please enter a number.\n");
			while (getchar() != '\n'); // Clear input buffer
			continue;
		}
		
		if (choice == 0) {
			return file_count; // Run all
		} else if (choice > 0 && choice <= file_count) {
			return choice - 1; // Run single
		} else {
			printf("Invalid choice. Try again.\n");
		}
	}
}

// Main benchmark function 
int run_benchmark(const char *file_name) {
	printf("Parsing benchmark: %s\n", file_name);
	
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
	printf("Parsed allocator type: %d\n", type);
	if (type < 0) {
		fclose(file);
		return -1;
	}
	// struct AllocatorConfig config;
	int result = -1;
	switch (type) {
		case SLAB_ALLOCATOR:
		printf("Running SLAB_ALLOCATOR benchmark...\n");
		// result = setup_slab_allocator(file, &config);
		break;
		case BUDDY_ALLOCATOR:
		printf("Running BUDDY_ALLOCATOR benchmark...\n");
		// result = setup_buddy_allocator(file, &config);
		break;
		case BITMAP_BUDDY_ALLOCATOR:
		printf("Running BITMAP_BUDDY_ALLOCATOR benchmark...\n");
		// result = setup_bitmap_buddy_allocator(file, &config);
		break;
		default:
		fprintf(stderr, "Unknown allocator type: %d\n", type);
	}
	
	fclose(file);
	return result;
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

