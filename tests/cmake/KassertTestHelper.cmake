include(FetchContent)

set(INSTALL_GTEST OFF)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest
    GIT_TAG v1.15.2
)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

# Convenience wrapper for adding tests for Kassert.
#
# TARGET_NAME the target name EXCEPTION_MODE option to run tests in exception or assertion mode FILES the files of the
# target
function (kassert_register_test KASSERT_TARGET_NAME)
    cmake_parse_arguments("KASSERT" "EXCEPTION_MODE" "" "FILES" ${ARGN})
    add_executable(${KASSERT_TARGET_NAME} ${KASSERT_FILES})
    target_link_libraries(${KASSERT_TARGET_NAME} PRIVATE gtest gtest_main gmock kassert)
    target_link_libraries(${KASSERT_TARGET_NAME} PRIVATE kassert_warnings)
    gtest_discover_tests(${KASSERT_TARGET_NAME})

    if (KASSERT_EXCEPTION_MODE)
        set_target_properties(${KASSERT_TARGET_NAME} PROPERTIES KASSERT_EXCEPTION_MODE ON)
    endif ()
endfunction ()

# Registers a set of tests which should fail to compile.
#
# TARGET prefix for the targets to be built FILES the list of files to include in the target SECTIONS sections of the
# compilation test to build LIBRARIES libraries to link via target_link_libraries(...)
#
# Loosely based on: https://stackoverflow.com/questions/30155619/expected-build-failure-tests-in-cmake
function (kassert_register_compilation_failure_test)
    cmake_parse_arguments(
        "KATESTROPHE" # prefix
        "" # options
        "TARGET" # one value arguments
        "FILES;SECTIONS;LIBRARIES" # multiple value arguments
        ${ARGN}
    )

    # the file should compile without any section enabled
    add_executable(${KATESTROPHE_TARGET} ${KATESTROPHE_FILES})
    target_link_libraries(${KATESTROPHE_TARGET} PUBLIC gtest ${KATESTROPHE_LIBRARIES})

    # For each given section, add a target.
    foreach (SECTION ${KATESTROPHE_SECTIONS})
        string(TOLOWER ${SECTION} SECTION_LOWERCASE)
        set(THIS_TARGETS_NAME "${KATESTROPHE_TARGET}.${SECTION_LOWERCASE}")

        # Add the executable and link the libraries.
        add_executable(${THIS_TARGETS_NAME} ${KATESTROPHE_FILES})
        target_link_libraries(${THIS_TARGETS_NAME} PUBLIC gtest ${KATESTROPHE_LIBRARIES})

        # Select the correct section of the target by setting the appropriate preprocessor define.
        target_compile_definitions(${THIS_TARGETS_NAME} PRIVATE ${SECTION})

        # Exclude the target fromn the "all" target.
        set_target_properties(${THIS_TARGETS_NAME} PROPERTIES EXCLUDE_FROM_ALL TRUE EXCLUDE_FROM_DEFAULT_BUILD TRUE)

        # Add a test invoking "cmake --build" to test if the target compiles.
        add_test(
            NAME "${THIS_TARGETS_NAME}"
            COMMAND cmake --build . --target ${THIS_TARGETS_NAME} --config $<CONFIGURATION>
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )

        # Specify, that the target should not compile.
        set_tests_properties("${THIS_TARGETS_NAME}" PROPERTIES WILL_FAIL TRUE)
    endforeach ()
endfunction ()
