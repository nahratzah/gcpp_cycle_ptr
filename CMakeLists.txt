cmake_minimum_required(VERSION 3.8)
project (gcpp_cycle_ptr)

add_executable(cycle_ptr_test test.cc gptr.cc member_ptr.cc)
target_compile_features (cycle_ptr_test PUBLIC cxx_std_17)
