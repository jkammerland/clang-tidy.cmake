# Example demonstrating per-target clang-tidy usage
cmake_minimum_required(VERSION 3.25)
project(PerTargetTidyExample CXX)

# Include clang-tidy.cmake
include(../CMakeLists.txt)

# Create multiple targets
add_executable(app1 app1.cpp)
add_executable(app2 app2.cpp)
add_library(mylib lib.cpp)

# Register sources for each target
register_project_sources(app1)
register_project_sources(app2)
register_project_sources(mylib)

# Option 1: Explicitly finalize specific targets
# This creates tidy-app1 and tidy-app1-fix targets
finalize_clang_tidy_target(app1)

# Option 2: Let finalize_tidy_targets() handle the rest
# This creates:
# - Global tidy and tidy-fix targets (all sources)
# - tidy-app2 and tidy-app2-fix (auto-finalized)
# - tidy-mylib and tidy-mylib-fix (auto-finalized)
finalize_tidy_targets()