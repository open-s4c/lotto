include_directories(${PROJECT_SOURCE_DIR}/src/include)
include_directories(${PROJECT_SOURCE_DIR}/src)

file(GLOB SRCS *_test.c)
foreach(SRC ${SRCS})
    get_filename_component(TARGET ${SRC} NAME_WLE)
    add_executable(${TARGET} ${SRC})
    target_link_libraries(
        ${TARGET}
        lotto-runtime-${MODULE_NAME}
        sys_testing.o
        base_testing.o
		statemgr_testing.o
        memmgr_runtime_libc.o
        memmgr_user_libc.o
		runtime-module.o
		dice)
    target_link_options(${TARGET} PUBLIC -rdynamic)
    add_test(NAME ${TARGET} COMMAND ${TARGET})
endforeach()

file(GLOB TIKL_SRCS *.c)
list(FILTER TIKL_SRCS EXCLUDE REGEX "_test\\.c$")

foreach(SRC ${TIKL_SRCS})
    add_module_tikl_test(${SRC})
endforeach()
