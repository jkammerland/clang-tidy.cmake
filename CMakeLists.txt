cmake_minimum_required(VERSION 3.25)
project(clang-tidy.cmake VERSION 1.1.0)

include(clang_tidy_functions.cmake)

project_include_guard()

if(CLANG_TIDY_CMAKE_INSTALL OR ${PROJECT_NAME}_BUILD_TESTS)
  if(NOT target_install_package_FOUND)
    # Some tests are dependent on installing the project in the build directory
    include(FetchContent)
    FetchContent_Declare(
      target_install_package
      GIT_REPOSITORY https://github.com/jkammerland/target_install_package.cmake.git
      GIT_TAG v5.3.1)
    FetchContent_MakeAvailable(target_install_package)
  endif()

  add_library(${PROJECT_NAME} INTERFACE)

  target_install_package(
    ${PROJECT_NAME}
    ADDITIONAL_FILES
    ${CMAKE_CURRENT_LIST_DIR}/list_file_include_guard.cmake
    ${CMAKE_CURRENT_LIST_DIR}/project_include_guard.cmake
    ${CMAKE_CURRENT_LIST_DIR}/.clang-tidy
    ADDITIONAL_FILES_DESTINATION
    ${CMAKE_INSTALL_DATADIR}/cmake/${PROJECT_NAME}
    PUBLIC_CMAKE_FILES
    ${CMAKE_CURRENT_LIST_DIR}/clang_tidy_functions.cmake
    RUNTIME_COMPONENT
    "ClangTidy"
    DEVELOPMENT_COMPONENT
    "ClangTidy")
endif()

if(${PROJECT_NAME}_BUILD_TESTS)
  include(CTest)
  enable_testing()
  add_subdirectory(tests)

  # --- Finalize and Create Tidy Targets ---
  # This call MUST be at the end, after all targets and add_subdirectory calls.
  finalize_tidy_targets()
endif()
