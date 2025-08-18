# ensure that we do not try to define the test targets again when using the vendoring feature
include_guard()

include(cmake/KassertTestHelper.cmake)
include(GoogleTest)

kassert_register_test(test_kassert_assertion_mode FILES kassert_test.cpp)
kassert_register_test(test_kassert_exception_mode EXCEPTION_MODE FILES kassert_test.cpp)

###################
# Vendoring tests #
###################
#  since we inherit variables from the parent scope, we need to unset the vendoring variables
# here
set(KASSERT_VENDOR_ID "mykassert")
unset(KASSERT_CMAKE_NAMESPACE)
unset(KASSERT_CXX_NAMESPACE)
unset(KASSERT_INCLUDE_SUBDIR)
unset(KASSERT_PREFIX)
set(KASSERT_INSTALL OFF) # we do not want to install the vendored kassert

# now include kassert again, but with the vendoring variables set
add_subdirectory(.. ${CMAKE_CURRENT_BINARY_DIR}/vendored)

add_executable(test_vendoring vendoring_test.cpp)
target_link_libraries(test_vendoring PRIVATE GTest::gtest_main)
target_link_libraries(test_vendoring PRIVATE mykassert::kassert kassert::kassert)
# set different assertion levels for both kassert and mykassert
set_target_properties(test_vendoring PROPERTIES KASSERT_ASSERTION_LEVEL 30)
set_target_properties(test_vendoring PROPERTIES MYKASSERT_ASSERTION_LEVEL 10)
gtest_discover_tests(test_vendoring)

