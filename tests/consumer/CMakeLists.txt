cmake_minimum_required(VERSION 3.25)
project(clang_tidy_consumer VERSION 1.0.0 LANGUAGES CXX)

# Find the installed clang-tidy.cmake package
find_package(clang-tidy.cmake REQUIRED)

# Create a simple executable
add_executable(example main.cpp)

# Register sources for clang-tidy
register_project_sources(example)

# Finalize clang-tidy targets
finalize_clang_tidy_targets()