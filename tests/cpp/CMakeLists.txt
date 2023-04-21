cmake_minimum_required(VERSION 3.10)

add_executable(TestCommon TestCommon.cpp)
target_link_libraries(TestCommon dap_sdk)
add_test(NAME TestCommon
         COMMAND TestCommon)

