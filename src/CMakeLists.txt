add_subdirectory(include)

# make the inclusion of ensure.h an error. add_compile_options(-DENSURE_DISABLE)
include_directories(${CMAKE_BINARY_DIR}/generated)

include_directories(include)

add_compile_definitions(DICE_MULTIFILE_MODULE)
if(${LOTTO_LTO})
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
endif()

add_subdirectory(sys)
add_subdirectory(base)
add_subdirectory(engine)
add_subdirectory(runtime)
add_subdirectory(driver)
add_subdirectory(cli)
