add_executable(test_spsc test_spsc.cpp)
target_link_libraries(test_spsc PRIVATE turbo_common)
add_test(NAME spsc COMMAND test_spsc)