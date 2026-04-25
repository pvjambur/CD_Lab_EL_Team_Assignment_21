# NSan Pass — LLVM Module
# Builds libnsan_pass.so: the loadable LLVM plugin that instruments FP ops.
if(NOT LLVM_FOUND)
  find_package(LLVM REQUIRED CONFIG)
  include(AddLLVM)
  include_directories(${LLVM_INCLUDE_DIRS})
  add_definitions(${LLVM_DEFINITIONS})
endif()

add_library(nsan_pass MODULE
  NSanPass.cpp
  ShadowValueMap.cpp
)

# Map LLVM components needed by the pass
llvm_map_components_to_libnames(llvm_libs support core irreader passes transformutils)

target_include_directories(nsan_pass PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${LLVM_INCLUDE_DIRS}
)

target_link_libraries(nsan_pass PRIVATE ${llvm_libs})

# LLVM passes MUST be built without RTTI/exceptions because LLVM itself is.
# Scoped to this target only — the runtime and tests are unaffected.
target_compile_options(nsan_pass PRIVATE
  -fno-rtti
  -fno-exceptions
)

# LLVM pass optimisation flags.
# -finline-functions: inline small helpers into runOnFunction
# -mllvm -gvn: remove redundant shadow loads (Global Value Numbering)
# -mllvm -licm: hoist loop-invariant shadow constants out of loops
# These are only applied in Release builds to keep debug symbols clean.
if(CMAKE_BUILD_TYPE STREQUAL "Release" OR ENABLE_AGGRESSIVE_OPTIMIZATION)
  target_compile_options(nsan_pass PRIVATE
    -finline-functions
  )
  if(ENABLE_AGGRESSIVE_OPTIMIZATION)
    target_compile_options(nsan_pass PRIVATE
      -mllvm -gvn
      -mllvm -licm
      -mllvm -inline-threshold=200
    )
  endif()
endif()
