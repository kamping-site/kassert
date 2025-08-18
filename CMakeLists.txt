cmake_minimum_required(VERSION 3.14)
project(
    kassert
    DESCRIPTION "Assertion library for KaMPIng"
    LANGUAGES CXX
    VERSION 0.2.2
)
if (CMAKE_VERSION VERSION_LESS 3.21)
    if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
        set(PROJECT_IS_TOP_LEVEL TRUE)
    else ()
        set(PROJECT_IS_TOP_LEVEL FALSE)
    endif ()
endif ()

# This is the vendor ID, which is used to create the CMake namespace and the C++ namespace.
if(NOT DEFINED KASSERT_VENDOR_ID)
  set(KASSERT_VENDOR_ID "kassert")
endif()

# Derive others if not already defined
if (NOT DEFINED KASSERT_CMAKE_NAMESPACE)
    set(KASSERT_CMAKE_NAMESPACE "${KASSERT_VENDOR_ID}")
endif ()

if (NOT DEFINED KASSERT_CXX_NAMESPACE)
    set(KASSERT_CXX_NAMESPACE "${KASSERT_VENDOR_ID}")
endif ()

if (NOT DEFINED KASSERT_INCLUDE_SUBDIR)
    set(KASSERT_INCLUDE_SUBDIR "${KASSERT_VENDOR_ID}")
endif ()

if(NOT DEFINED KASSERT_PREFIX)
  string(TOUPPER ${KASSERT_CXX_NAMESPACE} KASSERT_PREFIX)
endif()

# include guard to prevent duplicate targets when including this project as a subdirectory
if (TARGET ${KASSERT_CMAKE_NAMESPACE}::kassert)
    return()
endif ()

option(KASSERT_BUILD_TESTS "Build the tests." ${PROJECT_IS_TOP_LEVEL})
option(KASSERT_BUILD_DOCS "Build the docs." ${PROJECT_IS_TOP_LEVEL})
option(KASSERT_USE_BUNDLED_GTEST "Use bundled googletest (downloaded at configure time via FetchContent)." ON)

if (KASSERT_BUILD_DOCS)
    add_subdirectory(docs)
endif ()

add_library(${KASSERT_CMAKE_NAMESPACE}_kassert INTERFACE)
set(kassert_sources kassert.hpp internal/logger.hpp internal/assertion_macros.hpp internal/expression_decomposition.hpp)

# generate the header files from the .in files
foreach (file ${kassert_sources})
    set(source_file include/kassert/${file}.in)
    set(target_file include/${KASSERT_INCLUDE_SUBDIR}/${file})
    configure_file(${source_file} ${target_file} @ONLY)
endforeach ()

target_include_directories(
    ${KASSERT_CMAKE_NAMESPACE}_kassert INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
)

# set C++ standard to C++17
target_compile_features(${KASSERT_CMAKE_NAMESPACE}_kassert INTERFACE cxx_std_17)

add_library(${KASSERT_CMAKE_NAMESPACE}_kassert_warnings INTERFACE)
target_compile_options(
    ${KASSERT_CMAKE_NAMESPACE}_kassert_warnings
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
        ${KASSERT_CMAKE_NAMESPACE}_kassert_warnings INTERFACE "-Wcast-align" "-Wnull-dereference" "-Wpedantic"
                                                              "-Wextra-semi" "-Wno-gnu-zero-variadic-macro-arguments"
    )
endif ()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_compile_options(
        ${KASSERT_CMAKE_NAMESPACE}_kassert_warnings
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
target_compile_definitions(
    ${KASSERT_CMAKE_NAMESPACE}_kassert INTERFACE "$<${exception_mode_enabled}:-DKASSERT_EXCEPTION_MODE>"
)

# The assertion level controls which assertions are enabled during runtime. Assertion levels can be set explicitly using
# the -DKASSERT_ASSERTION_LEVEL=... flag. You can also set the KASSERT_ASSERTION_LEVEL target property to the desired
# level on the target which links to kassert. This will properly #define the assertion level for the target.
set(assertion_level_set "$<BOOL:$<TARGET_PROPERTY:KASSERT_ASSERTION_LEVEL>>")
set(assertion_level "$<TARGET_PROPERTY:KASSERT_ASSERTION_LEVEL>")
target_compile_definitions(
    ${KASSERT_CMAKE_NAMESPACE}_kassert
    INTERFACE "$<${assertion_level_set}:-DKASSERT_ASSERTION_LEVEL=${assertion_level}>"
)

add_library(${KASSERT_CMAKE_NAMESPACE}::kassert ALIAS ${KASSERT_CMAKE_NAMESPACE}_kassert)

# Tests are only built if KASSERT_BUILD_TESTS is set (OFF by default, ON if this is the top-level project)
if (KASSERT_BUILD_TESTS)
    # this has to be enabled in the main CMakeLists file
    include(CTest)
    add_subdirectory(tests)
endif ()

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${KASSERT_CMAKE_NAMESPACE}ConfigVersion.cmake
    VERSION ${kassert_PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion ARCH_INDEPENDENT # requires CMake 3.14
)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/${KASSERT_CMAKE_NAMESPACE}Config.cmake @ONLY
)

include(GNUInstallDirs)
install(
    TARGETS ${KASSERT_CMAKE_NAMESPACE}_kassert
    EXPORT ${KASSERT_CMAKE_NAMESPACE}Targets
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

foreach (file ${kassert_sources})
    get_filename_component(rel_dir ${file} DIRECTORY)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/${KASSERT_INCLUDE_SUBDIR}/${file}
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${KASSERT_INCLUDE_SUBDIR}/${rel_dir}
    )
endforeach ()

install(
    EXPORT ${KASSERT_CMAKE_NAMESPACE}Targets
    FILE ${KASSERT_CMAKE_NAMESPACE}Targets.cmake
    NAMESPACE ${KASSERT_TARGET_PREFIX}::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${KASSERT_INCLUDE_SUBDIR}
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${KASSERT_CMAKE_NAMESPACE}ConfigVersion.cmake ${CMAKE_CURRENT_BINARY_DIR}/${KASSERT_CMAKE_NAMESPACE}Config.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${KASSERT_INCLUDE_SUBDIR}
)
