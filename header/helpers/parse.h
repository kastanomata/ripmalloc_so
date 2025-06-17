#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <allocator.h>

enum AllocatorType {
  SLAB_ALLOCATOR,
  BUDDY_ALLOCATOR,
  BITMAP_BUDDY_ALLOCATOR,
};

struct AllocatorConfig {
  enum AllocatorType type;
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

