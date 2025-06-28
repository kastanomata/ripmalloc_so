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
    
    printf("\nSelect a benchmark file:\n");
    printf("0: Run all benchmarks\n");
    for (int i = 0; i < file_count; i++) {
        printf("%d: %s\n", i + 1, files[i]);
    }
    printf("q: Quit\n");
    
    char input[32];
    while (1) {
        printf("\nEnter choice [0-%d, q]: ", file_count);
        if (scanf("%31s", input) != 1) {
            printf("Invalid input. Please try again.\n");
            while (getchar() != '\n'); // Clear input buffer
            continue;
        }
        
        // Check for quit command
        if (input[0] == 'q' || input[0] == 'Q') {
            return -2; // Special code for quit
        }
        
        // Convert to number
        char *endptr;
        long choice = strtol(input, &endptr, 10);
        if (endptr == input || *endptr != '\0') {
            printf("Invalid input. Please enter a number or 'q'.\n");
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

int benchmark() {
    char *files[100];  // Adjust size as needed
    int file_count = get_benchmark_files(files, 100);
    if (file_count <= 0) {
        return -1;  // Error occurred
    }
    
    while (1) {
        int run = select_benchmark_file(files, file_count);
        
        if (run == -2) { // Quit command
            printf("Exiting benchmark menu.\n");
            break;
        } else if (run == file_count) {
            // Run all files
            printf("\nRunning all benchmarks:\n");
            for (int i = 0; i < file_count; i++) {
                printf("\n=== Benchmark %d/%d: %s ===\n", i+1, file_count, files[i]);
                Allocator_benchmark_initialize(files[i]);  
            }
            printf("\nAll benchmarks completed.\n");
        } else if (run >= 0) {
            // Run the selected file
            printf("\n=== Running benchmark: %s ===\n", files[run]);
            Allocator_benchmark_initialize(files[run]);  
            printf("\nBenchmark completed. ");
        } else {
            printf(RED "No valid benchmark selected. " RESET);
        }
        
        // Option to continue or quit
        printf("Press any key to continue, or 'q' to quit...");
        // Clear input buffer before waiting for key press
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF); // Discard leftover input
        ch = getchar(); // Wait for actual key press
        if (ch == 'q' || ch == 'Q') {
            printf("Exiting benchmark menu.\n");
            break;
        }
        // Clear input buffer after key press
        while (ch != '\n' && ch != EOF) ch = getchar();
    }
    
    // Free the allocated memory for file names
    free_benchmark_files(files, file_count);
    return 0;  // Success
}

