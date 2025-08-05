cmake_minimum_required(VERSION 3.25)
project(clang_tidy_consumer VERSION 1.0.0 LANGUAGES CXX)

# Find the installed clang-tidy.cmake package
# IMPORTANT: YOU MUST INSTALL IT FIRST
find_package(clang-tidy.cmake REQUIRED)

# Create a simple executable
add_executable(example main.cpp)

# Set C++11 standard for the example code
set_target_properties(example PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON)

# Register sources for clang-tidy
register_project_sources(example)

# Finalize clang-tidy targets
finalize_tidy_targets()