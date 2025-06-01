#include <main.h>

int main() {
    test_slab_allocator();

    printf("\n\n\n");
    test_buddy_allocator();
    printf("\n\n\n");
    printf("=== START FREEFORM ===\n");
    // Freeform testing area
    
    test_bitmap();

    test_double_linked_list();


    printf("=== END FREEFORM ===\n");
    return 0;
}