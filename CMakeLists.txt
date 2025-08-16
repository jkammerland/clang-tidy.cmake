cmake_minimum_required(VERSION 3.25)
project(clang-tidy.cmake VERSION 1.2.1)

# Also has list_file_include_guard
include(clang_tidy_functions.cmake)

# add_subdirectory guard (should never happen in any sane project)
project_include_guard()

if(CLANG_TIDY_CMAKE_INSTALL OR ${PROJECT_NAME}_BUILD_TESTS)
  # For the tests, we need to install target_install_package for a so that we can test a consumer
  include(FetchContent)
  FetchContent_Declare(
    target_install_package
    GIT_REPOSITORY https://github.com/jkammerland/target_install_package.cmake.git
    GIT_TAG v5.6.2
    FIND_PACKAGE_ARGS)
  FetchContent_MakeAvailable(target_install_package)

  # Dummy target that can be installed
  add_library(${PROJECT_NAME} INTERFACE)

  # Install dummy target with the cmake files
  target_install_package(
    ${PROJECT_NAME}
    ADDITIONAL_FILES
    ${CMAKE_CURRENT_LIST_DIR}/list_file_include_guard.cmake
    ${CMAKE_CURRENT_LIST_DIR}/project_include_guard.cmake
    ${CMAKE_CURRENT_LIST_DIR}/project_log.cmake
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
endif()
