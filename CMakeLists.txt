cmake_minimum_required(VERSION 3.25)
project(clang-tidy.cmake VERSION 1.1.0)

if(NOT COMMAND list_file_include_guard)
  find_package(target_install_package)

  if(NOT target_install_package_FOUND)
    include(list_file_include_guard.cmake)
    include(project_include_guard.cmake)
    include(project_log.cmake)
  else()
    project_log(VERBOSE "target_install_package found, using project_include_guard from target_install_package")
  endif()
endif()

project_include_guard()

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/.clang-tidy)
  project_log(VERBOSE "No .clang-tidy file found in project root, you can use this for reference:")
  project_log(VERBOSE "--> ${CMAKE_CURRENT_LIST_DIR}/.clang-tidy")
endif()

# --- Define global properties for collected sources ---
define_property(GLOBAL PROPERTY PROJECT_TRANSLATION_UNITS_FOR_TIDY
    BRIEF_DOCS "Collected translation units for clang-tidy"
    FULL_DOCS "List of all translation units registered for clang-tidy processing")

define_property(GLOBAL PROPERTY PROJECT_HEADERS_FOR_TIDY
    BRIEF_DOCS "Collected headers for clang-tidy"
    FULL_DOCS "List of all headers registered for clang-tidy (collected but not directly processed)")

define_property(GLOBAL PROPERTY PROJECT_TARGETS_FOR_TIDY
    BRIEF_DOCS "List of targets registered for clang-tidy"
    FULL_DOCS "List of all targets that have been registered with register_project_sources()")

define_property(GLOBAL PROPERTY PROJECT_FINALIZED_TARGETS_FOR_TIDY
    BRIEF_DOCS "List of finalized targets for clang-tidy"
    FULL_DOCS "List of targets that have had their clang-tidy targets created")

define_property(GLOBAL PROPERTY CLANG_TIDY_COMMON_SETUP_DONE
    BRIEF_DOCS "Flag indicating if common clang-tidy setup has been done"
    FULL_DOCS "Boolean flag to prevent duplicate clang-tidy configuration")

# Initialize properties
set_property(GLOBAL PROPERTY PROJECT_TRANSLATION_UNITS_FOR_TIDY "")
set_property(GLOBAL PROPERTY PROJECT_HEADERS_FOR_TIDY "")
set_property(GLOBAL PROPERTY PROJECT_TARGETS_FOR_TIDY "")
set_property(GLOBAL PROPERTY PROJECT_FINALIZED_TARGETS_FOR_TIDY "")
set_property(GLOBAL PROPERTY CLANG_TIDY_COMMON_SETUP_DONE OFF)

if(NOT CMAKE_EXPORT_COMPILE_COMMANDS)
  project_log(FATAL_ERROR "Compilation database is required by clang-tidy, set with -DCMAKE_EXPORT_COMPILE_COMMANDS=ON")
endif()

# --- Will control multi-threading of clang-tidy too ---
set(TIDY_USES_TERMINAL OFF CACHE BOOL "Serializes output with colors, also makes it single threaded")
set(TIDY_SINGLE_THREADED OFF CACHE BOOL "Single threaded clang-tidy")
project_log(DEBUG "TIDY_USES_TERMINAL: ${TIDY_USES_TERMINAL}")
project_log(DEBUG "TIDY_SINGLE_THREADED: ${TIDY_SINGLE_THREADED}, otherwise parallel per translation unit.")

# Alias for the old function, for backwards compatibility
function(target_tidy_sources TARGET_NAME)
  register_project_sources(${TARGET_NAME})
endfunction(target_tidy_sources)

