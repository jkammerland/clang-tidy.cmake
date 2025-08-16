# Clang-Tidy-Cmake

### Using FetchContent
```cmake
# CMakeLists.txt
include(FetchContent)

FetchContent_Declare(
  ClangTidyCmake
  GIT_REPOSITORY https://github.com/jkammerland/clang-tidy.cmake.git
  GIT_TAG        1.2.1
)

FetchContent_MakeAvailable(ClangTidyCmake)
# or 
# cpmaddpackage("gh:jkammerland/clang-tidy.cmake@1.2.1")
```

Usually you should wrap this to avoid exposing tidy to a consumer of this CMake project, e.g
```cmake
# add_library(${PROJECT_NAME} ...)

option(${PROJECT_NAME}_ENABLE_CLANG_TIDY_CMAKE "Enable clang-tidy.cmake" OFF)
if(${PROJECT_NAME}_ENABLE_CLANG_TIDY_CMAKE)
  include(FetchContent)
  FetchContent_Declare(
    ClangTidyCmake
    GIT_REPOSITORY https://github.com/jkammerland/clang-tidy.cmake.git
    GIT_TAG 1.2.1
    # Optional arg to first try find_package locally before fetching, see manual installation
    # NOTE: This must be called last, with 0 to N args following FIND_PACKAGE_ARGS
    # FIND_PACKAGE_ARGS
  )
  FetchContent_MakeAvailable(ClangTidyCmake)
  
  # add tidy target checking your target's sources
  target_tidy_sources(${PROJECT_NAME})
endif()
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
target_tidy_sources(tests)  # Register target for tidy

add_library(mylib src/lib.cpp)
target_tidy_sources(mylib)  # Register another target

# Tidy targets are automatically created at the end of configuration
```

This creates:
- Global targets: `tidy` and `tidy-fix` (process all registered sources)
- Per-target: `tidy-tests`, `tidy-tests-fix`, `tidy-mylib`, `tidy-mylib-fix`

E.g, the tests from this repo look like this:

[demo](https://github.com/user-attachments/assets/4e9afb19-c1b9-4b83-90d5-38d7e3b002be)

> [!TIP]
> Some tidy issues can be automatically fixed by tidy itself, this library creates CMake custom targets to apply these fixes.

## Configuration Options

*   **Disable clang-tidy entirely** (can save configuration time):
```bash
cmake .. -DENABLE_CLANG_TIDY=OFF
```
*   **Single-threaded mode** (default is multi-threaded):
```bash
cmake .. -DTIDY_SINGLE_THREADED=ON
```

## Running

1.  **Configure CMake:**
```bash
mkdir build && cd build
cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

2.  **List Available Tidy Targets:**
    
> [!TIP]
> To see all tidy targets:
> ```bash
> cmake --build . --target help | grep ".*: phony" | grep -v "/" | cut -d: -f1 | grep tidy
> ```

3.  **Run Tidy Targets:**
*   **Global targets** (all registered sources):
    ```bash
    cmake --build . --target tidy        # Check for issues
    cmake --build . --target tidy-fix    # Apply fixes
    ```
*   **Per-target** (specific target only):
    ```bash
    cmake --build . --target tidy-mylib        # Check mylib only
    cmake --build . --target tidy-mylib-fix    # Apply fixes to mylib only
    ```

A cross-platform way to integrate `clang-tidy` into your build process, respecting your existing `.clang-tidy` configuration and allowing both full project and single-file analysis.
