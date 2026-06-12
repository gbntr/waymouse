cmake_minimum_required(VERSION 3.22)

# Tests stub
# TODO: add real tests (Catch2 or GoogleTest)

add_executable(stub_test stub_test.cpp)
target_include_directories(stub_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME stub_test COMMAND stub_test)
