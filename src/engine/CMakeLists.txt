sanitize()
add_compile_options(-fPIC)
add_compile_definitions(_GNU_SOURCE)

add_subdirectory(handlers)

file(GLOB HANDLERS handlers/*.c)

set(MANDATORY_HANDLERS "creation;blocking;watchdog")

foreach(HANDLER ${LOTTO_DISABLE_HANDLERS})
    if(${HANDLER} IN_LIST MANDATORY_HANDLERS)
        message(FATAL_ERROR "Cannot disable mandatory '${HANDLER}' handler")
    endif()
    set(HANDLER_FILE "handler_${HANDLER}.c")
    set(HANDLER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/handlers/${HANDLER_FILE}")
    if(NOT EXISTS ${HANDLER_PATH})
        message(
            FATAL_ERROR
                "Disabling '${HANDLER}' handler: cannot find '${HANDLER_FILE}'")
    endif()
    message(STATUS "Disabling '${HANDLER}' handler")
    list(REMOVE_ITEM HANDLERS ${HANDLER_PATH})
endforeach()

set(SRCS engine.c sequencer.c recorder.c dispatcher.c clock.c ${HANDLERS})

add_library(engine.o OBJECT ${SRCS})
target_compile_options(engine.o PUBLIC -g)

add_library(engine SHARED)
target_link_libraries(
    engine
    PRIVATE sys.o
            base.o
            engine.o
            states.o
            brokers.o
            memmgr_runtime.o
            memmgr_user.o)
set_target_properties(engine PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                        ${CMAKE_BINARY_DIR})

set(LIBTARGET "libengine.so")
make_blob(blob-engine ${LIBTARGET} engine)
make_units(engine "")