function(register_project_sources TARGET_NAME)
  # Check if target was already registered
  get_property(REGISTERED_TARGETS GLOBAL PROPERTY PROJECT_TARGETS_FOR_TIDY)
  if(${TARGET_NAME} IN_LIST REGISTERED_TARGETS)
    project_log(WARNING "Target ${TARGET_NAME} was already registered for clang-tidy")
    return()
  endif()

  get_target_property(TARGET_SOURCES ${TARGET_NAME} SOURCES)
  if(NOT TARGET_SOURCES)
    project_log(WARNING "Target ${TARGET_NAME} has no sources or does not exist when registering for tidy.")
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
        project_log(WARNING "Could not get SOURCE_DIR for target ${TARGET_NAME}. Using CMAKE_CURRENT_SOURCE_DIR (${CMAKE_CURRENT_SOURCE_DIR}) for resolving relative paths.")
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
      project_log(WARNING "Tidy: File from target ${TARGET_NAME} not found: ${ABS_FILE_PATH} (original: ${FILE_PATH_RELATIVE} in ${TARGET_SOURCE_DIR})")
    endif()
  endforeach()

  # Add target to the list of registered targets
  get_property(TEMP_TARGETS GLOBAL PROPERTY PROJECT_TARGETS_FOR_TIDY)
  list(APPEND TEMP_TARGETS ${TARGET_NAME})
  set_property(GLOBAL PROPERTY PROJECT_TARGETS_FOR_TIDY ${TEMP_TARGETS})

  # Store per-target source lists
  if(LOCAL_TUS_FOR_TARGET)
    define_property(GLOBAL PROPERTY PROJECT_TUS_FOR_TIDY_${TARGET_NAME})
    set_property(GLOBAL PROPERTY PROJECT_TUS_FOR_TIDY_${TARGET_NAME} ${LOCAL_TUS_FOR_TARGET})
    
    # Also add to global list for backward compatibility
    get_property(TEMP_TUS GLOBAL PROPERTY PROJECT_TRANSLATION_UNITS_FOR_TIDY)
    list(APPEND TEMP_TUS ${LOCAL_TUS_FOR_TARGET})
    list(REMOVE_DUPLICATES TEMP_TUS)
    set_property(GLOBAL PROPERTY PROJECT_TRANSLATION_UNITS_FOR_TIDY ${TEMP_TUS})
  endif()

  if(LOCAL_HEADERS_FOR_TARGET)
    define_property(GLOBAL PROPERTY PROJECT_HEADERS_FOR_TIDY_${TARGET_NAME})
    set_property(GLOBAL PROPERTY PROJECT_HEADERS_FOR_TIDY_${TARGET_NAME} ${LOCAL_HEADERS_FOR_TARGET})
    
    # Also add to global list for backward compatibility
    get_property(TEMP_HEADERS GLOBAL PROPERTY PROJECT_HEADERS_FOR_TIDY)
    list(APPEND TEMP_HEADERS ${LOCAL_HEADERS_FOR_TARGET})
    list(REMOVE_DUPLICATES TEMP_HEADERS)
    set_property(GLOBAL PROPERTY PROJECT_HEADERS_FOR_TIDY ${TEMP_HEADERS})
  endif()
endfunction()

# Helper function to setup common clang-tidy configuration
function(_setup_clang_tidy_common)
  get_property(SETUP_DONE GLOBAL PROPERTY CLANG_TIDY_COMMON_SETUP_DONE)
  if(SETUP_DONE)
    return()
  endif()

  option(ENABLE_CLANG_TIDY "Enable clang-tidy integration" ON)
  option(CLANG_TIDY_APPLY_FIXES_BY_DEFAULT "Hint that clang-tidy should apply fixes (use clang-tidy-fix target)" OFF)
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

  if(NOT ENABLE_CLANG_TIDY)
    project_log(STATUS "Clang-tidy integration is disabled (ENABLE_CLANG_TIDY=OFF).")
    return()
  endif()

  set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # Crucial for clang-tidy

  find_program(
    CLANG_TIDY_EXE
    NAMES clang-tidy clang-tidy-20 clang-tidy-19 clang-tidy-18 clang-tidy-17
    DOC "Path to clang-tidy executable" REQUIRED)
  project_log(STATUS "Found clang-tidy: ${CLANG_TIDY_EXE}")

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
    project_log(STATUS "Using .clang-tidy configuration file from project root: ${CMAKE_SOURCE_DIR}/.clang-tidy")
    if(CLANG_TIDY_HEADER_FILTER AND NOT CLANG_TIDY_HEADER_FILTER STREQUAL "")
      project_log(STATUS "  NOTE: CLANG_TIDY_HEADER_FILTER='${CLANG_TIDY_HEADER_FILTER}' overrides HeaderFilterRegex in .clang-tidy")
    endif()
    if(CLANG_TIDY_HEADER_EXCLUDE AND NOT CLANG_TIDY_HEADER_EXCLUDE STREQUAL "")
      project_log(STATUS "  NOTE: CLANG_TIDY_HEADER_EXCLUDE='${CLANG_TIDY_HEADER_EXCLUDE}' overrides ExcludeHeaderFilterRegex in .clang-tidy")
    endif()
  else()
    project_log(WARNING "No .clang-tidy file found in project root (${CMAKE_SOURCE_DIR}/.clang-tidy). Clang-tidy will use its default checks. It is highly recommended to create a .clang-tidy file.")
  endif()

  set(TIDY_CHECK_ARGS ${TIDY_COMMON_ARGS})
  set(TIDY_FIX_ARGS ${TIDY_COMMON_ARGS})
  list(APPEND TIDY_FIX_ARGS "--fix")

  # Export variables to parent scope
  set(CLANG_TIDY_EXE ${CLANG_TIDY_EXE} PARENT_SCOPE)
  set(TIDY_CHECK_ARGS ${TIDY_CHECK_ARGS} PARENT_SCOPE)
  set(TIDY_FIX_ARGS ${TIDY_FIX_ARGS} PARENT_SCOPE)
  set(ENABLE_CLANG_TIDY ${ENABLE_CLANG_TIDY} PARENT_SCOPE)

  set_property(GLOBAL PROPERTY CLANG_TIDY_COMMON_SETUP_DONE ON)
