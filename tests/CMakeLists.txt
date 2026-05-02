# tests/CMakeLists.txt
cmake_minimum_required(VERSION 3.10)

file(GLOB TEST_SRCS "*.cpp")

foreach(TEST_SRC IN LISTS TEST_SRCS)
    get_filename_component(TEST_NAME ${TEST_SRC} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_SRC})
    
    # Link our static runtime library target
    target_link_libraries(${TEST_NAME} PRIVATE nsan_runtime)
    
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()
# Add unit test target
add_executable(test_shadow_map unit/test_shadow_map.cpp)
target_link_libraries(test_shadow_map PRIVATE nsan_runtime)
add_test(NAME ShadowMapTest COMMAND test_shadow_map)