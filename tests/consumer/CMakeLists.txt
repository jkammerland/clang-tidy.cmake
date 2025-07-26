cmake_minimum_required(VERSION 3.25)
project(clang_tidy_consumer VERSION 1.0.0)

# Add the install location to the prefix path
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../build/install")

# Find the installed clang-tidy package
find_package(clang-tidy 1.0 REQUIRED)

# The clang-tidy package provides the functions through inclusion
# We need to include the main CMakeLists.txt to get the functions
include(${CMAKE_CURRENT_SOURCE_DIR}/../../CMakeLists.txt)

# Create a simple executable
add_executable(example main.cpp)

# Register sources for clang-tidy
target_tidy_sources(example)

# Finalize clang-tidy targets
finalize_clang_tidy_targets()