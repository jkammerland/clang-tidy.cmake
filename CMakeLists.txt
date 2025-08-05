cmake_minimum_required(VERSION 3.25)
project(clang-tidy.cmake VERSION 1.0.4)

if(NOT COMMAND list_file_include_guard)
  find_package(target_install_package)

  if(NOT target_install_package_FOUND)
    include(list_file_include_guard.cmake)
    include(project_include_guard.cmake)
  else()
    project_log(VERBOSE "target_install_package found, using project_include_guard from target_install_package")
  endif()
endif()

project_include_guard()

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/.clang-tidy)
  if(COMMAND project_log)
    project_log(VERBOSE "No .clang-tidy file found in project root, you can use this for reference:")
    project_log(VERBOSE "--> ${CMAKE_CURRENT_LIST_DIR}/.clang-tidy")
  else()
    message(VERBOSE "No .clang-tidy file found in project root, you can use this for reference:")
    message(VERBOSE "--> ${CMAKE_CURRENT_LIST_DIR}/.clang-tidy")
  endif()
endif()

# --- Global CACHE variables for collected sources ---
set(PROJECT_TRANSLATION_UNITS_FOR_TIDY
    ""
    CACHE INTERNAL "Collected TUs for clang-tidy")
set(PROJECT_HEADERS_FOR_TIDY
    ""
    CACHE INTERNAL "Collected Headers for clang-tidy") # Headers are collected but not directly processed per-header by this script

if(NOT CMAKE_EXPORT_COMPILE_COMMANDS)
  message(FATAL_ERROR "Compilation database is required by clang-tidy, set with -DCMAKE_EXPORT_COMPILE_COMMANDS=ON")
endif()

# --- Will control multi-threading of clang-tidy too ---
option(TIDY_USES_TERMINAL "Serializes output with colors, also makes it single threaded" OFF)
option(TIDY_SINGLE_THREADED "Single threaded clang-tidy" OFF)

# Alias for the old function, for backwards compatibility
function(target_tidy_sources TARGET_NAME)
  register_project_sources(${TARGET_NAME})
endfunction(target_tidy_sources)

