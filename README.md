# Clang-Tidy-Cmake

### Using FetchContent
```cmake
# CMakeLists.txt
include(FetchContent)

FetchContent_Declare(
  ClangTidyCmake
  GIT_REPOSITORY https://github.com/jkammerland/clang-tidy.cmake.git
  GIT_TAG        1.1.0
)

FetchContent_MakeAvailable(ClangTidyCmake)
```

### Using cpmaddpackage (FetchContent wrapper)
```cmake
cpmaddpackage("gh:jkammerland/clang-tidy.cmake@1.1.0")
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

### Basic Usage - Global Tidy Targets

```cmake
file(GLOB TIDY_TESTS CONFIGURE_DEPENDS "*.cpp")
add_executable(tests ${TIDY_TESTS})
target_tidy_sources(tests)  # Register target for tidy

# Other targets...
# target_tidy_sources(...)
# ...

# --- Finalize and Create Tidy Targets ---
# This call MUST be at the end, after all targets and add_subdirectory calls.
finalize_tidy_targets()
```

### Per-Target Tidy

You can create individual tidy targets for specific CMake targets without waiting for the global finalize:

```cmake
add_executable(myapp src/main.cpp src/utils.cpp)
target_tidy_sources(myapp)  # Register target
finalize_tidy_target(myapp) # Create tidy targets immediately

# This creates:
#   tidy-myapp     - Check only myapp's sources
#   tidy-myapp-fix - Apply fixes to myapp's sources

add_library(mylib src/lib.cpp src/helper.cpp)
target_tidy_sources(mylib)  # Register target
finalize_tidy_target(mylib) # Create tidy targets immediately

# This creates:
#   tidy-mylib     - Check only mylib's sources
#   tidy-mylib-fix - Apply fixes to mylib's sources

# Later, at the end of your CMakeLists.txt:
finalize_tidy_targets()  # Creates global targets and auto-finalizes any remaining
```

### How Per-Target and Global Targets Interact

1. **Registration**: `target_tidy_sources(target)` registers a target's sources for tidy processing
2. **Per-Target Finalization**: `finalize_tidy_target(target)` creates target-specific tidy targets immediately
3. **Global Finalization**: `finalize_tidy_targets()` does two things:
   - Creates global `tidy` and `tidy-fix` targets that process ALL registered sources
   - Auto-finalizes any registered targets that haven't been explicitly finalized yet

**Important Notes:**
- Each target can only be finalized once - subsequent calls are safely ignored
- The global `tidy` target always includes ALL registered sources, regardless of per-target finalization
- Per-target tidy targets are independent - you can run `tidy-myapp` without affecting other targets
- If you never call `finalize_tidy_target()`, the global `finalize_tidy_targets()` will create per-target targets automatically

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
