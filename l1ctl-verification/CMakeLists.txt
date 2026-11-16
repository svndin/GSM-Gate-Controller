cmake_minimum_required(VERSION 3.16)
project(l1ctl_verification LANGUAGES CXX)

add_library(l1ctl_pm STATIC src/l1ctl_pm.cpp)
target_include_directories(l1ctl_pm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(l1ctl_pm PUBLIC cxx_std_17)
target_compile_options(l1ctl_pm PRIVATE -Wall -Wextra -Wpedantic)

add_executable(test_l1ctl_pm tests/test_l1ctl_pm.cpp)
target_link_libraries(test_l1ctl_pm PRIVATE l1ctl_pm)
target_compile_options(test_l1ctl_pm PRIVATE -Wall -Wextra -Wpedantic)

enable_testing()
add_test(NAME l1ctl_pm_conf COMMAND test_l1ctl_pm)
