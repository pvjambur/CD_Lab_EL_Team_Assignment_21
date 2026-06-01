// shadow_memory.h - Shadow Memory Management Header

#ifndef SHADOW_MEMORY_H
#define SHADOW_MEMORY_H

#include <cstddef>

extern "C" {

void *__nsan_allocate_shadow(void *original_addr, size_t size);
void __nsan_deallocate_shadow(void *original_addr);
void *__nsan_get_shadow_address(void *original_addr, size_t original_size);

} // extern "C"

#endif // SHADOW_MEMORY_H
