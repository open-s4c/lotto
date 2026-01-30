sanitize()
add_compile_options(-fPIC)
add_compile_definitions(_GNU_SOURCE)

set(SRCS engine.c sequencer.c recorder.c dispatcher.c clock.c creation.c)
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
