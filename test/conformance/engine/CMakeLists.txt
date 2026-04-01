add_compile_definitions(LOGGER_PREFIX="conformance")
add_subdirectory(mock)

add_racket_test(FILENAME engine_test.rkt)
add_racket_test(FILENAME engine_prop_test.rkt)
