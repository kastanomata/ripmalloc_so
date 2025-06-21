#include <benchmark.h>

int Allocator_benchmark(struct AllocatorConfig config, void *instructions, long remaining) {
	Allocator *allocator = config.allocator;
	enum AllocatorType type = config.type;
	if (!allocator || !instructions || remaining <= 0) {
		fprintf(stderr, "Invalid parameters for Allocator benchmark\n");
		return -1;
	}

	int n_pointers = 0;
	switch(config.type) {
		case SLAB_ALLOCATOR:
			n_pointers = ((SlabAllocator *)allocator)->free_list_size_max;
			break;
		case BUDDY_ALLOCATOR:
			n_pointers = 1 << (((BuddyAllocator *)allocator)->num_levels - 1);
			printf("Buddy Allocator has %d pointers\n", n_pointers);
			break;
		case BITMAP_BUDDY_ALLOCATOR:
			n_pointers = ((BitmapBuddyAllocator *)allocator)->bitmap.num_bits;
			printf("Bitmap Buddy Allocator has %d pointers\n", n_pointers);
			break;
		default:
			fprintf(stderr, "Unknown allocator type: %d\n", type);
			return -1;
	}

	char *pointers[n_pointers]; // Large enough for most cases; can be adjusted as needed
	long pointer_count = 0;
	// Initialize pointers to NULL
	// memset(pointers, 0, n_pointers);
	for (int i = 0; i < n_pointers; ++i) {
		pointers[i] = NULL;
	}

	unsigned char *instr = (unsigned char *)instructions;
	long start = 0;
	char instruction_str[256];
	for (long i = 0; i < remaining; ++i) {
		if (instr[i] == '\n') {
			long len = i - start;
			if (len > 0 && len < (long)sizeof(instruction_str)) {
				memcpy(instruction_str, &instr[start], len);
				instruction_str[len] = '\0';
				if (instruction_str[0] != '%') {
					printf("Instruction: %s -- ", instruction_str);
					parse_allocator_request(instruction_str, &config, pointers, &pointer_count);
				}
			}
			start = i + 1;
		}
	}
	if (start < remaining) {
		long len = remaining - start;
		if (len > 0 && len < (long)sizeof(instruction_str)) {
			memcpy(instruction_str, &instr[start], len);
			instruction_str[len] = '\0';
			if (instruction_str[0] != '%') {
				printf("Instruction: %s -- ", instruction_str);
				parse_allocator_request(instruction_str, &config, pointers, &pointer_count);
			}
		}
	}

	// check if all pointers are freed
	for (int i = 0; i < n_pointers; ++i) {
		if (pointers[i] != NULL) {
			printf(RED "Pointer at index %d was not freed: %p\n" RESET, i, pointers[i]);
			return -1; // Not all pointers were freed
		}
	}

	return 0;
}


int run_benchmark_from_file(const char *file_name) {
	int result = 0;
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
  struct AllocatorConfig config;
  config.type = type;
  config.allocator = NULL;
  if( type > VARIABLE_ALLOCATION_DELIMITER ) {
    config.is_variable_size_allocation = true;
  } else {
    config.is_variable_size_allocation = false;
  }

  union AllocatorConfigData params = parse_allocator_create_parameters(file, &config);
	// struct AllocatorConfig config;
	void *map_base = NULL;
	long delta = 0;
	long remaining = 0;
	union GeneralAllocator allocator;
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
	}

	long offset = ftell(file); // Get current file position (after parsing headers)
	fseek(file, 0, SEEK_END);
	long file_size = ftell(file);
	remaining = file_size - offset;
	if (remaining < 0) {
		result = -1;
		goto end;
	}
	long page_size = sysconf(_SC_PAGE_SIZE);
	long page_offset = offset & ~(page_size - 1);
	delta = offset - page_offset;
	map_base = mmap(NULL, remaining + delta, PROT_READ, MAP_PRIVATE, fileno(file), page_offset);
	if (map_base == MAP_FAILED) {
		perror("mmap failed");
		result = -1;
		goto end;
	}
	void *instructions = (unsigned char *)map_base + delta;
	result = Allocator_benchmark(config, instructions, remaining);
	if (result < 0) {
		fprintf(stderr, "SlabAllocator benchmark failed\n");
	}
	switch (type) {
		case SLAB_ALLOCATOR:
			SlabAllocator_print_state((SlabAllocator *)config.allocator);
			SlabAllocator_destroy((SlabAllocator *)config.allocator);
			break;
		case BUDDY_ALLOCATOR:
			BuddyAllocator_print_state((BuddyAllocator *)config.allocator);
			BuddyAllocator_destroy((BuddyAllocator *)config.allocator);
			break;
		case BITMAP_BUDDY_ALLOCATOR:
			BitmapBuddyAllocator_print_state((BitmapBuddyAllocator *)config.allocator);
			Bitmap b = ((BitmapBuddyAllocator *)config.allocator)->bitmap;
			// show bitmap state
			printf("Bitmap state:\n");
			for (int i = 0; i < b.num_bits; i++) {
				if (i % 32 == 0) {
					printf("\n");
				}
				printf("%d ", bitmap_test(&b, i));
			}
			printf("\n");
			BitmapBuddyAllocator_destroy((BitmapBuddyAllocator *)config.allocator);
			break;
		default:
			fprintf(stderr, "Unknown allocator type: %d\n", type);
	}
end:
	if (map_base != NULL && map_base != MAP_FAILED) {
		munmap(map_base, remaining + delta);
	}
	fclose(file);
	return result;
}