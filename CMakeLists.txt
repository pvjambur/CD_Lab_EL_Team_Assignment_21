# Root CMakeLists.txt
cmake_minimum_required(VERSION 3.20)
project(NSan)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── LLVM ──────────────────────────────────────────────────────────────────────
# Default: look for a sibling llvm-workspace build.
# Override with:  cmake .. -DLLVM_DIR=/your/path/lib/cmake/llvm
set(LLVM_DIR
    "${CMAKE_SOURCE_DIR}/../llvm-workspace/build/lib/cmake/llvm"
    CACHE PATH "Path to LLVMConfig.cmake")

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(AddLLVM)

# LLVM_INCLUDE_DIRS already contains both the source-tree include dir
# AND the build-tree include dir (where generated headers live).
# Do NOT add extra symlinks — that just causes "too many levels" errors.
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# ── Build-type options ────────────────────────────────────────────────────────
option(ENABLE_AGGRESSIVE_OPTIMIZATION
       "Use -O3 + LLVM loop/inline passes for maximum shadow-computation speed"
       OFF)
option(ENABLE_LTO
       "Link-Time Optimization — reduces call overhead between pass and runtime"
       ON)
option(ENABLE_SHADOW_OPTIMIZATION
       "Apply -ffast-math to shadow computations"
       ON)

# ── Default optimisation level ────────────────────────────────────────────────
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(ENABLE_AGGRESSIVE_OPTIMIZATION)
  add_compile_options(-O3)
  message(STATUS "NSan: Aggressive optimisation ON (-O3)")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release" OR
       CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
  add_compile_options(-O2)
else()
  add_compile_options(-O0 -g)
endif()

# ── LTO ───────────────────────────────────────────────────────────────────────
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

# ── Subdirectories ────────────────────────────────────────────────────────────
enable_testing()

add_subdirectory(src/nsan)
add_subdirectory(src/runtime)

message(STATUS "LLVM include dirs: ${LLVM_INCLUDE_DIRS}")
message(STATUS "LLVM definitions:  ${LLVM_DEFINITIONS}")
message(STATUS "Build type:        ${CMAKE_BUILD_TYPE}")