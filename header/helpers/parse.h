#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <allocator.h>
#include <stdbool.h>

#define VARIABLE_ALLOCATION_DELIMITER 0

enum AllocatorType {
  SLAB_ALLOCATOR,
  BUDDY_ALLOCATOR,
  BITMAP_BUDDY_ALLOCATOR,
};

enum RequestType {
  ALLOCATE,
  FREE
};

struct AllocatorBenchmarkConfig {
  Allocator *allocator;
  enum AllocatorType type;
  bool is_variable_size_allocation;
  const char *log_data; // Pointer to log data that is mmaped to a file
  size_t max_log_size;
  size_t log_offset;
};

union AllocatorParameterData {
  struct {
    size_t slab_size;
    size_t n_slabs;
  } slab;
  struct {
    size_t total_size;
    size_t max_levels;
  } buddy;
};

struct RequestResult {
  bool success;
  size_t fragmentation;
};



int parse(const char *filename);
enum AllocatorType parse_allocator_create(FILE *file);
union AllocatorParameterData parse_allocator_create_parameters(FILE *file, struct AllocatorBenchmarkConfig *config);
int parse_allocator_request(const char *line, struct AllocatorBenchmarkConfig *config, char **pointers, int num_pointers, long *allocation_counter);

