function(run)
    execute_process(COMMAND ${ARGV} RESULT_VARIABLE result
                    OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr TIMEOUT 30)
    if(NOT result STREQUAL "0")
        message(FATAL_ERROR "${ARGV}: ${result}\n${stdout}${stderr}")
    endif()
endfunction()

function(fails expected)
    execute_process(COMMAND ${ARGN} RESULT_VARIABLE result
                    OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr TIMEOUT 30)
    if(result STREQUAL "0" OR NOT "${stdout}${stderr}" MATCHES "${expected}")
        message(FATAL_ERROR
                "Expected '${expected}': ${ARGN}\n${result}\n${stdout}${stderr}")
    endif()
endfunction()

set(out "${WORK}/subscription-slots")
file(MAKE_DIRECTORY "${out}/lotto")
file(READ "${FIXTURES}/system.dice" configuration)
set(flags -std=gnu11 -D_GNU_SOURCE -Wall -Wextra -Werror -fPIC
          "-I${out}" "-I${SOURCE}/include" "-I${DICE_SOURCE}/include"
          "-I${DICE_SOURCE}/src/dice"
          "-I${DICE_SOURCE}/deps/libvsync/include")
set(link -pthread ${DL_LIBS})
if(APPLE)
    set(shared -dynamiclib)
    set(plugin_flags -undefined dynamic_lookup)
    set(suffix .dylib)
else()
    set(shared -shared)
    set(plugin_flags)
    set(suffix .so)
endif()
set(runtime "${out}/runtime${suffix}")
set(early "${out}/early${suffix}")
set(late "${out}/late${suffix}")
set(app "${out}/app")

foreach(callbacks true false)
    string(REPLACE "(callbacks true)" "(callbacks ${callbacks})"
                   config "${configuration}")
    file(WRITE "${out}/system.dice" "${config}")
    run("${DICE}" "${out}/system.dice" -o "${out}")
    configure_file("${out}/dice.h" "${out}/lotto/dice-config.h" COPYONLY)
    foreach(checked 0 1)
        foreach(lto 0 1)
            set(build_flags ${flags} -DDICE_CHECK_ROUTES=${checked})
            if(lto)
                list(APPEND build_flags -O3 -flto)
            endif()
            set(objects)
            foreach(core pubsub mempool pubsub-box)
                set(object "${out}/${core}.o")
                run("${CC}" ${build_flags} -DDICE_ROLLED=1
                    -DDICE_ROLL_RUNTIME -DDICE_MODULE_SLOT=0
                    -DDICE_DISABLE_PUBSUB_CTOR_INIT
                    -include "${out}/dice.h"
                    -c "${DICE_SOURCE}/src/dice/${core}.c" -o "${object}")
                list(APPEND objects "${object}")
            endforeach()
            run("${CC}" ${build_flags} -c "${out}/dice.c"
                -o "${out}/dice.o")
            list(APPEND objects "${out}/dice.o")

            # Sources select one fixed default before including the header.
            # linked.c mixes that default with explicit slot overrides.
            run("${CC}" ${build_flags} -DDICE_DISPATCH_MODULE
                -c "${FIXTURES}/linked.c" -o "${out}/linked.o")
            run("${CC}" ${build_flags} -DDICE_DISPATCH_MODULE
                -c "${FIXTURES}/legacy.c" -o "${out}/legacy.o")
            list(APPEND objects "${out}/linked.o" "${out}/legacy.o")

            foreach(name early late)
                set(variant)
                if(name STREQUAL "late")
                    set(variant -DTEST_LATE)
                endif()
                if(callbacks)
                    # Plugin selection wins over the inherited linked flag.
                    run("${CC}" ${build_flags} -DDICE_DISPATCH_MODULE
                        -DDICE_PLUGIN_MODULE ${variant}
                        ${shared} ${plugin_flags} "${FIXTURES}/plugin.c"
                        -o "${out}/${name}${suffix}")
                else()
                    run("${CC}" ${build_flags} -DDICE_DISPATCH_MODULE
                        ${variant} -c "${FIXTURES}/plugin.c"
                        -o "${out}/${name}.o")
                    list(APPEND objects "${out}/${name}.o")
                endif()
            endforeach()
            run("${CC}" ${build_flags} ${shared} ${objects}
                "${SOURCE}/src/dice.c" ${link} -o "${runtime}")
            run("${CC}" ${build_flags} "${FIXTURES}/main.c"
                "${runtime}" ${link} -o "${app}")
            if(callbacks)
                run("${app}" "${late}" "${early}")
                run("${app}" "${early}" "${late}")
                fails("late subscription" "${app}" "${early}" "${late}"
                      closed)
            else()
                run("${app}")
            endif()

            file(WRITE "${out}/bad.c" [=[
#define DICE_MODULE_SLOT SLOT_SECOND
#include <lotto/dice.h>
PS_SUBSCRIBE_SLOT(CHAIN_REPLY, EVENT_REPLY, SLOT_FIRST, {})
]=])
            fails("dice_consumes" "${CC}" ${build_flags}
                  -DDICE_DISPATCH_MODULE -c "${out}/bad.c"
                  -o "${out}/bad.o")
        endforeach()
    endforeach()
endforeach()
