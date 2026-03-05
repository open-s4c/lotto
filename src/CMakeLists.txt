add_subdirectory(include)

# make the inclusion of ensure.h an error. add_compile_options(-DENSURE_DISABLE)

include_directories(${CMAKE_BINARY_DIR}/generated)

include_directories(include)
include_directories(../modules/termination/include)
include_directories(../modules/enforce/include)
include_directories(../modules/available/include)

add_compile_definitions(DICE_MULTIFILE_MODULE)

add_subdirectory(sys)
add_subdirectory(base)
add_subdirectory(engine)
add_subdirectory(subscribers)
add_subdirectory(runtime)
add_subdirectory(cli)
