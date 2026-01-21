macro(make_blob TARGET FILE FILE_TARGET)
    if("${FILE_TARGET}" STREQUAL "")
        set(TARGET_DEPS ${FILE})
        get_filename_component(DIR ${FILE} DIRECTORY)
        get_filename_component(NAME ${FILE} NAME)
    else()
        # assume FILE_TARGET is a library target, then use $<TARGET_FILE:.> to
        # get the file name, and use LIBRARY_OUTPUT_DIRECTORY property as
        # working directory. Otherwise, xxd will prepend the whole path in the
        # variable name of the encoded file.
        set(TARGET_DEPS ${FILE_TARGET})
        set(FILE $<TARGET_FILE:${FILE_TARGET}>)
        get_target_property(DIR ${FILE_TARGET} LIBRARY_OUTPUT_DIRECTORY)
        if(${DIR} STREQUAL DIR-NOTFOUND)
            message(FATAL "Could not find OUTPUT_DIRECTORY of ${TARGET}")
        endif()
        get_filename_component(NAME ${FILE} NAME)
    endif()

    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/include/${TARGET}.h
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/include
        COMMAND xxd -i ${NAME} ${CMAKE_BINARY_DIR}/include/${TARGET}.h
        DEPENDS ${TARGET_DEPS}
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
