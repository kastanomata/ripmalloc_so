#include <main.h>

#define line printf("- - - - - - - - - - - - - - - - - - - - - - - -\n");


int main(int argc, char* argv[]) {
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
  benchmark();
  if(argc>1) {
    printf("Program arguments (%d):\n", argc);
    for (int i = 0; i < argc; ++i) {
      if (strcmp(argv[i], "freeform") == 0) {
        freeform();
      }
      if (strcmp(argv[i], "benchmark") == 0) {
        benchmark();
      }
    }
  }
}