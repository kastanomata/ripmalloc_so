#include <main.h>

int main() {

    
    int result = test_slab_allocator();
    
    if (result == 0) {
        printf("All tests passed!\n");
    } else {
        printf("Some tests failed!\n");
    }
    
    return result;
}