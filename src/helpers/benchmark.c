#include <benchmark.h>

static int get_benchmark_files(char *files[], int max_files) {
	// This function should populate the files array with benchmark file names
	// go into the benchmark folder and get all files that end with .alloc
	printf("Getting benchmark files from %s\n", BENCHMARK_FOLDER);
	DIR *dir = opendir(BENCHMARK_FOLDER);
	if (!dir) {
		perror("Failed to open benchmark directory");
		return -1;
	}
	struct dirent *entry;
	int count = 0;
	while ((entry = readdir(dir)) != NULL) {
		if (count >= max_files) {
			break;
		}
		// Check if the file ends with .alloc
		if (strstr(entry->d_name, ".alloc") != NULL) {
			files[count] = malloc(strlen(entry->d_name) + 1);
			if (!files[count]) {
				perror("Failed to allocate memory for file name");
				closedir(dir);
				return -1;
			}
			strcpy(files[count], entry->d_name);
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
	// This function should run the benchmark file
	// For now, we will just print the file name
	printf("Parsing benchmark: %s\n", file_name);
	
	
	// Here you would implement the actual benchmark logic
	// For example, you could call a function that executes the benchmark
	// and returns the results.
	
	return 0;  // Return 0 on success
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

