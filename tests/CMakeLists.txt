cmake_minimum_required(VERSION 3.25)
project(TidyTests_Subproject CXX)

file(GLOB TIDY_TESTS CONFIGURE_DEPENDS "*.cpp")
add_executable(tests ${TIDY_TESTS})
register_project_sources(tests)
