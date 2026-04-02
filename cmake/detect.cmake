# ##############################################################################
# Capability detection
# ##############################################################################

find_program(CARGO_EXECUTABLE cargo)
if(CARGO_EXECUTABLE)
    execute_process(
        COMMAND ${CARGO_EXECUTABLE} --version
        RESULT_VARIABLE _cargo_result
        OUTPUT_QUIET
        ERROR_QUIET)
    if(_cargo_result EQUAL 0)
        set(HAVE_CARGO ON)
    else()
        set(HAVE_CARGO OFF)
    endif()
else()
    set(HAVE_CARGO OFF)
endif()

find_package(Curses QUIET)
if(CURSES_FOUND)
    set(HAVE_CURSES ON)
else()
    set(HAVE_CURSES OFF)
endif()
