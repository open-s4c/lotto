include_directories(include)
add_subdirectory(src)

if(NOT ${LOTTO_RACKET_TESTS})
    return()
endif()

add_subdirectory(test)
