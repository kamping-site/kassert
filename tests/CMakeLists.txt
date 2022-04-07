list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(KassertTestHelper)
include(GoogleTest)

kassert_register_test(test_kassert_assertion_mode FILES kassert_test.cpp)
kassert_register_test(test_kassert_exception_mode EXCEPTION_MODE FILES kassert_test.cpp)

kassert_register_compilation_failure_test(
  TARGET test_kassert_compilation_failure
  FILES kassert_compilation_failures_test.cpp 
  SECTIONS "FORBIDDEN_AND" "FORBIDDEN_OR"
  LIBRARIES kassert_base
)
