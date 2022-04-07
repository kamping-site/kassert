add_subdirectory("${PROJECT_SOURCE_DIR}/extern/googletest" "extern/googletest")

include(GoogleTest)

# Convenience wrapper for adding tests for Kassert.
#
# TARGET_NAME the target name
# EXCEPTION_MODE option to run tests in exception or assertion mode
# FILES the files of the target
function(kassert_register_test KASSERT_TARGET_NAME)
  cmake_parse_arguments(
    "KASSERT"
    "EXCEPTION_MODE"
    ""
    "FILES"
    ${ARGN}
    )
  add_executable(${KASSERT_TARGET_NAME} ${KASSERT_FILES})
  target_link_libraries(${KASSERT_TARGET_NAME} PRIVATE gtest gtest_main gmock kassert_base)
  target_compile_options(${KASSERT_TARGET_NAME} PRIVATE ${KASSERT_WARNING_FLAGS})
  gtest_discover_tests(${KASSERT_TARGET_NAME} WORKING_DIRECTORY ${PROJECT_DIR})

  if (KASSERT_EXCEPTION_MODE) 
    target_compile_options(${KASSERT_TARGET_NAME} PRIVATE -DKASSERT_EXCEPTION_MODE)
  endif ()
endfunction()
