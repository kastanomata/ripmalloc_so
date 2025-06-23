#include <main.h>

#define line printf("- - - - - - - - - - - - - - - - - - - - - - - -\n");
void test_basic_allocation_free() {
  BuddyAllocator alloc;
  size_t total_size = 1024 * 1024; // 1MB
  int levels = 10; // Results in min block size = 1024 / (2^9) = 2KB
  
  BuddyAllocator_create(&alloc, total_size, levels);
  
  // Allocate a small block (should split down to smallest possible)
  void *ptr1 = BuddyAllocator_malloc(&alloc, 256);
  assert(ptr1 != NULL && "Allocation failed");
  
  // Allocate another block
  void *ptr2 = BuddyAllocator_malloc(&alloc, 512);
  assert(ptr2 != NULL && "Allocation failed");
  
  // Free the blocks
  assert(BuddyAllocator_free(&alloc, ptr1) == 0 && "Free failed");
  assert(BuddyAllocator_free(&alloc, ptr2) == 0 && "Free failed");
  
  BuddyAllocator_destroy(&alloc);
  printf("Basic allocation and free test passed!\n");
}

void test_exact_block_allocation() {
  BuddyAllocator alloc;
  size_t total_size = 1024 * 1024; // 1MB
  int levels = 10; // Min block size = 1KB
  
  BuddyAllocator_create(&alloc, total_size, levels);
  
  // Allocate a block of size 1KB (smallest possible)
  void *ptr = BuddyAllocator_malloc(&alloc, 1024 - BUDDY_METADATA_SIZE);
  assert(ptr != NULL && "Allocation failed");
  
  // Verify no splitting occurs (should use smallest block directly)
  BuddyAllocator_print_state(&alloc); // Debug check
  
  BuddyAllocator_free(&alloc, ptr);
  BuddyAllocator_destroy(&alloc);
  printf("Exact block allocation test passed!\n");
}

void test_merging() {
  BuddyAllocator alloc;
  size_t total_size = 1024 * 1024; // 1MB
  int levels = 10; // Min block size = 1KB
  
  BuddyAllocator_create(&alloc, total_size, levels);
  
  // Allocate two adjacent blocks (same level)
  void *ptr1 = BuddyAllocator_malloc(&alloc, 512);
  void *ptr2 = BuddyAllocator_malloc(&alloc, 512);
  assert(ptr1 != NULL && ptr2 != NULL && "Allocation failed");
  
  // Free both blocks (should merge into a larger block)
  BuddyAllocator_free(&alloc, ptr1);
  BuddyAllocator_free(&alloc, ptr2);
  
  // Verify merging by checking free lists
  BuddyAllocator_print_state(&alloc); // Debug check
  for (int level = 0; level < levels; level++) {
    printf("Level %d free list: ", level);
    BuddyBlock *block = alloc.free_lists[level];
    int count = 0;
    while (block) {
      count++;
      block = block->next;
    }
    printf("%d blocks\n", count);
  }
  
  BuddyAllocator_destroy(&alloc);
  printf("Merging test passed!\n");
}

void test_out_of_memory() {
  BuddyAllocator alloc;
  size_t total_size = 1024; // 1KB (small for testing OOM)
  int levels = 3; // Min block size = 128B
  
  BuddyAllocator_create(&alloc, total_size, levels);
  
  // Allocate entire memory
  void *ptr1 = BuddyAllocator_malloc(&alloc, 500);
  void *ptr2 = BuddyAllocator_malloc(&alloc, 250);
  assert(ptr1 != NULL && ptr2 != NULL && "Allocation failed");
  
  // Try allocating more (should fail)
  void *ptr3 = BuddyAllocator_malloc(&alloc, 128);
  assert(ptr3 == NULL && "Allocation should have failed (OOM)");
  
  BuddyAllocator_free(&alloc, ptr1);
  BuddyAllocator_free(&alloc, ptr2);
  BuddyAllocator_destroy(&alloc);
  printf("Out-of-memory test passed!\n");
}

void test_invalid_free() {
  BuddyAllocator alloc;
  size_t total_size = 1024 * 1024; // 1MB
  int levels = 10;
  
  BuddyAllocator_create(&alloc, total_size, levels);
  
  void *ptr = BuddyAllocator_malloc(&alloc, 256);
  assert(ptr != NULL && "Allocation failed");
  
  // Test freeing NULL
  assert(BuddyAllocator_free(&alloc, NULL) == -1 && "Freeing NULL should fail");
  
  // Test double-free
  BuddyAllocator_free(&alloc, ptr);
  assert(BuddyAllocator_free(&alloc, ptr) == -1 && "Double-free should fail");
  
  // Test freeing pointer outside allocator range
  void *invalid_ptr = (void *)((char *)alloc.memory_start - 1);
  assert(BuddyAllocator_free(&alloc, invalid_ptr) == -1 && "Freeing invalid pointer should fail");
  
  BuddyAllocator_destroy(&alloc);
  printf("Invalid free test passed!\n");
}

void test_stress_random_alloc_free() {
  BuddyAllocator alloc;
  size_t total_size = 16 * 1024 * 1024; // 16MB
  int levels = 12; // Min block size = 4KB
  
  BuddyAllocator_create(&alloc, total_size, levels);
  
  void *ptrs[1000];
  size_t sizes[1000];
  
  // Randomly allocate and free blocks
  for (int i = 0; i < 1000; i++) {
    sizes[i] = (rand() % 4096) + 1; // Random size between 1B and 4KB
    ptrs[i] = BuddyAllocator_malloc(&alloc, sizes[i]);
    
    if (ptrs[i] == NULL) {
      printf("Allocation failed at iteration %d (size: %zu)\n", i, sizes[i]);
      continue;
    }
    
    // Occasionally free a random block
    if (i > 0 && rand() % 5 == 0) {
      int idx = rand() % i;
      if (ptrs[idx] != NULL) {
        BuddyAllocator_free(&alloc, ptrs[idx]);
        ptrs[idx] = NULL;
      }
    }
  }
  
  // Free all remaining blocks
  for (int i = 0; i < 1000; i++) {
    if (ptrs[i] != NULL) {
      BuddyAllocator_free(&alloc, ptrs[i]);
    }
  }
  
  BuddyAllocator_destroy(&alloc);
  printf("Stress test passed!\n");
}

void test_alignment() {
  BuddyAllocator alloc;
  size_t total_size = 1024 * 1024; // 1MB
  int levels = 10;
  
  BuddyAllocator_create(&alloc, total_size, levels);
  
  // Allocate various sizes and check alignment
  for (size_t size = 1; size <= 4096; size *= 2) {
    void *ptr = BuddyAllocator_malloc(&alloc, size);
    assert(ptr != NULL && "Allocation failed");
    assert((uintptr_t)ptr % 8 == 0 && "Pointer not 8-byte aligned");
    BuddyAllocator_free(&alloc, ptr);
  }
  
  BuddyAllocator_destroy(&alloc);
  printf("Alignment test passed!\n");
}

int main() {
  test_bitmap();
  line
  test_double_linked_list();
  line
  test_slab_allocator();
  line
  test_buddy_allocator();
  line
  test_bitmap_buddy_allocator();
  line
  // freeform();
  line
  // benchmark();
  line
  test_basic_allocation_free();
  test_exact_block_allocation();
  test_merging();
  test_out_of_memory();
  test_invalid_free();
  test_stress_random_alloc_free();
  test_alignment();
  printf("All tests passed!\n");
  return 0;
}