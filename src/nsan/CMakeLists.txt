# NSan Pass - LLVM Module
add_library(nsan_pass MODULE
  NSanPass.cpp
  ShadowValueMap.cpp
)

# Map LLVM components to link libraries
llvm_map_components_to_libnames(llvm_libs support core irreader passes)

target_include_directories(nsan_pass PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${LLVM_INCLUDE_DIRS}
)

target_link_libraries(nsan_pass PRIVATE ${llvm_libs})

# LLVM passes must be built without RTTI/exceptions (LLVM is compiled that way)
target_compile_options(nsan_pass PRIVATE -fno-rtti -fno-exceptions)
