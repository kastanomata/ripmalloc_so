#include <main.h>

#define line printf("- - - - - - - - - - - - - - - - - - - - - - - -\n");


int main(int argc, char* argv[]) {
  // printf("Program arguments (%d):\n", argc);
  // for (int i = 0; i < argc; ++i) {
  //   printf("  argv[%d]: %s\n", i, argv[i]);
  // }
  line
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