cmake_minimum_required(VERSION 3.12)
project(
  kassert
  DESCRIPTION "Assertion library for KaMPI.ng"
  LANGUAGES CXX
)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  # folder support for IDEs
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)

  # this has to be enabled in the main CMakeLists file
  include(CTest)

  # find Doxygen
  find_package(Doxygen)
  if(DOXYGEN_FOUND)
    if (DOXYGEN_VERSION VERSION_LESS "1.9.2")
      message(WARNING "Doxygen must be version 1.9.2 or newer. Documentation may not be displayed correctly and CI may not pass even if checks pass locally.")
    endif()
    add_custom_target(docs
      COMMAND ${DOXYGEN_EXECUTABLE} ${PROJECT_SOURCE_DIR}/docs/Doxyfile
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
      COMMENT "Generating Documentation"
      VERBATIM)
  endif()
else()
  message(STATUS "Doxygen not found, not building docs")
endif()

# Require out-of-source builds
file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)
if(EXISTS "${LOC_PATH}")
  message(FATAL_ERROR "You cannot build in a source directory (or any directory with a CMakeLists.txt file). Please make a build subdirectory. Feel free to remove CMakeCache.txt and CMakeFiles.")
endif()

option(KASSERT_WARNINGS_ARE_ERRORS OFF)
option(KASSERT_BUILD_TESTS OFF)

add_subdirectory(extern)

# Dummy target without any compile definitions -- used for unit tests only
add_library(kassert_base INTERFACE)
target_include_directories(kassert_base INTERFACE include)

# set C++ standard to C++17
target_compile_features(kassert_base INTERFACE cxx_std_17)

list(
  APPEND KASSERT_WARNING_FLAGS
  "-Wall"
  "-Wextra"
  "-Wconversion"
  "-Wnon-virtual-dtor"
  "-Woverloaded-virtual"
  "-Wshadow"
  "-Wsign-conversion"
  "-Wundef"
  "-Wunreachable-code"
  "-Wunused"
)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  list(
    APPEND KASSERT_WARNING_FLAGS
    "-Wcast-align"
    "-Wnull-dereference"
    "-Wpedantic"
    "-Wextra-semi"
    "-Wno-gnu-zero-variadic-macro-arguments"
  )
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  list(
    APPEND KASSERT_WARNING_FLAGS
    "-Wcast-align"
    "-Wnull-dereference"
    "-Wpedantic"
    "-Wnoexcept"
    "-Wsuggest-attribute=const"
    "-Wsuggest-attribute=noreturn"
    "-Wsuggest-override"
  )
endif()

# OFF by default.
if(KASSERT_WARNINGS_ARE_ERRORS)
  list(
    APPEND KASSERT_WARNING_FLAGS
    "-Werror"
  )
endif()

# Actual library target with compile definitions
add_library(kassert INTERFACE)
target_link_libraries(kassert INTERFACE kassert_base)

if (NOT DEFINED KASSERT_EXCEPTION_MODE OR KASSERT_EXCEPTION_MODE EQUAL 1)
  message(STATUS "Exception mode enabled.")
  target_compile_definitions(kassert INTERFACE -DKASSERT_EXCEPTION_MODE)
else ()
  message(STATUS "Exception mode disabled.")
endif()

# The assertion level controls which assertions are enabled during runtime.
# Assertion levels can be set explicitly using the -DKASSERT_ASSERTION_LEVEL=... flag. 
if (NOT DEFINED KASSERT_ASSERTION_LEVEL) 
  message(WARNING "Assertion level no set. Default to level 0.")
  set(KASSERT_ASSERTION_LEVEL 0)
else ()
  message(STATUS "Assertion level set to ${KASSERT_ASSERTION_LEVEL}.")
endif()
target_compile_definitions(kassert INTERFACE -DKASSERT_ASSERTION_LEVEL=${KASSERT_ASSERTION_LEVEL})

add_library(kassert::kassert ALIAS kassert)

# Testing and examples are only built if this is the main project
# or if KASSERT_BUILD_TESTS is set (OFF by default)
if(
  CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME OR KASSERT_BUILD_TESTS
)
  add_subdirectory(tests)
endif()
