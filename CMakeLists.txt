cmake_minimum_required(VERSION 3.12)
project(
    kassert
    DESCRIPTION "Assertion library for KaMPIng"
    LANGUAGES CXX
)

# include guard to prevent duplicate targets when including this project as a subdirectory
if (TARGET kassert)
    return()
endif ()

# dependencies
include(FetchContent)
if (POLICY CMP0135)
    cmake_policy(SET CMP0135 OLD)
endif()
set(fetch_content_deps googletest)

set(fetch_content_supported_flags "")
if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
    list(APPEND fetch_content_supported_flags OVERRIDE_FIND_PACKAGE)
endif ()
if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.25)
    list(APPEND fetch_content_supported_flags SYSTEM)
endif ()
if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.28)
    list(APPEND fetch_content_supported_flags EXCLUDE_FROM_ALL)
endif ()

# Using CMake >= 3.24 we override find_package and it "just works".
# For older versions we simulate the same behavior by first making the dependency
# available and then writing an empty `googletest-config.cmake` such that
# find_package does not fail.
#
# This does not allow for easy overriding of dependencies in top-level projects
# because then the toplevel also has to override the config file, but this is
# the price to pay for using old CMake versions.
#
# When using CMake >= 3.24 a library user can override the depencies by
# providing an earlier FetchContent_Declare declaration with the `OVERRIDE_FIND_PACKAGE` flag enabled.
# Only the first call to FetchContent_Declare for each dependency will be registered.

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
    ${fetch_content_supported_flags}
)

function(ensure_googletest)
    if (CMAKE_VERSION VERSION_LESS 3.24)
        if(NOT googletest_POPULATED)
            FetchContent_MakeAvailable(googletest)
            set(googletest_DIR ${CMAKE_BINARY_DIR}/my_pkgRedirects/ PARENT_SCOPE)
            file(WRITE ${CMAKE_BINARY_DIR}/my_pkgRedirects/googletest-config.cmake "")
        endif()
    endif()
endfunction()


if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    # folder support for IDEs
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)

    # this has to be enabled in the main CMakeLists file
    include(CTest)

    # find Doxygen
    find_package(Doxygen)
    if (DOXYGEN_FOUND)
        if (DOXYGEN_VERSION VERSION_LESS "1.9.2")
            message(
                WARNING
                    "Doxygen must be version 1.9.2 or newer. Documentation may not be displayed correctly and CI may not pass even if checks pass locally."
            )
        endif ()
        add_custom_target(
            docs
            COMMAND ${DOXYGEN_EXECUTABLE} ${PROJECT_SOURCE_DIR}/docs/Doxyfile
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            COMMENT "Generating Documentation"
            VERBATIM
        )
    else ()
        message(STATUS "Doxygen not found, not building docs")
    endif ()
endif ()

# Require out-of-source builds
file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)
if (EXISTS "${LOC_PATH}")
    message(
        FATAL_ERROR
            "You cannot build in a source directory (or any directory with a CMakeLists.txt file). Please make a build subdirectory. Feel free to remove CMakeCache.txt and CMakeFiles."
    )
endif ()

option(KASSERT_WARNINGS_ARE_ERRORS OFF)
option(KASSERT_BUILD_TESTS OFF)

add_subdirectory(extern)

# Dummy target without any compile definitions -- used for unit tests only
add_library(kassert_base INTERFACE)
target_include_directories(kassert_base INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>")

# set C++ standard to C++17
target_compile_features(kassert_base INTERFACE cxx_std_17)
add_library(kassert_warnings INTERFACE)
target_compile_options(kassert_warnings INTERFACE
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

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    target_compile_options(kassert_warnings INTERFACE
        "-Wcast-align"
        "-Wnull-dereference"
        "-Wpedantic"
        "-Wextra-semi"
        "-Wno-gnu-zero-variadic-macro-arguments"
    )
endif ()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_compile_options(kassert_warnings INTERFACE
        "-Wcast-align"
        "-Wnull-dereference"
        "-Wpedantic"
        "-Wnoexcept"
        "-Wsuggest-attribute=const"
        "-Wsuggest-attribute=noreturn"
        "-Wsuggest-override"
    )
endif ()

# OFF by default.
if (KASSERT_WARNINGS_ARE_ERRORS)
    target_compile_options(kassert_warnings INTERFACE "-Werror")
endif ()

# Actual library target with compile definitions
add_library(kassert INTERFACE)
target_link_libraries(kassert INTERFACE kassert_base)

if (NOT DEFINED KASSERT_EXCEPTION_MODE OR KASSERT_EXCEPTION_MODE EQUAL 1)
    message(STATUS "Exception mode enabled.")
    target_compile_definitions(kassert INTERFACE -DKASSERT_EXCEPTION_MODE)
else ()
    message(STATUS "Exception mode disabled.")
endif ()

# The assertion level controls which assertions are enabled during runtime. Assertion levels can be set explicitly using
# the -DKASSERT_ASSERTION_LEVEL=... flag.
if (NOT DEFINED KASSERT_ASSERTION_LEVEL)
    message(WARNING "Assertion level no set. Default to level 0.")
    set(KASSERT_ASSERTION_LEVEL 0)
else ()
    message(STATUS "Assertion level set to ${KASSERT_ASSERTION_LEVEL}.")
endif ()
target_compile_definitions(kassert INTERFACE -DKASSERT_ASSERTION_LEVEL=${KASSERT_ASSERTION_LEVEL})

add_library(kassert::kassert ALIAS kassert)

# Testing and examples are only built if this is the main project or if KASSERT_BUILD_TESTS is set (OFF by default)
if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME OR KASSERT_BUILD_TESTS)
    ensure_googletest()
    add_subdirectory(tests)
endif ()