function(register_project_sources TARGET_NAME)
  # Check if the global list variables are defined, error out if not (as a safeguard)
  if(NOT DEFINED CACHE{PROJECT_TRANSLATION_UNITS_FOR_TIDY} OR NOT DEFINED CACHE{PROJECT_HEADERS_FOR_TIDY}) # Check cache existence
    message(FATAL_ERROR "CACHE variables PROJECT_TRANSLATION_UNITS_FOR_TIDY and PROJECT_HEADERS_FOR_TIDY must be initialized before calling target_tidy_sources.")
    return()
  endif()

  get_target_property(TARGET_SOURCES ${TARGET_NAME} SOURCES)
  if(NOT TARGET_SOURCES)
    message(WARNING "Target ${TARGET_NAME} has no sources or does not exist when registering for tidy.")
    return()
  endif()

  set(LOCAL_TUS_FOR_TARGET "")
  set(LOCAL_HEADERS_FOR_TARGET "")

  foreach(FILE_PATH_RELATIVE ${TARGET_SOURCES})
    if(IS_ABSOLUTE "${FILE_PATH_RELATIVE}")
      set(ABS_FILE_PATH "${FILE_PATH_RELATIVE}")
    else()
      get_target_property(TARGET_SOURCE_DIR ${TARGET_NAME} SOURCE_DIR)
      if(NOT TARGET_SOURCE_DIR)
        set(TARGET_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
        message(WARNING "Could not get SOURCE_DIR for target ${TARGET_NAME}. Using CMAKE_CURRENT_SOURCE_DIR (${CMAKE_CURRENT_SOURCE_DIR}) for resolving relative paths.")
      endif()
      get_filename_component(ABS_FILE_PATH ${FILE_PATH_RELATIVE} ABSOLUTE BASE_DIR ${TARGET_SOURCE_DIR})
    endif()

    if(EXISTS "${ABS_FILE_PATH}")
      if(ABS_FILE_PATH MATCHES "\\.(c|cpp|cc|cxx|cu)$")
        list(APPEND LOCAL_TUS_FOR_TARGET "${ABS_FILE_PATH}")
      elseif(ABS_FILE_PATH MATCHES "\\.(h|hpp|hh|hxx|cuh)$")
        list(APPEND LOCAL_HEADERS_FOR_TARGET "${ABS_FILE_PATH}")
      endif()
    else()
      message(WARNING "Tidy: File from target ${TARGET_NAME} not found: ${ABS_FILE_PATH} (original: ${FILE_PATH_RELATIVE} in ${TARGET_SOURCE_DIR})")
    endif()
  endforeach()

  if(LOCAL_TUS_FOR_TARGET)
    set(TEMP_TUS ${PROJECT_TRANSLATION_UNITS_FOR_TIDY})
    list(APPEND TEMP_TUS ${LOCAL_TUS_FOR_TARGET})
    list(REMOVE_DUPLICATES TEMP_TUS)
    set(PROJECT_TRANSLATION_UNITS_FOR_TIDY
        ${TEMP_TUS}
        CACHE INTERNAL "Collected TUs for clang-tidy" FORCE)
  endif()

  if(LOCAL_HEADERS_FOR_TARGET)
    set(TEMP_HEADERS ${PROJECT_HEADERS_FOR_TIDY})
    list(APPEND TEMP_HEADERS ${LOCAL_HEADERS_FOR_TARGET})
    list(REMOVE_DUPLICATES TEMP_HEADERS)
    set(PROJECT_HEADERS_FOR_TIDY
        ${TEMP_HEADERS}
        CACHE INTERNAL "Collected Headers for clang-tidy" FORCE)
  endif()
endfunction()

function(finalize_clang_tidy_targets)
  if(NOT DEFINED CACHE{PROJECT_TRANSLATION_UNITS_FOR_TIDY})
    message(FATAL_ERROR "CACHE variable PROJECT_TRANSLATION_UNITS_FOR_TIDY must be initialized before calling finalize_clang_tidy_targets.")
  endif()

  option(ENABLE_CLANG_TIDY "Enable clang-tidy integration" ON)
  option(CLANG_TIDY_APPLY_FIXES_BY_DEFAULT "Hint that clang-tidy should apply fixes (use clang-tidy-fix target)" OFF) # This option is less relevant with per-TU commands, but can be kept
  set(CLANG_TIDY_FILE
      ""
      CACHE STRING "Path to a specific file to run clang-tidy on (relative to project root or absolute)")
  set(CLANG_TIDY_TARGET_NAME
      "tidy"
      CACHE STRING "Name for the clang-tidy check-only target")
  set(CLANG_TIDY_FIX_TARGET_NAME
      "${CLANG_TIDY_TARGET_NAME}-fix"
      CACHE STRING "Name for the clang-tidy apply-fixes target")
  set(CLANG_TIDY_HEADER_FILTER
      ""
      CACHE STRING "Regex for filtering which headers to check (overrides HeaderFilterRegex in .clang-tidy if set)")
  set(CLANG_TIDY_HEADER_EXCLUDE
      ""
      CACHE STRING "Regex for excluding headers from checking (optional, overrides ExcludeHeaderFilterRegex in .clang-tidy if set)")

  if(ENABLE_CLANG_TIDY)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # Crucial for clang-tidy

    find_program(
      CLANG_TIDY_EXE
      NAMES clang-tidy clang-tidy-20 clang-tidy-19 clang-tidy-18 clang-tidy-17
      DOC "Path to clang-tidy executable" REQUIRED)
    message(STATUS "Found clang-tidy: ${CLANG_TIDY_EXE}")

    set(TIDY_COMMON_ARGS "")
    # The -p argument tells clang-tidy where to find compile_commands.json
    list(APPEND TIDY_COMMON_ARGS "-p=${CMAKE_BINARY_DIR}")
    # Add color output option
    option(CLANG_TIDY_USE_COLOR "Enable colored output for clang-tidy" ON)
    if(CLANG_TIDY_USE_COLOR)
      list(APPEND TIDY_COMMON_ARGS "--use-color")
    endif()
    # Add header filter if specified (overrides HeaderFilterRegex in .clang-tidy)
    if(CLANG_TIDY_HEADER_FILTER AND NOT CLANG_TIDY_HEADER_FILTER STREQUAL "")
      list(APPEND TIDY_COMMON_ARGS "--header-filter=${CLANG_TIDY_HEADER_FILTER}")
    endif()
    # Add header exclude filter if specified (overrides ExcludeHeaderFilterRegex in .clang-tidy)
    if(CLANG_TIDY_HEADER_EXCLUDE AND NOT CLANG_TIDY_HEADER_EXCLUDE STREQUAL "")
      list(APPEND TIDY_COMMON_ARGS "--exclude-header-filter=${CLANG_TIDY_HEADER_EXCLUDE}")
    endif()

    if(EXISTS "${CMAKE_SOURCE_DIR}/.clang-tidy")
      message(STATUS "Using .clang-tidy configuration file from project root: ${CMAKE_SOURCE_DIR}/.clang-tidy")
      if(CLANG_TIDY_HEADER_FILTER AND NOT CLANG_TIDY_HEADER_FILTER STREQUAL "")
        message(STATUS "  NOTE: CLANG_TIDY_HEADER_FILTER='${CLANG_TIDY_HEADER_FILTER}' overrides HeaderFilterRegex in .clang-tidy")
      endif()
      if(CLANG_TIDY_HEADER_EXCLUDE AND NOT CLANG_TIDY_HEADER_EXCLUDE STREQUAL "")
        message(STATUS "  NOTE: CLANG_TIDY_HEADER_EXCLUDE='${CLANG_TIDY_HEADER_EXCLUDE}' overrides ExcludeHeaderFilterRegex in .clang-tidy")
      endif()
    else()
      message(WARNING "No .clang-tidy file found in project root (${CMAKE_SOURCE_DIR}/.clang-tidy). Clang-tidy will use its default checks. It is highly recommended to create a .clang-tidy file.")
    endif()

    set(TIDY_CHECK_ARGS ${TIDY_COMMON_ARGS})
    set(TIDY_FIX_ARGS ${TIDY_COMMON_ARGS})
    list(APPEND TIDY_FIX_ARGS "--fix")
    # For --fix-errors, you might want to add it here or make it conditional list(APPEND TIDY_FIX_ARGS "--fix-errors")

    set(CURRENT_FILES_TO_PROCESS ${PROJECT_TRANSLATION_UNITS_FOR_TIDY}) # Reads from CACHE
    set(TIDY_OPERATION_SCOPE "all registered project translation units")

    if(CLANG_TIDY_FILE AND NOT CLANG_TIDY_FILE STREQUAL "")
      if(IS_ABSOLUTE "${CLANG_TIDY_FILE}")
        set(SPECIFIC_FILE "${CLANG_TIDY_FILE}")
      else()
        set(SPECIFIC_FILE "${CMAKE_SOURCE_DIR}/${CLANG_TIDY_FILE}")
      endif()

      if(EXISTS "${SPECIFIC_FILE}")
        set(CURRENT_FILES_TO_PROCESS "${SPECIFIC_FILE}") # This will now be a list with one element
        set(TIDY_OPERATION_SCOPE "specified file: ${CLANG_TIDY_FILE}")
        message(STATUS "Clang-tidy targets will run on specific file: ${SPECIFIC_FILE}")
      else()
        message(WARNING "CLANG_TIDY_FILE specified ('${CLANG_TIDY_FILE}'), but not found at '${SPECIFIC_FILE}'. Running on ${TIDY_OPERATION_SCOPE}.")
      endif()
    endif()

    if(NOT CURRENT_FILES_TO_PROCESS)
      message(WARNING "No files available for clang-tidy (either none registered or specified file not found). Tidy targets will do nothing.")
      add_custom_target(${CLANG_TIDY_TARGET_NAME} COMMENT "No files to process with clang-tidy.")
      add_custom_target(${CLANG_TIDY_FIX_TARGET_NAME} COMMENT "No files to process with clang-tidy (fix mode).")
    else()
      set(TIDY_STAMP_FILES "")
      set(TIDY_FIX_STAMP_FILES "")
      set(STAMP_DIR "${CMAKE_BINARY_DIR}/tidy_stamps")
      file(MAKE_DIRECTORY ${STAMP_DIR}) # Ensure the directory for stamp files exists

      # This file is essential for clang-tidy to know how to compile each TU
      set(COMPILE_COMMANDS_JSON "${CMAKE_BINARY_DIR}/compile_commands.json")

      foreach(TU_FILE ${CURRENT_FILES_TO_PROCESS})
        # Create a unique but predictable name for the stamp file based on the TU path Use CMAKE_PATH for proper cross-platform path handling
        cmake_path(GET TU_FILE FILENAME TU_FILENAME)
        cmake_path(GET TU_FILE PARENT_PATH TU_PARENT)
        cmake_path(HASH TU_PARENT TU_PARENT_HASH)
        # Combine filename and parent hash for unique identifier
        set(SAFE_TU_IDENTIFIER "${TU_FILENAME}_${TU_PARENT_HASH}")
        string(MAKE_C_IDENTIFIER "${SAFE_TU_IDENTIFIER}" SAFE_TU_IDENTIFIER)

        set(CHECK_STAMP_FILE "${STAMP_DIR}/${SAFE_TU_IDENTIFIER}.checked")
        set(FIX_STAMP_FILE "${STAMP_DIR}/${SAFE_TU_IDENTIFIER}.fixed")

        # Custom command for checking a single TU
        if(TIDY_USES_TERMINAL OR TIDY_SINGLE_THREADED)
          add_custom_command(
            OUTPUT ${CHECK_STAMP_FILE}
            COMMAND ${CMAKE_COMMAND} -E echo "Tidying (check): ${TU_FILE}" # Optional: print which file is being processed
            COMMAND ${CLANG_TIDY_EXE} ${TIDY_CHECK_ARGS} "${TU_FILE}"
            COMMAND ${CMAKE_COMMAND} -E touch ${CHECK_STAMP_FILE} # Create stamp file on success
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} # Or CMAKE_BINARY_DIR if preferred, but ensure -p points correctly
            DEPENDS ${COMPILE_COMMANDS_JSON} ${TU_FILE} ${TU_FILE} # Re-run if compile commands or source file change
            COMMENT "Running clang-tidy (check) on ${TU_FILE}"
            VERBATIM USES_TERMINAL)
        else()
          add_custom_command(
            OUTPUT ${CHECK_STAMP_FILE}
            COMMAND ${CMAKE_COMMAND} -E echo "Tidying (check): ${TU_FILE}" # Optional: print which file is being processed
            COMMAND ${CLANG_TIDY_EXE} ${TIDY_CHECK_ARGS} "${TU_FILE}"
            COMMAND ${CMAKE_COMMAND} -E touch ${CHECK_STAMP_FILE} # Create stamp file on success
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} # Or CMAKE_BINARY_DIR if preferred, but ensure -p points correctly
            DEPENDS ${COMPILE_COMMANDS_JSON} ${TU_FILE} ${TU_FILE} # Re-run if compile commands or source file change
            COMMENT "Running clang-tidy (check) on ${TU_FILE}"
            VERBATIM)
        endif()
        list(APPEND TIDY_STAMP_FILES ${CHECK_STAMP_FILE})

        # Custom command for fixing a single TU
        if(TIDY_USES_TERMINAL OR TIDY_SINGLE_THREADED)
          add_custom_command(
            OUTPUT ${FIX_STAMP_FILE}
            COMMAND ${CMAKE_COMMAND} -E echo "Tidying (fix): ${TU_FILE}"
            COMMAND ${CLANG_TIDY_EXE} ${TIDY_FIX_ARGS} "${TU_FILE}"
            COMMAND ${CMAKE_COMMAND} -E touch ${FIX_STAMP_FILE}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            DEPENDS ${COMPILE_COMMANDS_JSON} ${TU_FILE}
            COMMENT "Running clang-tidy (apply-fixes) on ${TU_FILE}"
            VERBATIM USES_TERMINAL)
        else()
          add_custom_command(
            OUTPUT ${FIX_STAMP_FILE}
            COMMAND ${CMAKE_COMMAND} -E echo "Tidying (fix): ${TU_FILE}"
            COMMAND ${CLANG_TIDY_EXE} ${TIDY_FIX_ARGS} "${TU_FILE}"
            COMMAND ${CMAKE_COMMAND} -E touch ${FIX_STAMP_FILE}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            DEPENDS ${COMPILE_COMMANDS_JSON} ${TU_FILE}
            COMMENT "Running clang-tidy (apply-fixes) on ${TU_FILE}"
            VERBATIM)
        endif()
        list(APPEND TIDY_FIX_STAMP_FILES ${FIX_STAMP_FILE})
      endforeach()

      message(STATUS "Creating clang-tidy target '${CLANG_TIDY_TARGET_NAME}' for checking (parallel per TU).")
      add_custom_target(
        ${CLANG_TIDY_TARGET_NAME}
        DEPENDS ${TIDY_STAMP_FILES}
        COMMENT "Aggregated clang-tidy (check) for ${TIDY_OPERATION_SCOPE}"
        VERBATIM)

      message(STATUS "Creating clang-tidy target '${CLANG_TIDY_FIX_TARGET_NAME}' for applying fixes (parallel per TU).")
      add_custom_target(
        ${CLANG_TIDY_FIX_TARGET_NAME}
        DEPENDS ${TIDY_FIX_STAMP_FILES}
        COMMENT "Aggregated clang-tidy (apply fixes) for ${TIDY_OPERATION_SCOPE}"
        VERBATIM)
    endif()
  else()
    message(STATUS "Clang-tidy integration is disabled (ENABLE_CLANG_TIDY=OFF).")
  endif()
endfunction()

option(${PROJECT_NAME}_BUILD_TESTS "Build tests for clang-tidy" OFF)
if(${PROJECT_NAME}_BUILD_TESTS)
  include(CTest)
  enable_testing()
  add_subdirectory(tests)

  # --- Finalize and Create Tidy Targets ---
  # This call MUST be at the end, after all targets and add_subdirectory calls.
  finalize_clang_tidy_targets()
endif()

set(CLANG_TIDY_CMAKE_INSTALL
    "OFF"
    CACHE BOOL "Install clang-tidy.cmake")
if(CLANG_TIDY_CMAKE_INSTALL)
  if(NOT target_install_package_FOUND)
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
    ${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt
    RUNTIME_COMPONENT
    "ClangTidy"
    DEVELOPMENT_COMPONENT
    "ClangTidy")
endif()
