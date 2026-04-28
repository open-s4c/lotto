file(GLOB SRCS *.c)

foreach(SRC ${SRCS})
    add_module_tikl_test(${SRC})
endforeach()
