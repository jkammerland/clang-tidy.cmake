# Clang-Tidy-Cmake

## pre-requisites

```cmake
# CMakeLists.txt
include(FetchContent)

FetchContent_Declare(
  ClangTidyCmake
  GIT_REPOSITORY https://github.com/jkammerland/clang-tidy.cmake.git
  GIT_TAG        1.0.0
)

FetchContent_MakeAvailable(ClangTidyCmake)
```

## Usage Example

```cmake
file(GLOB TIDY_TESTS CONFIGURE_DEPENDS "*.cpp")
add_executable(tests ${TIDY_TESTS})
register_project_sources(tests)

# --- Finalize and Create Tidy Targets ---
# This call MUST be at the end, after all targets and add_subdirectory calls.
finalize_clang_tidy_targets()
```

## Running

1.  **Configure CMake:**
    ```bash
    mkdir build && cd build
    cmake ..
    ```
    *   To run on a specific file:
        `cmake .. -DCLANG_TIDY_FILE="src/main.cpp"`
    *   To change target names (less common):
        `cmake .. -DCLANG_TIDY_TARGET_NAME="lint" -DCLANG_TIDY_FIX_TARGET_NAME="lint-fix"`

2.  **Run Tidy Targets:**
    *   **Check for issues (read-only):**
        ```bash
        cmake --build . --target tidy # Or your custom CLANG_TIDY_TARGET_NAME
        # or: make tidy / ninja tidy
        ```
    *   **Apply fixes:**
        ```bash
        cmake --build . --target tidy-fix # Or your custom CLANG_TIDY_FIX_TARGET_NAME
        # or: make tidy-fix / ninja tidy-fix
        ```

A cross-platform way to integrate `clang-tidy` into your build process, respecting your existing `.clang-tidy` configuration and allowing both full project and single-file analysis.
