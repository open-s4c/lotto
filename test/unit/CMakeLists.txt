include_directories(${PROJECT_SOURCE_DIR}/src/include)
include_directories(${PROJECT_SOURCE_DIR}/src)

add_compile_definitions(LOTTO_TEST _GNU_SOURCE)
add_subdirectory(base)
add_subdirectory(sys)
add_subdirectory(engine)
add_subdirectory(runtime)
# add_subdirectory(cli)
