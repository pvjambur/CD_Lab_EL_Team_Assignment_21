# src/nsan/CMakeLists.txt

if(NOT LLVM_FOUND)
  find_package(LLVM REQUIRED CONFIG)
  include(AddLLVM)
endif()

add_library(nsan_pass MODULE
  NSanPass.cpp
  ShadowValueMap.cpp
)

# Map component names to actual library targets
llvm_map_components_to_libnames(llvm_libs
  support core irreader passes transformutils analysis)

target_include_directories(nsan_pass PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${LLVM_INCLUDE_DIRS}
)

target_link_libraries(nsan_pass PRIVATE ${llvm_libs})

target_compile_options(nsan_pass PRIVATE
  -fno-rtti
  -fno-exceptions
)

if(CMAKE_BUILD_TYPE STREQUAL "Release" OR ENABLE_AGGRESSIVE_OPTIMIZATION)
  target_compile_options(nsan_pass PRIVATE -finline-functions)
  if(ENABLE_AGGRESSIVE_OPTIMIZATION)
    target_compile_options(nsan_pass PRIVATE
      -mllvm -gvn
      -mllvm -licm
      -mllvm -inline-threshold=200
    )
  endif()
endif()