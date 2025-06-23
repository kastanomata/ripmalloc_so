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

struct AllocatorConfig {
  enum AllocatorType type;
  bool is_variable_size_allocation;
  Allocator *allocator;
};

union AllocatorConfigData {
  struct {
    size_t slab_size;
    size_t n_slabs;
  } slab;
  struct {
    size_t total_size;
    size_t max_levels;
  } buddy;
};



int parse(const char *filename);
enum AllocatorType parse_allocator_create(FILE *file);
union AllocatorConfigData parse_allocator_create_parameters(FILE *file, struct AllocatorConfig *config);
int parse_allocator_request(const char *line, struct AllocatorConfig *config, char **pointers, int num_pointers, long *allocation_counter);

