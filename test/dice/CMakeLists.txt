if(NOT CMAKE_CROSSCOMPILING)
    set(dice_test_dl_libs ${CMAKE_DL_LIBS})
    list(TRANSFORM dice_test_dl_libs PREPEND "-l")
    add_test(
        NAME dice-subscription-slots
        COMMAND ${CMAKE_COMMAND}
                -DDICE=$<TARGET_FILE:dice-cli>
                -DCC=${CMAKE_C_COMPILER}
                -DSOURCE=${PROJECT_SOURCE_DIR}
                -DDICE_SOURCE=${DICE_SOURCE}
                -DWORK=${CMAKE_CURRENT_BINARY_DIR}
                -DFIXTURES=${CMAKE_CURRENT_SOURCE_DIR}
                -DAPPLE=${APPLE}
                "-DDL_LIBS=${dice_test_dl_libs}"
                -P ${CMAKE_CURRENT_SOURCE_DIR}/subscriptions.cmake)
    set_tests_properties(dice-subscription-slots PROPERTIES TIMEOUT 180)
endif()
