#include <main.h>

int main() {
    test_slab_allocator();

    printf("\n\n\n");
    test_buddy_allocator();
    printf("\n\n\n");
    printf("=== FREEFORM ===\n");


    printf("=== END FREEFORM ===\n");
    return 0;
}