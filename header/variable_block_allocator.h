#pragma once
#include <allocator.h>

typedef struct VariableBlockAllocator VariableBlockAllocator;

struct VariableBlockAllocator {
  Allocator base;
  size_t internal_fragmentation;
  size_t external_fragmentation;
};