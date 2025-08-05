# Cross-Platform CI Best Practices for CMake Projects

Based on research of current best practices (2024), here are key recommendations for successful cross-platform CI workflows:

## 1. Shell Selection

**Use bash consistently across all platforms:**
```yaml
- name: Configure CMake
  shell: bash  # Works on Windows, macOS, and Linux
  run: |
    cmake -B build -G Ninja \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

**Why:** GitHub Actions provides bash on all platforms, including Windows. This avoids shell-specific syntax issues (like PowerShell backticks vs bash backslashes).

## 2. CMAKE_INSTALL_PREFIX Best Practices

**Use relative paths for portability:**
```yaml
# Good - relative path
-DCMAKE_INSTALL_PREFIX=build/install

# Avoid - absolute paths can cause issues
-DCMAKE_INSTALL_PREFIX=${{ github.workspace }}/install
```

**Why:** Relative paths work better with CPack and cmake --install --prefix. They also avoid permission issues on different platforms.

## 3. Build Configuration

**Avoid specifying build types for multi-config generators:**
```yaml
# Good - works for both single and multi-config
cmake --build build

# Avoid unless necessary
cmake --build build --config Release
```

**Why:** Windows uses multi-config generators (Visual Studio) by default, while Unix typically uses single-config (Ninja/Make).

## 4. Platform-Specific Tool Installation

**Use platform package managers appropriately:**
```yaml
- name: Install dependencies (Ubuntu)
  if: runner.os == 'Linux'
  run: |
    sudo apt-get update
    sudo apt-get install -y clang-tidy ninja-build

- name: Install dependencies (macOS)
  if: runner.os == 'macOS'
  run: |
    brew install llvm ninja

- name: Install dependencies (Windows)
  if: runner.os == 'Windows'
  run: |
    choco install llvm ninja
```

## 5. Matrix Strategy

**Use matrix builds effectively:**
```yaml
strategy:
  fail-fast: false  # Continue other builds if one fails
  matrix:
    os: [ubuntu-latest, macos-latest, windows-latest]
    include:
      - os: ubuntu-latest
        cmake_generator: "Ninja"
      - os: macos-latest
        cmake_generator: "Ninja"
      - os: windows-latest
        cmake_generator: "Ninja"  # Or "Visual Studio 17 2022"
```

## 6. Path Handling

**Add tools to PATH correctly:**
```yaml
# Linux/macOS
echo "/path/to/bin" >> $GITHUB_PATH

# Windows (PowerShell)
echo "C:\path\to\bin" | Out-File -FilePath $env:GITHUB_PATH -Encoding utf8 -Append
```

## 7. Test Execution

**Use CTest without configuration specification:**
```yaml
- name: Test
  working-directory: build
  run: ctest --output-on-failure --verbose
```

## 8. Common Pitfalls to Avoid

1. **Line continuation in cross-platform scripts:**
   - Use single-line commands or ensure bash shell
   - Avoid mixing shell syntaxes

2. **Hardcoded paths:**
   - Use CMake variables and relative paths
   - Avoid system-specific paths like /usr/local or C:\Program Files

3. **Missing compile_commands.json:**
   - Always set `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` for clang-tidy

4. **Tool version assumptions:**
   - Try multiple versions when installing (e.g., clang-tidy-20, -19, -18)
   - Provide fallbacks

## 9. Debugging CI Failures

**Add diagnostic output:**
```yaml
- name: Diagnostic Info
  if: failure()  # Only run on failure
  run: |
    echo "CMake version: $(cmake --version)"
    echo "Build directory contents:"
    ls -la build || dir build
    echo "Install directory:"
    ls -la build/install || dir build\install
```

## 10. Gitea-Specific Considerations

For self-hosted Gitea runners:
- Document required pre-installed tools
- Consider using Docker containers for consistency
- Adjust paths based on runner configuration

## Example Minimal Working Workflow

```yaml
name: CI

on: [push, pull_request]

jobs:
  test:
    runs-on: ${{ matrix.os }}
    strategy:
      matrix:
        os: [ubuntu-latest, macos-latest, windows-latest]
    
    steps:
    - uses: actions/checkout@v4
    
    - name: Configure
      shell: bash
      run: cmake -B build -DCMAKE_INSTALL_PREFIX=build/install
    
    - name: Build
      run: cmake --build build
    
    - name: Test
      working-directory: build
      run: ctest --output-on-failure
```

This approach ensures maximum compatibility and reduces platform-specific issues.