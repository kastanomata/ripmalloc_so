#include <main.h>

int main() {
    test_slab_allocator();
    test_buddy_allocator();

    printf("=== FREEFORM ===\n");


    printf("=== END FREEFORM ===\n");
    return 0;
}