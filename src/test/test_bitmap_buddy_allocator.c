#include <test/test_bitmap_buddy_allocator.h>

int test_bitmap_buddy_allocator() {
  BitmapBuddyAllocator buddy;
  BitmapBuddyAllocator_create(&buddy, PAGESIZE, DEF_LEVELS_NUMBER);

  // Allocate
  void* ptr = buddy.base.malloc(&buddy.base, PAGESIZE / 8);
  if (!ptr) {
    printf("First allocation failed\n");
    buddy.base.dest(&buddy.base);
    return -1;
  }
  // Allocate two buddies
  void* ptr2 = buddy.base.malloc(&buddy.base, PAGESIZE / 8);
  if (!ptr2) {
    printf("Second allocation failed\n");
    buddy.base.free(&buddy.base, ptr);
    buddy.base.dest(&buddy.base);
    return -1;
  }

  line
  printf("Allocated first buddy: %zu bytes at %p\n", PAGESIZE / 8, ptr);
  printf("Allocated second buddy: %zu bytes at %p\n", PAGESIZE / 8, ptr2);

  // Print memory patterns separately
  fill_memory_pattern(ptr, PAGESIZE / 8, 0xAA);
  fill_memory_pattern(ptr2, PAGESIZE / 8, 0xBB);

  printf("First buddy memory pattern:\n");
  print_memory_pattern(ptr, PAGESIZE / 8);

  printf("Second buddy memory pattern:\n");
  print_memory_pattern(ptr2, PAGESIZE / 8);

  line
  printf("Both buddies together in memory:\n");
  print_memory_pattern(ptr2, PAGESIZE / 4); // print both regions together

  // Free second buddy
  buddy.base.free(&buddy.base, ptr2);

  // Free
  buddy.base.free(&buddy.base, ptr);  

  int max_number_buddies = (buddy.memory_size / (PAGESIZE / 8));
  void* buddies[max_number_buddies];
  size_t alloc_size = PAGESIZE / 8 - sizeof(void*);
  printf("trying to allocate %d buddies of size %zu bytes each\n", 
          max_number_buddies, alloc_size);    

  // Allocate all buddies
  for (int i = 0; i < max_number_buddies; ++i) {
    buddies[i] = buddy.base.malloc(&buddy.base, alloc_size);
    if (!buddies[i]) {
      printf("Allocation failed at buddy %d\n", i);
      // Free any previously allocated buddies
      for (int j = 0; j < i; ++j) {
          buddy.base.free(&buddy.base, buddies[j]);
      }
      break;
    }
    fill_memory_pattern(buddies[i], alloc_size, 0xAA + i);
  }

  line
  print_memory_pattern(buddy.memory, buddy.memory_size);
  // Deallocate all buddies
  for (int i = 0; i < max_number_buddies; ++i) {
    if (buddies[i]) {
      buddy.base.free(&buddy.base, buddies[i]);
    }
  }


  // Optionally, print state
  // BitmapBuddyAllocator_print_state(&buddy);

  buddy.base.dest(&buddy.base);
  return 0;
}

