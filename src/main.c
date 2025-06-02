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
    // freeform();
    BitmapBuddyAllocator buddy;
    BitmapBuddyAllocator_create(&buddy, PAGESIZE, DEF_LEVELS_NUMBER);

    // Allocate
    void* ptr = buddy.base.malloc(&buddy.base, PAGESIZE / 8);
    if (!ptr) {
        printf("Allocation failed\n");
        return -1;
    }
    // BitmapBuddyAllocator_print_state(&buddy);
    line
    printf("Allocated %zu bytes at %p\n", PAGESIZE / 8, ptr);
    fill_memory_pattern(ptr, PAGESIZE / 8, 0xAA);
    assert(!verify_memory_pattern(ptr, PAGESIZE / 8, 0xAA));
    printf("Memory pattern filled and verified\n");
    print_memory_pattern(ptr, PAGESIZE / 8);
    // Print state

    // Free
    buddy.base.free(&buddy.base, ptr);

    buddy.base.dest(&buddy.base);
}