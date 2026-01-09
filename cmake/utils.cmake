macro(make_blob TARGET FILE FILE_TARGET)
    get_filename_component(DIR ${FILE} DIRECTORY)
    get_filename_component(NAME ${FILE} NAME)
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/include/${TARGET}.h
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/include
        # xxd's first argument is the input file, the second the output file.
        # it's not possible to use `$<TARGET_FILE>`, since the input file name
        # is relied upon in preload.c, so the command relies on the fact that
        # TARGET is defined in the same BINARY_DIR.
        COMMAND xxd -i ${NAME} ${CMAKE_BINARY_DIR}/include/${TARGET}.h
        DEPENDS ${FILE_TARGET} ${FILE}
        WORKING_DIRECTORY ${DIR})
    add_custom_target(${TARGET} DEPENDS ${CMAKE_BINARY_DIR}/include/${TARGET}.h)
endmacro()

macro(make_units PREFIX EXCLUDES)
    file(GLOB UNITS *.c)
    foreach(EXCLUDE ${EXCLUDES})
        list(REMOVE_ITEM UNITS "${EXCLUDE}")
    endforeach()
    foreach(SRC ${UNITS})
        get_filename_component(BASE ${SRC} NAME_WLE)
        set(TARGET ${PREFIX}_${BASE}_testing.o)
        add_library(${TARGET} OBJECT ${SRC})
        target_compile_definitions(${TARGET} PUBLIC -DLOTTO_TEST)
    endforeach()
endmacro()
