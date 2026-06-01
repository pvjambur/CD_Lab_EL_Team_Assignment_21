// shadow_memory.cpp - Shadow Memory Management
//
// Manages shadow values stored in memory for heap-allocated
// floating-point data.

#include "shadow_memory.h"
#include <cstdlib>
#include <cstring>
#include <map>

static std::map<void *, void *> shadow_memory_map;

extern "C" {

void *__nsan_allocate_shadow(void *original_addr, size_t size) {
  void *shadow = malloc(size * 2); // 2x for higher precision
  shadow_memory_map[original_addr] = shadow;
  return shadow;
}

void __nsan_deallocate_shadow(void *original_addr) {
  auto it = shadow_memory_map.find(original_addr);
  if (it != shadow_memory_map.end()) {
    free(it->second);
    shadow_memory_map.erase(it);
  }
}

void *__nsan_get_shadow_address(void *original_addr, size_t original_size) {
  auto it = shadow_memory_map.find(original_addr);
  if (it != shadow_memory_map.end()) {
    return it->second;
  }
  return nullptr;
}

} // extern "C"
