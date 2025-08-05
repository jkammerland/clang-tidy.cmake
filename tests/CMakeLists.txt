# Create test executables separately
add_executable(test_main test_main.cpp test_common.cpp test1.cpp test2.cpp)
target_tidy_sources(test_main)

add_executable(test_header_check test_header_check.cpp test_header.h)
target_tidy_sources(test_header_check)

# Add consumer test that runs after install
if(${PROJECT_NAME}_BUILD_TESTS)
  add_test(
    NAME install_for_consumer
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target install
  )
  set_tests_properties(install_for_consumer PROPERTIES 
    FIXTURES_SETUP install_fixture
  )
  
  add_test(
    NAME consumer_test
    COMMAND ${CMAKE_COMMAND} 
      -S ${CMAKE_CURRENT_SOURCE_DIR}/consumer
      -B ${CMAKE_CURRENT_BINARY_DIR}/consumer/build
      -DCMAKE_PREFIX_PATH=${CMAKE_INSTALL_PREFIX}
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
  )
  set_tests_properties(consumer_test PROPERTIES 
    FIXTURES_REQUIRED install_fixture
  )
  
  add_test(
    NAME consumer_build_test
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR}/consumer/build
  )
  set_tests_properties(consumer_build_test PROPERTIES 
    DEPENDS consumer_test
  )
  
  add_test(
    NAME consumer_tidy_test
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR}/consumer/build --target tidy
  )
  set_tests_properties(consumer_tidy_test PROPERTIES 
    DEPENDS consumer_build_test
  )
endif()
