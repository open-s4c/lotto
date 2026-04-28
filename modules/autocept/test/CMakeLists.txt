set(AUTOCEPT_TEST_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/foo.h)
add_library(autocept_testing SHARED test_interceptors.S)
target_compile_definitions(
    autocept_testing
    PRIVATE DICE_MULTIFILE_MODULE LOTTO_RUNTIME_MODULE=1 LOTTO_DRIVER_MODULE=0
            DICE_MODULE_SLOT=5 LOGGER_PREFIX="autocept")
target_link_libraries(
    autocept_testing PRIVATE lotto.h dice.h dice.o dice-self.o autocept-bare.o
                             ${CMAKE_DL_LIBS})
set_target_properties(autocept_testing PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                                  ${CMAKE_CURRENT_BINARY_DIR})

add_library(autocept_test_mock SHARED autocept_mock.c variadic_mock.c)
target_compile_definitions(autocept_test_mock PRIVATE LOGGER_PREFIX="test")
target_include_directories(autocept_test_mock
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(autocept_test_mock PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                                    ${CMAKE_CURRENT_BINARY_DIR})

add_library(autocept_checker SHARED checker.c)
target_compile_definitions(autocept_checker PRIVATE LOGGER_PREFIX="test")
target_link_libraries(autocept_checker PRIVATE dice dice.h dice-self)
set_target_properties(autocept_checker PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                                  ${CMAKE_CURRENT_BINARY_DIR})

add_executable(autocept_baseline_test baseline.c)
target_compile_definitions(autocept_baseline_test PRIVATE LOGGER_PREFIX="test")
add_dependencies(autocept_baseline_test autocept_testing)
add_dependencies(autocept_baseline_test autocept_checker)
target_include_directories(autocept_baseline_test
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(autocept_baseline_test PRIVATE autocept_test_mock)

add_test(NAME autocept-baseline-test COMMAND autocept_baseline_test)
add_test(
    NAME autocept-interposed-test
    COMMAND
        env DICE_SOURCE=${PROJECT_SOURCE_DIR}/deps/dice
        DICE_BUILD=${PROJECT_BINARY_DIR}/deps/dice
        ${PROJECT_SOURCE_DIR}/deps/dice/scripts/dice -preload
        $<TARGET_FILE:autocept_testing>:$<TARGET_FILE:autocept_checker>
        $<TARGET_FILE:autocept_baseline_test>)
