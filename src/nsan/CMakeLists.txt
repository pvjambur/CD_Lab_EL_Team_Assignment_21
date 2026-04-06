# NSan Pass - LLVM Module
add_library(nsan_pass MODULE
  NSanPass.cpp
  ShadowValueMap.cpp
)

target_include_directories(nsan_pass PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${LLVM_INCLUDE_DIRS}
)
