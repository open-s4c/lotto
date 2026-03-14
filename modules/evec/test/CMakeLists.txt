file(GLOB SRCS *.cpp)

foreach(SRC ${SRCS})
    add_module_tikl_test(${SRC})
endforeach()