endfunction()

# Helper function to create a custom command for tidying a single file
function(_create_tidy_custom_command TU_FILE STAMP_FILE TIDY_ARGS MODE)
  set(COMPILE_COMMANDS_JSON "${CMAKE_BINARY_DIR}/compile_commands.json")
  
  if(MODE STREQUAL "fix")
    set(MODE_TEXT "fix")
  else()
    set(MODE_TEXT "check")
  endif()

  if(TIDY_USES_TERMINAL OR TIDY_SINGLE_THREADED)
    add_custom_command(
      OUTPUT ${STAMP_FILE}
      COMMAND ${CMAKE_COMMAND} -E echo "Tidying (${MODE_TEXT}): ${TU_FILE}"
      COMMAND ${CLANG_TIDY_EXE} ${TIDY_ARGS} "${TU_FILE}"
      COMMAND ${CMAKE_COMMAND} -E touch ${STAMP_FILE}
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      DEPENDS ${COMPILE_COMMANDS_JSON} ${TU_FILE}
      COMMENT "Running clang-tidy (${MODE_TEXT}) on ${TU_FILE}"
      VERBATIM USES_TERMINAL)
  else()
    add_custom_command(
      OUTPUT ${STAMP_FILE}
      COMMAND ${CMAKE_COMMAND} -E echo "Tidying (${MODE_TEXT}): ${TU_FILE}"
      COMMAND ${CLANG_TIDY_EXE} ${TIDY_ARGS} "${TU_FILE}"
      COMMAND ${CMAKE_COMMAND} -E touch ${STAMP_FILE}
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      DEPENDS ${COMPILE_COMMANDS_JSON} ${TU_FILE}
      COMMENT "Running clang-tidy (${MODE_TEXT}) on ${TU_FILE}"
      VERBATIM)
  endif()
endfunction()

# Helper function to create tidy targets for a list of files
function(_create_tidy_targets_for_files FILES_TO_PROCESS TARGET_NAME FIX_TARGET_NAME OPERATION_SCOPE)
  if(NOT FILES_TO_PROCESS)
    project_log(WARNING "No files available for clang-tidy for ${OPERATION_SCOPE}. Tidy targets will do nothing.")
    add_custom_target(${TARGET_NAME} COMMENT "No files to process with clang-tidy for ${OPERATION_SCOPE}.")
    add_custom_target(${FIX_TARGET_NAME} COMMENT "No files to process with clang-tidy (fix mode) for ${OPERATION_SCOPE}.")
    return()
  endif()

  set(TIDY_STAMP_FILES "")
  set(TIDY_FIX_STAMP_FILES "")
  set(STAMP_DIR "${CMAKE_BINARY_DIR}/tidy_stamps")
  file(MAKE_DIRECTORY ${STAMP_DIR})

  foreach(TU_FILE ${FILES_TO_PROCESS})
    # Create a unique but predictable name for the stamp file based on the TU path
    cmake_path(GET TU_FILE FILENAME TU_FILENAME)
    cmake_path(GET TU_FILE PARENT_PATH TU_PARENT)
    cmake_path(HASH TU_PARENT TU_PARENT_HASH)
    # Combine filename and parent hash for unique identifier
    set(SAFE_TU_IDENTIFIER "${TU_FILENAME}_${TU_PARENT_HASH}")
    string(MAKE_C_IDENTIFIER "${SAFE_TU_IDENTIFIER}" SAFE_TU_IDENTIFIER)

    # Add target name to stamp file to avoid conflicts between different targets
    string(MAKE_C_IDENTIFIER "${TARGET_NAME}" SAFE_TARGET_NAME)
    set(CHECK_STAMP_FILE "${STAMP_DIR}/${SAFE_TARGET_NAME}_${SAFE_TU_IDENTIFIER}.checked")
    set(FIX_STAMP_FILE "${STAMP_DIR}/${SAFE_TARGET_NAME}_${SAFE_TU_IDENTIFIER}.fixed")

    _create_tidy_custom_command(${TU_FILE} ${CHECK_STAMP_FILE} "${TIDY_CHECK_ARGS}" "check")
    list(APPEND TIDY_STAMP_FILES ${CHECK_STAMP_FILE})

    _create_tidy_custom_command(${TU_FILE} ${FIX_STAMP_FILE} "${TIDY_FIX_ARGS}" "fix")
    list(APPEND TIDY_FIX_STAMP_FILES ${FIX_STAMP_FILE})
  endforeach()

  project_log(VERBOSE "Creating clang-tidy target '${TARGET_NAME}' for checking ${OPERATION_SCOPE}.")
  add_custom_target(
    ${TARGET_NAME}
    DEPENDS ${TIDY_STAMP_FILES}
    COMMENT "Aggregated clang-tidy (check) for ${OPERATION_SCOPE}"
    VERBATIM)

  project_log(VERBOSE "Creating clang-tidy target '${FIX_TARGET_NAME}' for applying fixes to ${OPERATION_SCOPE}.")
  add_custom_target(
    ${FIX_TARGET_NAME}
    DEPENDS ${TIDY_FIX_STAMP_FILES}
    COMMENT "Aggregated clang-tidy (apply fixes) for ${OPERATION_SCOPE}"
    VERBATIM)
