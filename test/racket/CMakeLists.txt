add_custom_target(
    racket-format-apply
    COMMAND env SILENT=true ${PROJECT_SOURCE_DIR}/scripts/racket-format.sh
            ${CMAKE_SOURCE_DIR})
add_custom_target(
    racket-format-check
    COMMAND env ${PROJECT_SOURCE_DIR}/scripts/racket-format.sh
            ${CMAKE_SOURCE_DIR})

if(NOT ${LOTTO_RACKET_TESTS})
    return()
endif()

add_subdirectory(base)
add_subdirectory(engine)
