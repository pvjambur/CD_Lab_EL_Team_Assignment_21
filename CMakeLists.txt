# Root CMakeLists.txt
cmake_minimum_required(VERSION 3.12)
project(NSan)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find LLVM
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

# Add LLVM modules
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(AddLLVM)

# Include directories
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Compiler flags (LLVM style)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -fno-exceptions")

# Add subdirectories
add_subdirectory(src/nsan)
add_subdirectory(src/runtime)
add_subdirectory(tests)

# Enable testing
enable_testing()

# Print configuration info
message(STATUS "LLVM libs: ${LLVM_LIBRARIES}")
message(STATUS "LLVM include dirs: ${LLVM_INCLUDE_DIRS}")
message(STATUS "LLVM definitions: ${LLVM_DEFINITIONS}")