endfunction()

# Alias function for backward compatibility
function(finalize_tidy_target TARGET_NAME)
  finalize_clang_tidy_target(${TARGET_NAME})
endfunction()

# Function to finalize clang-tidy targets for a specific target
function(finalize_clang_tidy_target TARGET_NAME)
  # Setup common clang-tidy configuration if not already done
  _setup_clang_tidy_common()
  
  if(NOT ENABLE_CLANG_TIDY)
    return()
  endif()

  # Check if target is already finalized
  get_property(FINALIZED_TARGETS GLOBAL PROPERTY PROJECT_FINALIZED_TARGETS_FOR_TIDY)
  list(FIND FINALIZED_TARGETS ${TARGET_NAME} ALREADY_FINALIZED)
  if(NOT ALREADY_FINALIZED EQUAL -1)
    project_log(STATUS "Target ${TARGET_NAME} already has clang-tidy targets finalized")
    return()
  endif()

  # Check if target is registered
  get_property(REGISTERED_TARGETS GLOBAL PROPERTY PROJECT_TARGETS_FOR_TIDY)
  list(FIND REGISTERED_TARGETS ${TARGET_NAME} TARGET_INDEX)
  if(TARGET_INDEX EQUAL -1)
    project_log(WARNING "Target ${TARGET_NAME} was not registered with register_project_sources()")
    return()
  endif()

  # Get target-specific translation units
  get_property(TARGET_TUS GLOBAL PROPERTY PROJECT_TUS_FOR_TIDY_${TARGET_NAME})
  if(NOT TARGET_TUS)
    set(TARGET_TUS "")
  endif()

  # Handle CLANG_TIDY_FILE option for specific file filtering
  set(FILES_TO_PROCESS ${TARGET_TUS})
  set(OPERATION_SCOPE "target ${TARGET_NAME}")
  
  if(CLANG_TIDY_FILE AND NOT CLANG_TIDY_FILE STREQUAL "")
    if(IS_ABSOLUTE "${CLANG_TIDY_FILE}")
      set(SPECIFIC_FILE "${CLANG_TIDY_FILE}")
    else()
      set(SPECIFIC_FILE "${CMAKE_SOURCE_DIR}/${CLANG_TIDY_FILE}")
    endif()

    if(EXISTS "${SPECIFIC_FILE}")
      # Check if the specific file is part of this target
      list(FIND TARGET_TUS "${SPECIFIC_FILE}" FILE_INDEX)
      if(NOT FILE_INDEX EQUAL -1)
        set(FILES_TO_PROCESS "${SPECIFIC_FILE}")
        set(OPERATION_SCOPE "specified file from target ${TARGET_NAME}: ${CLANG_TIDY_FILE}")
      else()
        # File not in this target, no files to process
        set(FILES_TO_PROCESS "")
        project_log(STATUS "CLANG_TIDY_FILE '${CLANG_TIDY_FILE}' is not part of target ${TARGET_NAME}")
      endif()
    else()
      project_log(WARNING "CLANG_TIDY_FILE specified ('${CLANG_TIDY_FILE}'), but not found at '${SPECIFIC_FILE}'.")
    endif()
  endif()

  # Create target-specific tidy targets
  set(TARGET_TIDY_NAME "tidy-${TARGET_NAME}")
  set(TARGET_TIDY_FIX_NAME "tidy-${TARGET_NAME}-fix")
  
  _create_tidy_targets_for_files("${FILES_TO_PROCESS}" ${TARGET_TIDY_NAME} ${TARGET_TIDY_FIX_NAME} "${OPERATION_SCOPE}")

  # Mark target as finalized
  get_property(TEMP_FINALIZED GLOBAL PROPERTY PROJECT_FINALIZED_TARGETS_FOR_TIDY)
  list(APPEND TEMP_FINALIZED ${TARGET_NAME})
  set_property(GLOBAL PROPERTY PROJECT_FINALIZED_TARGETS_FOR_TIDY ${TEMP_FINALIZED})
