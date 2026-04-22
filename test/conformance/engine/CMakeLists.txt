add_compile_definitions(LOGGER_PREFIX="conformance")
if(${LOTTO_MOCKOTO_TESTS})
    add_subdirectory(mock)

    add_racket_test(FILENAME engine_test.rkt)
    add_racket_test(FILENAME engine_prop_test.rkt)
endif()
