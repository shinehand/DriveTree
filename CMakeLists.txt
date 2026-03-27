cmake_minimum_required(VERSION 3.16)

project(DriveTree LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(FLTK REQUIRED)
include(${FLTK_USE_FILE})

add_library(drivetree_lib
  src/drive_tree.cpp
)

target_include_directories(drivetree_lib
  PUBLIC src
)

add_executable(drivetree
  src/main.cpp
)

target_link_libraries(drivetree PRIVATE drivetree_lib ${FLTK_LIBRARIES})

add_executable(drivetree_tests
  tests/drive_tree_test.cpp
)

target_link_libraries(drivetree_tests PRIVATE drivetree_lib)

enable_testing()
add_test(NAME drivetree_tests COMMAND drivetree_tests)