endfunction()

# Alias function for backward compatibility
function(finalize_tidy_targets)
  finalize_clang_tidy_targets()
endfunction(finalize_tidy_targets)

function(finalize_clang_tidy_targets)
  # Properties are always defined, so no need to check

  # Setup common clang-tidy configuration
  _setup_clang_tidy_common()
  
  if(NOT ENABLE_CLANG_TIDY)
    return()
  endif()

  # Create global tidy targets using all collected files
  get_property(CURRENT_FILES_TO_PROCESS GLOBAL PROPERTY PROJECT_TRANSLATION_UNITS_FOR_TIDY)
  set(TIDY_OPERATION_SCOPE "all registered project translation units")

  # Handle CLANG_TIDY_FILE option for global targets
  if(CLANG_TIDY_FILE AND NOT CLANG_TIDY_FILE STREQUAL "")
    if(IS_ABSOLUTE "${CLANG_TIDY_FILE}")
      set(SPECIFIC_FILE "${CLANG_TIDY_FILE}")
    else()
      set(SPECIFIC_FILE "${CMAKE_SOURCE_DIR}/${CLANG_TIDY_FILE}")
    endif()

    if(EXISTS "${SPECIFIC_FILE}")
      set(CURRENT_FILES_TO_PROCESS "${SPECIFIC_FILE}")
      set(TIDY_OPERATION_SCOPE "specified file: ${CLANG_TIDY_FILE}")
      project_log(STATUS "Clang-tidy targets will run on specific file: ${SPECIFIC_FILE}")
    else()
      project_log(WARNING "CLANG_TIDY_FILE specified ('${CLANG_TIDY_FILE}'), but not found at '${SPECIFIC_FILE}'. Running on ${TIDY_OPERATION_SCOPE}.")
    endif()
  endif()

  # Create the global tidy targets
  _create_tidy_targets_for_files("${CURRENT_FILES_TO_PROCESS}" ${CLANG_TIDY_TARGET_NAME} ${CLANG_TIDY_FIX_TARGET_NAME} "${TIDY_OPERATION_SCOPE}")

  # Finalize any registered targets that haven't been explicitly finalized
  get_property(REGISTERED_TARGETS GLOBAL PROPERTY PROJECT_TARGETS_FOR_TIDY)
  get_property(FINALIZED_TARGETS GLOBAL PROPERTY PROJECT_FINALIZED_TARGETS_FOR_TIDY)
  foreach(TARGET_NAME ${REGISTERED_TARGETS})
    list(FIND FINALIZED_TARGETS ${TARGET_NAME} IS_FINALIZED)
    if(IS_FINALIZED EQUAL -1)
      project_log(DEBUG "Auto-finalizing clang-tidy targets for '${TARGET_NAME}'")
      finalize_tidy_target(${TARGET_NAME})
    endif()
  endforeach()
endfunction()

set(CLANG_TIDY_CMAKE_INSTALL OFF CACHE BOOL "Install clang-tidy.cmake")
set(${PROJECT_NAME}_BUILD_TESTS OFF CACHE BOOL "Build tests for clang-tidy")

if(CLANG_TIDY_CMAKE_INSTALL OR ${PROJECT_NAME}_BUILD_TESTS)
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

if(${PROJECT_NAME}_BUILD_TESTS)
  include(CTest)
  enable_testing()
  add_subdirectory(tests)

  # --- Finalize and Create Tidy Targets ---
  # This call MUST be at the end, after all targets and add_subdirectory calls.
  finalize_tidy_targets()
endif()
