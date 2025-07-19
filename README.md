# Clang-Tidy-Cmake

### Using FetchContent
```cmake
# CMakeLists.txt
include(FetchContent)

FetchContent_Declare(
  ClangTidyCmake
  GIT_REPOSITORY https://github.com/jkammerland/clang-tidy.cmake.git
  GIT_TAG        1.0.4
)

FetchContent_MakeAvailable(ClangTidyCmake)
```

### Using cpmaddpackage (FetchContent wrapper)
```cmake
cpmaddpackage("gh:jkammerland/clang-tidy.cmake@1.0.4")
```

### Manual install

```cmake
mkdir build && cd build
cmake .. -DCLANG_TIDY_CMAKE_INSTALL=ON # OR -DCMAKE_INSTALL_PREFIX=/path/to/install
cmake --install .
```

Then in the consumer project:

```cmake
find_package(clang-tidy.cmake CONFIG REQUIRED)
```

## Usage Example

```cmake
file(GLOB TIDY_TESTS CONFIGURE_DEPENDS "*.cpp")
add_executable(tests ${TIDY_TESTS})
target_tidy_sources(tests)

# Other targets...
# target_tidy_sources(...)
# ...

# --- Finalize and Create Tidy Targets ---
# This call MUST be at the end, after all targets and add_subdirectory calls.
finalize_clang_tidy_targets()
```

## Running

1.  **Configure CMake:**
    ```bash
    mkdir build && cd build
    cmake ..
    # Do not forget to set the compile_commands.json!
    # Can be generated with:
    # cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    ```
    *   Single-threaded option (default is automatic multi-threaded):
        `cmake .. -DTIDY_SINGLE_THREADED=ON`

2.  **Run Tidy Targets:**
    *   **Check for issues (read-only):**
        ```bash
        cmake --build . --target tidy
        # or: make tidy / ninja tidy
        ```
    *   **Apply fixes:**
        ```bash
        cmake --build . --target tidy-fix
        # or: make tidy-fix / ninja tidy-fix
        ```

A cross-platform way to integrate `clang-tidy` into your build process, respecting your existing `.clang-tidy` configuration and allowing both full project and single-file analysis.
