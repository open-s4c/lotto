# The runtime, driver, and loadable modules share a single generated ABI.
function(lotto_prepare_dice)
    option(LOTTO_CHECK_ROUTES "Check Lotto's declared Dice routes" OFF)
    set(checks)
    if(LOTTO_CHECK_ROUTES)
        list(APPEND checks CHECK_ROUTES)
    endif()
    file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/dice")
    foreach(component runtime driver)
        set(target lotto-${component}-dice)
        add_library(${target} OBJECT)
        set_property(TARGET ${target} PROPERTY INTERPROCEDURAL_OPTIMIZATION
                                               ${LOTTO_LTO})
        # Preserve Lotto's lazily backed pool instead of clearing 2 GiB at boot.
        target_compile_definitions(${target} PRIVATE DICE_MEMPOOL_USE_MMAP)
        add_dice_core(
            ${target}
            "${PROJECT_SOURCE_DIR}/lotto.dice"
            ${checks}
            IMPORT_PATHS
            "lotto=${PROJECT_SOURCE_DIR}/src/dice"
            "lotto=${PROJECT_SOURCE_DIR}/modules"
            "lotto=${PROJECT_BINARY_DIR}/dice")
        if(APPLE)
            target_compile_definitions(${target}
                                       PRIVATE DICE_DISABLE_PUBSUB_CTOR_INIT)
        endif()
    endforeach()
    set(modules
        self
        cxa
        mman
        malloc
        pthread_cond
        pthread_create
        pthread_mutex
        pthread_rwlock
        sem
        stacktrace
        tsan)
    if(NOT APPLE)
        list(APPEND modules pthread_spinlock)
    endif()
    add_dice_mods(lotto-runtime-dice ${modules})

    get_target_property(header lotto-runtime-dice DICE_ROLL_HEADER)
    set(public_header "${PROJECT_BINARY_DIR}/include/lotto/dice-config.h")
    add_custom_command(
        OUTPUT "${public_header}"
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "${PROJECT_BINARY_DIR}/include/lotto"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${header}"
                "${public_header}"
        DEPENDS "${header}" lotto-runtime-dice-dice-generate
        VERBATIM)
    add_custom_target(lotto-dice-header DEPENDS "${public_header}")
    add_library(lotto-dice.h INTERFACE)
    add_dependencies(lotto-dice.h lotto-dice-header)
    target_link_libraries(lotto-dice.h INTERFACE lotto-runtime-dice-dice.h)
    target_include_directories(
        lotto-dice.h INTERFACE "${PROJECT_SOURCE_DIR}/include"
                               "${PROJECT_BINARY_DIR}/include")
    target_compile_options(
        lotto-dice.h
        INTERFACE "$<$<COMPILE_LANGUAGE:C,CXX>:-include;lotto/dice.h>")
    install(
        FILES "${header}"
        DESTINATION include/lotto
        RENAME dice-config.h)
    add_library(lotto-subscriptions.o OBJECT "${PROJECT_SOURCE_DIR}/src/dice.c")
    set_property(TARGET lotto-subscriptions.o
                 PROPERTY INTERPROCEDURAL_OPTIMIZATION ${LOTTO_LTO})
    target_link_libraries(lotto-subscriptions.o PRIVATE lotto-dice.h)
endfunction()

function(lotto_dice_slot output name)
    string(REPLACE "-" "_" slot "${name}")
    string(TOUPPER "${slot}" slot)
    set(${output}
        "SLOT_LOTTO_${slot}"
        PARENT_SCOPE)
endfunction()

function(lotto_dice_module target name kind)
    lotto_dice_slot(slot "${name}")
    # Replace legacy directory defaults used by isolated unit-test objects.
    target_compile_options(
        ${target}
        PRIVATE
            "$<$<COMPILE_LANGUAGE:C,CXX>:-UDICE_MODULE_SLOT;-DDICE_MODULE_SLOT=${slot}>"
    )
    target_link_libraries(${target} PRIVATE lotto-dice.h)
    if(kind STREQUAL "PLUGIN")
        target_compile_definitions(${target} PRIVATE DICE_PLUGIN_MODULE)
    endif()
endfunction()

function(lotto_finalize_dice)
    get_property(imports GLOBAL PROPERTY LOTTO_DICE_IMPORTS)
    list(REMOVE_DUPLICATES imports)
    set(config "; Selected by Lotto's CMake module options.\n")
    foreach(name IN LISTS imports)
        string(APPEND config "(import (lotto ${name}))\n")
    endforeach()
    if(NOT APPLE)
        string(APPEND config "(import (dice pthread_spinlock))\n")
    endif()

    file(CONFIGURE OUTPUT "${PROJECT_BINARY_DIR}/dice/selected.dice" CONTENT
         "${config}" @ONLY)

    foreach(
        target
        mediator.o
        switcher.o
        memmgr_runtime_dice.o
        lotto-runtime-phases.o
        lotto
        lotto-dbg
        runtime-module.o)
        lotto_dice_module(${target} runtime BUILTIN)
    endforeach()
    lotto_dice_module(subscribers.o threads BUILTIN)
    foreach(target cli.o lotto-driver-phases.o lotto-driver)
        lotto_dice_module(${target} driver BUILTIN)
    endforeach()
endfunction()
