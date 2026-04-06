# NSan Runtime Library
add_library(nsan_runtime STATIC
  nsan_runtime.cpp
  shadow_memory.cpp
)

target_include_directories(nsan_runtime PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/include/internal
)
