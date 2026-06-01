#include <cstdio>
#include <cassert>
#include <cstdlib>

// Forward declarations from shadow memory runtime
extern "C" {
void *__nsan_allocate_shadow(void *original_addr, size_t size);
void __nsan_deallocate_shadow(void *original_addr);
void *__nsan_get_shadow_address(void *original_addr, size_t original_size);
}

void test_shadow_allocation() {
    float dummy_val = 42.0f;
    size_t data_size = sizeof(float);

    // 1. Allocate shadow memory
    void *shadow = __nsan_allocate_shadow(&dummy_val, data_size);
    assert(shadow != nullptr && "Failed to allocate shadow address.");

    // 2. Retrieve shadow memory
    void *retrieved = __nsan_get_shadow_address(&dummy_val, data_size);
    assert(retrieved == shadow && "Shadow address lookup mismatch.");

    // 3. Deallocate and test cleanup
    __nsan_deallocate_shadow(&dummy_val);
    void *after_free = __nsan_get_shadow_address(&dummy_val, data_size);
    assert(after_free == nullptr && "Shadow memory freed but still present in map.");

    printf("[PASS] Unit test: shadow memory integrity verified.\n");
}

int main() {
    printf("Running unit test for Shadow Map...\n");
    test_shadow_allocation();
    return 0;
}