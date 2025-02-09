cmake_minimum_required(VERSION 3.14)
project(
    kassert
    DESCRIPTION "Assertion library for KaMPIng"
    LANGUAGES CXX
    VERSION 0.2.0
)


# include guard to prevent duplicate targets when including this project as a subdirectory
if (TARGET kassert)
    return()
endif()
if (CMAKE_VERSION VERSION_LESS 3.21)
    if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
        set(PROJECT_IS_TOP_LEVEL TRUE)
    else ()
        set(PROJECT_IS_TOP_LEVEL FALSE)
    endif ()
endif ()
option(KASSERT_BUILD_TESTS "Build the tests." ${PROJECT_IS_TOP_LEVEL})

# dependencies
include(FetchContent)

if (PROJECT_IS_TOP_LEVEL)
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

add_library(kassert INTERFACE)
target_include_directories(kassert INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

# set C++ standard to C++17
target_compile_features(kassert INTERFACE cxx_std_17)

add_library(kassert_warnings INTERFACE)
target_compile_options(
    kassert_warnings
    INTERFACE "-Wall"
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
    target_compile_options(
        kassert_warnings INTERFACE "-Wcast-align" "-Wnull-dereference" "-Wpedantic" "-Wextra-semi"
                                   "-Wno-gnu-zero-variadic-macro-arguments"
    )
endif ()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_compile_options(
        kassert_warnings
        INTERFACE "-Wcast-align"
                  "-Wnull-dereference"
                  "-Wpedantic"
                  "-Wnoexcept"
                  "-Wsuggest-attribute=const"
                  "-Wsuggest-attribute=noreturn"
                  "-Wsuggest-override"
    )
endif ()

# THROWING_KASSERT is able to throw an exception if an assertion fails. If KASSERT_EXCEPTION_MODE is defined, than an
# exception will be thrown upon failure, otherwise it behaves like regular KASSERT. You can either manually #define it,
# or by setting the KASSERT_EXCEPTION_MODE target property to ON on the target which links to kassert.
set(exception_mode_enabled "$<BOOL:$<TARGET_PROPERTY:KASSERT_EXCEPTION_MODE>>")
target_compile_definitions(kassert INTERFACE "$<${exception_mode_enabled}:-DKASSERT_EXCEPTION_MODE>")

# The assertion level controls which assertions are enabled during runtime. Assertion levels can be set explicitly using
# the -DKASSERT_ASSERTION_LEVEL=... flag. You can also set the KASSERT_ASSERTION_LEVEL target property to the desired
# level on the target which links to kassert. This will properly #define the assertion level for the target.
set(assertion_level_set "$<BOOL:$<TARGET_PROPERTY:KASSERT_ASSERTION_LEVEL>>")
set(assertion_level "$<TARGET_PROPERTY:KASSERT_ASSERTION_LEVEL>")
target_compile_definitions(kassert INTERFACE "$<${assertion_level_set}:-DKASSERT_ASSERTION_LEVEL=${assertion_level}>")

add_library(kassert::kassert ALIAS kassert)

# Tests are only built if KASSERT_BUILD_TESTS is set (OFF by default, ON if this is the top-level project)
if (KASSERT_BUILD_TESTS)
    # this has to be enabled in the main CMakeLists file
    include(CTest)
    add_subdirectory(tests)
endif ()

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/kassertConfigVersion.cmake
    VERSION ${kassert_PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion ARCH_INDEPENDENT # requires CMake 3.14
)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/kassertConfig.cmake ${CMAKE_CURRENT_BINARY_DIR}/kassertConfig.cmake COPYONLY
)

include(GNUInstallDirs)
install(
    TARGETS kassert
    EXPORT kassertTargets
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(
    DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
    PATTERN "*.hpp"
)
install(
    EXPORT kassertTargets
    FILE kassertTargets.cmake
    NAMESPACE kassert::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/kassert
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/kassertConfigVersion.cmake ${CMAKE_CURRENT_BINARY_DIR}/kassertConfig.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/kassert
)
