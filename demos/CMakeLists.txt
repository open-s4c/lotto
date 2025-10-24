file(
    GLOB DEMOS
    LIST_DIRECTORIES true
    */)
add_custom_target(demos)
foreach(DEMO ${DEMOS})
    if(NOT IS_DIRECTORY ${DEMO})
        continue()
    endif()
    get_filename_component(DEMO_NAME ${DEMO} NAME)
    add_custom_target(${DEMO_NAME} COMMAND make -C ${DEMO})
    add_dependencies(demos ${DEMO_NAME})
endforeach()
