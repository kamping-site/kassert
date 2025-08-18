# ensure that we do not try to define the test targets again when using the vendoring feature
include_guard()

include(cmake/KassertTestHelper.cmake)
include(GoogleTest)

configure_file(kassert_test.cpp.in kassert_test.cpp @ONLY)
kassert_register_test(test_kassert_assertion_mode FILES ${CMAKE_CURRENT_BINARY_DIR}/kassert_test.cpp)
kassert_register_test(test_kassert_exception_mode EXCEPTION_MODE FILES ${CMAKE_CURRENT_BINARY_DIR}/kassert_test.cpp)

# ######################################################################################################################
# Vendoring tests
# ######################################################################################################################

# since we inherit variables from the parent scope, we need to unset the vendoring variables here store old values
set(old_KASSERT_VENDOR_ID ${KASSERT_VENDOR_ID})
set(old_KASSERT_CMAKE_NAMESPACE ${KASSERT_CMAKE_NAMESPACE})
set(old_KASSERT_CXX_NAMESPACE ${KASSERT_CXX_NAMESPACE})
set(old_KASSERT_INCLUDE_SUBDIR ${KASSERT_INCLUDE_SUBDIR})
set(old_KASSERT_PREFIX ${KASSERT_PREFIX})
unset(KASSERT_VENDOR_ID)
unset(KASSERT_CMAKE_NAMESPACE)
unset(KASSERT_CXX_NAMESPACE)
unset(KASSERT_INCLUDE_SUBDIR)
unset(KASSERT_PREFIX)

set(KASSERT_VENDOR_ID "mykassert")
set(KASSERT_INCLUDE_SUBDIR "${old_KASSERT_INCLUDE_SUBDIR}/mykassert")
set(KASSERT_INSTALL OFF) # we do not want to install the vendored kassert
set(KASSERT_BUILD_DOCS OFF)

# now include kassert again, but with the vendoring variables set
add_subdirectory(.. ${CMAKE_CURRENT_BINARY_DIR}/vendored)

# restore old values
set(KASSERT_VENDOR_ID ${old_KASSERT_VENDOR_ID})
set(KASSERT_CMAKE_NAMESPACE ${old_KASSERT_CMAKE_NAMESPACE})
set(KASSERT_CXX_NAMESPACE ${old_KASSERT_CXX_NAMESPACE})
set(KASSERT_INCLUDE_SUBDIR ${old_KASSERT_INCLUDE_SUBDIR})
set(KASSERT_PREFIX ${old_KASSERT_PREFIX})

configure_file(vendoring_test.cpp.in vendoring_test.cpp @ONLY)
add_executable(test_vendoring ${CMAKE_CURRENT_BINARY_DIR}/vendoring_test.cpp)
target_link_libraries(test_vendoring PRIVATE GTest::gtest_main)
target_link_libraries(test_vendoring PRIVATE mykassert::kassert ${KASSERT_CMAKE_NAMESPACE}::kassert)
# set different assertion levels for both kassert and mykassert
set_target_properties(test_vendoring PROPERTIES ${KASSERT_PREFIX}_ASSERTION_LEVEL 30)
set_target_properties(test_vendoring PROPERTIES MYKASSERT_ASSERTION_LEVEL 10)
gtest_discover_tests(test_vendoring)

FetchContent_Declare(
    InstallTest.cmake
    GIT_REPOSITORY https://github.com/niklas-uhl/InstallTest.cmake
    GIT_TAG main
)
FetchContent_MakeAvailable(InstallTest.cmake)
add_install_test(NAME kassert_install SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/..)
test_subdirectory(
    NAME kassert_install_works SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/install-test INSTALL_NAMES kassert_install
)
