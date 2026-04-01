include_directories(${PROJECT_SOURCE_DIR}/src/include)
include_directories(${PROJECT_SOURCE_DIR}/src)

add_compile_definitions(LOTTO_TEST _GNU_SOURCE)
add_compile_definitions(LOGGER_PREFIX="unit_tests")
add_subdirectory(base)
add_subdirectory(sys)
add_subdirectory(engine)
add_subdirectory(runtime)
