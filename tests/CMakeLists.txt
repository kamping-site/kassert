list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(KassertTestHelper)
include(GoogleTest)

kassert_register_test(test_kassert_assertion_mode FILES kassert_test.cpp)
kassert_register_test(test_kassert_exception_mode EXCEPTION_MODE FILES kassert_test.cpp)
