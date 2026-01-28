add_library(lotto.h INTERFACE)
target_include_directories(
    lotto.h INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
                      "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

install(
    DIRECTORY lotto
    DESTINATION include
    COMPONENT LottoSrc
    PATTERN "include/lotto/unsafe/_sys.h" EXCLUDE)

install(
    FILES CMakeLists.txt
    DESTINATION include/
    COMPONENT LottoSrc
    EXCLUDE_FROM_ALL)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/include/lotto/unsafe/sys.h
    COMMAND ${CMAKE_COMMAND} -E make_directory
            ${CMAKE_BINARY_DIR}/include/lotto/unsafe
    COMMAND
        ${CMAKE_C_COMPILER} -E -I${CMAKE_SOURCE_DIR}/src/include
        -I${CMAKE_BINARY_DIR}/include
        "-I$<JOIN:$<TARGET_PROPERTY:vsync,INTERFACE_INCLUDE_DIRECTORIES>,;-I>"
        ${CMAKE_CURRENT_SOURCE_DIR}/lotto/unsafe/_sys.h -o
        ${CMAKE_BINARY_DIR}/include/lotto/unsafe/sys.h
    COMMAND_EXPAND_LISTS
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/lotto/unsafe/_sys.h)

add_custom_target(lotto_unsafe_sys ALL
                  DEPENDS ${CMAKE_BINARY_DIR}/include/lotto/unsafe/sys.h)
