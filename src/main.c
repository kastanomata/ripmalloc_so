#include <main.h>

#define line printf("- - - - - - - - - - - - - - - - - - - - - - - -\n");

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
  benchmark();
}