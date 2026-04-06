# NSan Runtime Library
# Builds libnsan_runtime.a: linked into every instrumented binary.
# This library's performance directly determines NSan's overhead.

add_library(nsan_runtime STATIC
  nsan_runtime.cpp
  shadow_memory.cpp
)

target_include_directories(nsan_runtime PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/include/internal
)

# Shadow computation optimisation.
# -ffast-math flags are SAFE here because shadows are only used for
# comparison — they are never fed back into user-visible computations.
#   -fno-signed-zeros    : allows (x+0)==x optimisations in shadow math
#   -fno-honor-nans      : skip NaN propagation in shadow (detected separately)
#   -fmerge-all-constants: deduplicate shadow threshold constants
if(ENABLE_SHADOW_OPTIMIZATION)
  target_compile_options(nsan_runtime PRIVATE
    -ffast-math
    -fno-signed-zeros
    -fno-honor-nans
    -fmerge-all-constants
  )
  message(STATUS "NSan runtime: shadow fast-math ON")
endif()

# __nsan_check_consistency_float is called after EVERY float op in instrumented
# code. It must be as fast as possible — mark it for inlining.
if(CMAKE_BUILD_TYPE STREQUAL "Release" OR ENABLE_AGGRESSIVE_OPTIMIZATION)
  target_compile_options(nsan_runtime PRIVATE -finline-functions)
endif()
