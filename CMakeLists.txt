# Root CMakeLists.txt
cmake_minimum_required(VERSION 3.12)
project(NSan)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Build-type options ────────────────────────────────────────────────────────
# Usage:
#   Debug:       cmake -DCMAKE_BUILD_TYPE=Debug ..
#   Release:     cmake -DCMAKE_BUILD_TYPE=Release ..
#   Performance: cmake -DCMAKE_BUILD_TYPE=Release \
#                      -DENABLE_AGGRESSIVE_OPTIMIZATION=ON \
#                      -DENABLE_LTO=ON ..

option(ENABLE_AGGRESSIVE_OPTIMIZATION
       "Use -O3 + LLVM loop/inline passes for maximum shadow-computation speed" OFF)
option(ENABLE_LTO
       "Link-Time Optimization — reduces call overhead between pass and runtime" ON)
option(ENABLE_SHADOW_OPTIMIZATION
       "Apply -ffast-math to shadow computations (safe: shadows are for comparison only)" ON)

# ── Default optimisation level ────────────────────────────────────────────────
# -O2 is the balanced default (matches Google NSan's 2.3x overhead target).
# Override to -O3 with ENABLE_AGGRESSIVE_OPTIMIZATION.
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(ENABLE_AGGRESSIVE_OPTIMIZATION)
  # -O3: enables auto-vectorisation, loop unrolling, aggressive inlining
  add_compile_options(-O3)
  message(STATUS "NSan: Aggressive optimisation ON (-O3)")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
  add_compile_options(-O2)
else()
  add_compile_options(-O0 -g)
endif()

# ── LTO ───────────────────────────────────────────────────────────────────────
# Allows the linker to inline __nsan_check_consistency_float() across TUs,
# eliminating the function-call overhead that dominates runtime cost.
if(ENABLE_LTO)
  include(CheckIPOSupported)
  check_ipo_supported(RESULT _lto_ok OUTPUT _lto_err)
  if(_lto_ok)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    message(STATUS "NSan: LTO enabled")
  else()
    message(WARNING "NSan: LTO not supported: ${_lto_err}")
  endif()
endif()

# ── Find LLVM ─────────────────────────────────────────────────────────────────
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(AddLLVM)

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# ── Tests and subdirectories ──────────────────────────────────────────────────
# enable_testing() must come before add_subdirectory(tests)
enable_testing()

add_subdirectory(src/nsan)
add_subdirectory(src/runtime)
add_subdirectory(tests)

message(STATUS "LLVM include dirs: ${LLVM_INCLUDE_DIRS}")
message(STATUS "LLVM definitions:  ${LLVM_DEFINITIONS}")
message(STATUS "Build type:        ${CMAKE_BUILD_TYPE}")
